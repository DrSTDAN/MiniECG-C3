#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <NimBLEDevice.h>     // NimBLE-Arduino 2.3.4
#include <esp_timer.h>        // <-- muestreador periódico μs

// ========= BOARD Abrobot ESP32-C3 =========
#define I2C_SDA     5
#define I2C_SCL     6
#define PIN_ECG_ADC 2
#define PIN_LO_PLUS 10
#define PIN_LO_MINUS 7
#define LED_PIN     8
// =========================================

#define SERIAL_BAUD   921600
#define SAMPLE_HZ     250
#define BLE_DEVICE_NAME "MiniECG C3"

// Ventana grafico (OLED 128x64, área útil 72x40)
#define VIEW_W      72
#define VIEW_H      40
#define VIEW_XOFF   27
#define VIEW_YOFF   24

// UUIDs NUS-like
#define NUS_SVC_UUID  "569a1101-b87f-490c-92cb-11ba5ea5167c"
#define NUS_RX_UUID   "569a2000-b87f-490c-92cb-11ba5ea5167c"
#define NUS_TX_UUID   "569a2001-b87f-490c-92cb-11ba5ea5167c"

#define ADC_BITS 12
#define ADC_MAX  4095

// OLED
#define OLED_DRIVER_SH1106 0
#if OLED_DRIVER_SH1106
  U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
#else
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
#endif

bool    OLED_PRESENT = false;
uint8_t OLED_ADDR    = 0x3C;

// ---- BLE (NimBLE) ----
NimBLEServer*          pServer = nullptr;
NimBLECharacteristic*  pChrTx  = nullptr;  // NOTIFY/INDICATE (C3 -> app)
NimBLECharacteristic*  pChrRx  = nullptr;  // WRITE/WRITE_NR  (app -> C3)
volatile bool          txSubscribed = false; // true cuando CCCD habilitado

// ---- Plot (72 puntos de ancho) ----
uint8_t traceY[VIEW_W];
uint8_t xCursor = 0;

// ---- Streaming control ----
volatile bool streamEnabled = true;   // START/STOP
volatile int  asciiEveryN   = 1;      // 1 => 250 Hz efectivos
int sampleDecim = 0;

// ---- Paquetizado ASCII ----
#define PACK_N 10                        // 10 muestras por línea -> 25 notifies/s
uint16_t packBuf[PACK_N];
int      packCount = 0;
uint32_t lastPackMs = 0;

// ---- Reset BLE diferido tras desconexión ----
volatile bool needBleReset = false;
uint32_t scheduleBleResetAt = 0;

// ============ MUESTREO ESTABLE ============
// Ring buffer para desacoplar muestreo/OLED/BLE
#define RB_BITS 10
#define RB_SIZE (1u << RB_BITS)  // 1024
#define RB_MASK (RB_SIZE - 1)
volatile uint16_t rb[RB_SIZE];
volatile uint32_t rbHead = 0, rbTail = 0;

// (Opcional) median-of-3 para quitar chisporroteo sin engordar QRS
#define USE_MEDIAN3 1

static inline uint16_t median3(uint16_t a, uint16_t b, uint16_t c) {
  // mediana sin floats
  if (a > b) { uint16_t t=a; a=b; b=t; }
  if (b > c) { uint16_t t=b; b=c; c=t; }
  if (a > b) { uint16_t t=a; a=b; b=t; }
  return b;
}

esp_timer_handle_t sampleTimer;

static void IRAM_ATTR onSample(void* /*arg*/) {
  static uint16_t p1=0, p2=0;
  uint16_t v = analogRead(PIN_ECG_ADC);  // permitido en callback de esp_timer

#if USE_MEDIAN3
  uint16_t m = median3(p1, p2, v);
  p1 = p2; p2 = v;
  v = m;
#endif

  rb[rbHead & RB_MASK] = v;
  rbHead++;
}
// ==========================================

// ===== Helpers =====
static inline uint8_t mapAdcToY(uint16_t v){
  // mapeo lineal 0..4095 -> 0..VIEW_H-1 (arriba es mayor)
  return (uint8_t)(VIEW_H-1 - ((uint32_t)v * (VIEW_H-1) / ADC_MAX));
}
bool leadsOff(){ return (digitalRead(PIN_LO_PLUS)==HIGH) || (digitalRead(PIN_LO_MINUS)==HIGH); }
static inline bool isConnected(){ return pServer && pServer->getConnectedCount() > 0; }

// ---- Prototipos ----
static void startAdvertisingSafe();
static void doBleReset();

// --- Callbacks BLE (NimBLE 2.3.4) ---
class MyServerCb : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* , NimBLEConnInfo& ) override {
    Serial.println("onConnect");
    txSubscribed = false;
    streamEnabled = true;
    asciiEveryN = 1;
    packCount = 0;
    if (pChrTx) { const char* r = "READY\r\n"; pChrTx->setValue((uint8_t*)r, 7); pChrTx->notify(); }
  }
  void onDisconnect(NimBLEServer* , NimBLEConnInfo& , int reason) override {
    Serial.printf("onDisconnect (reason=%d)\n", reason);
    txSubscribed = false;
    streamEnabled = false;
    packCount = 0;
    // Re-ADV inmediato + reset completo del stack fuera del callback
    startAdvertisingSafe();
    needBleReset = true;
    scheduleBleResetAt = millis() + 250;
  }
};

// RX: comandos desde la app (START/STOP, RATE=nn)
class RxCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* c, NimBLEConnInfo& ) override {
    std::string s = c->getValue();
    if (pChrTx) { const char* ok = "OK\r\n"; pChrTx->setValue((uint8_t*)ok, 4); pChrTx->notify(); }

    for (char &ch: s) if(ch>='a' && ch<='z') ch -= 32;
    while(!s.empty() && (s.back()=='\r' || s.back()=='\n')) s.pop_back();

    if (s == "START")      { streamEnabled = true;  Serial.println("CMD START"); }
    else if (s == "STOP")  { streamEnabled = false; Serial.println("CMD STOP"); }
    else if (s.rfind("RATE=", 0) == 0) {
      int hz = atoi(s.c_str()+5);
      if (hz < 1) hz = 1;
      if (hz > SAMPLE_HZ) hz = SAMPLE_HZ;
      int n = SAMPLE_HZ / hz;      // 250->1, 125->2, 50->5...
      if (n < 1) n = 1;
      asciiEveryN = n;
      Serial.printf("CMD RATE=%dHz -> cada %d muestras\n", hz, asciiEveryN);
      packCount = 0;
    }
  }
};

// TX: saber cuando Android habilita NOTIFY/INDICATE (CCCD escrito)
class TxCallbacks : public NimBLECharacteristicCallbacks {
  void onSubscribe(NimBLECharacteristic* , NimBLEConnInfo& , uint16_t subValue) override {
    txSubscribed = (subValue != 0);
    Serial.printf("onSubscribe TX: 0x%04X (subscribed=%d)\n", subValue, txSubscribed ? 1 : 0);
    packCount = 0;
    lastPackMs = millis();
  }
};

void setupBLE(){
  // Init NimBLE con dirección pública estable (evita RPA que “pierde” al móvil)
  NimBLEDevice::init(BLE_DEVICE_NAME);
  NimBLEDevice::setDeviceName(BLE_DEVICE_NAME);
  NimBLEDevice::setOwnAddrType(BLE_OWN_ADDR_PUBLIC);
  NimBLEDevice::setSecurityAuth(false, false, false);
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  NimBLEDevice::setMTU(247);

  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new MyServerCb());
  pServer->advertiseOnDisconnect(false);   // lo gestionamos manualmente

  // Servicio
  NimBLEService* svc = pServer->createService(NUS_SVC_UUID);

  // TX
  pChrTx = svc->createCharacteristic(NUS_TX_UUID, NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::INDICATE);
  {
    NimBLEDescriptor* cccd = pChrTx->createDescriptor(
      NimBLEUUID((uint16_t)0x2902), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, 2);
    uint8_t zero[2] = {0x00, 0x00};
    cccd->setValue(zero, 2);
  }
  pChrTx->setCallbacks(new TxCallbacks());

  // RX
  pChrRx = svc->createCharacteristic(NUS_RX_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  pChrRx->setCallbacks(new RxCallbacks());

  svc->start();

  startAdvertisingSafe();
  Serial.println("BLE NUS-like adv started");
}

// ADV robusto: reconfigura siempre, para y arranca.
static void startAdvertisingSafe() {
  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  if (!adv) return;

  adv->stop();
  delay(30);

  adv->setScanFilter(false, false);
  adv->addServiceUUID(NUS_SVC_UUID);
  adv->setMinInterval(160); // ~100 ms
  adv->setMaxInterval(240); // ~150 ms

  NimBLEAdvertisementData advData, scanData;
  advData.setFlags(0x06);
  advData.setName(BLE_DEVICE_NAME);
  scanData.setName(BLE_DEVICE_NAME);
  adv->setAdvertisementData(advData);
  adv->setScanResponseData(scanData);

  adv->start();
  Serial.println("ADV (re)started");
}

// Reset completo del stack BLE (fuera del callback)
static void doBleReset(){
  Serial.println("BLE reset: deinit -> reinit");
  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  if (adv) adv->stop();
  delay(30);
  NimBLEDevice::deinit(true);  // libera host/controller
  delay(50);
  setupBLE();                  // reconstruye todo y re-ADV
}

// I2C / OLED
void tryDetectOLED(){
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000);
  delay(20);
  Wire.beginTransmission(OLED_ADDR);
  OLED_PRESENT = (Wire.endTransmission() == 0);
  Serial.printf("OLED %s en 0x%02X (SDA=%d SCL=%d)\n", OLED_PRESENT?"OK":"NO", OLED_ADDR, I2C_SDA, I2C_SCL);
}
void setupOLED(){
  u8g2.setI2CAddress(OLED_ADDR << 1);
  u8g2.begin();
  u8g2.setContrast(200);
  u8g2.setFlipMode(0);
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.drawStr(0, 10, "MiniECG C3 (NUS)");
  u8g2.sendBuffer();
}
void drawViewportFrame(){
  u8g2.drawFrame(VIEW_XOFF-1, VIEW_YOFF-1, VIEW_W+2, VIEW_H+2);
  for(int x=0; x<VIEW_W; x+=12) u8g2.drawVLine(VIEW_XOFF+x, VIEW_YOFF, VIEW_H);
  for(int y=0; y<VIEW_H; y+=10) u8g2.drawHLine(VIEW_XOFF, VIEW_YOFF+y, VIEW_W);
}

// Empaqueta y envía "v1 v2 ... vN\r\n"
void flushPackIfNeeded(bool force=false) {
  if (!pChrTx || !txSubscribed || !isConnected() || packCount == 0) return;
  if (!force) {
    if (millis() - lastPackMs < 35) return; // ~25 notifs/s
  }
  char line[200];
  int n = 0;
  for (int i=0; i<packCount; ++i) {
    n += snprintf(line+n, sizeof(line)-n, (i==packCount-1) ? "%u\r\n" : "%u ", (unsigned)packBuf[i]);
    if (n >= (int)sizeof(line)-8) break;
  }
  pChrTx->setValue((uint8_t*)line, n);
  pChrTx->notify();
  packCount = 0;
  lastPackMs = millis();
}

void setup(){
  Serial.begin(SERIAL_BAUD);
  pinMode(LED_PIN, OUTPUT);
  pinMode(PIN_LO_PLUS, INPUT_PULLUP);
  pinMode(PIN_LO_MINUS, INPUT_PULLUP);
  for(int i=0;i<VIEW_W;i++) traceY[i]=VIEW_H/2;

  analogReadResolution(ADC_BITS);

  setupBLE();
  tryDetectOLED();
  if (OLED_PRESENT) setupOLED();

  // --- Inicia muestreador periódico a 250 Hz (cada 4000 μs) ---
  esp_timer_create_args_t targs = {};
  targs.callback = &onSample;
  targs.name = "sampler";
  ESP_ERROR_CHECK(esp_timer_create(&targs, &sampleTimer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(sampleTimer, 1000000ULL / SAMPLE_HZ));
}

void loop(){
  // LED estado
  static bool led=false; static uint32_t tLed=0;
  if (isConnected()) digitalWrite(LED_PIN, HIGH);
  else if (millis()-tLed > 300) { tLed=millis(); led=!led; digitalWrite(LED_PIN, led); }

  // Reset BLE diferido si fue solicitado en onDisconnect
  if (needBleReset && millis() >= scheduleBleResetAt) {
    needBleReset = false;
    doBleReset();
  }

  // Drenar ring buffer: aquí ya NO alteramos el ritmo de muestreo
  while (rbTail != rbHead) {
    uint16_t adc = rb[rbTail & RB_MASK];
    rbTail++;

    // Plot local (72 px de ancho)
    uint8_t y = mapAdcToY(adc);
    traceY[xCursor] = y;
    xCursor = (xCursor+1) % VIEW_W;

    // Envío (diezmado según RATE) + paquete
    if (isConnected() && streamEnabled && txSubscribed) {
      sampleDecim++;
      if (sampleDecim >= asciiEveryN) {
        sampleDecim = 0;
        if (packCount < PACK_N) packBuf[packCount++] = adc;
        if (packCount >= PACK_N) flushPackIfNeeded(true);
      }
    }
  }

  // timeout para vaciar paquete si quedó a medias
  if (packCount > 0 && millis() - lastPackMs > 80) flushPackIfNeeded(true);

  // Watchdog de advertising (por si el controller se queda callado)
  static uint32_t tAdv=0;
  if (!isConnected() && millis()-tAdv > 1500) {
    tAdv = millis();
    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    if (adv && !adv->isAdvertising()) {
      startAdvertisingSafe();
      Serial.println("Re-ADV (watchdog)");
    }
  }

  // Refresco OLED (≈20 FPS)
  if (OLED_PRESENT) {
    static uint32_t tLast=0;
    if(millis() - tLast >= 250){      // aqui esta en 50 si necesitamos imprimir ecg en el oled display
      tLast = millis();
      u8g2.clearBuffer();
   //   drawViewportFrame();                                    //aqui esta el frame y el display de el ecg en la pantalla oled
    //  for(int i=0;i<VIEW_W;i++){
     //   int i0=(xCursor+i)%VIEW_W, i1=(xCursor+i+1)%VIEW_W;   //lo comentamos porrque reduce la frequencia de datos en la salida ble en un futuro lo vamos a tratar de integrar de otra forma menos 
     //   int x0 = VIEW_XOFF + i;                               // consumible de tiempo procesamiento en la operacion de draw en i2c
    //    int x1 = VIEW_XOFF + (i+1 < VIEW_W ? i+1 : i);
    //    int y0 = VIEW_YOFF + traceY[i0];
      //  int y1 = VIEW_YOFF + traceY[i1];
     //   u8g2.drawLine(x0, y0, x1, y1);
     // }
      u8g2.setFont(u8g2_font_6x12_tr);
      u8g2.drawStr(28, 35, leadsOff()? "LEADS OFF!!!" : "ECG OK");
      u8g2.drawStr(28, 62, "BLE Daniel-S");
      u8g2.sendBuffer();
    }
  }
}
