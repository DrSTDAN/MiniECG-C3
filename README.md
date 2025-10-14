# MiniECG-C3 12bit D
********************************************************************************************************************************************************************************
Caution!!!!
IMPORTANT PLEASE READ BEFORE, IS VERY IMPORTANT :
Disclaimer: This is not a medical device; it is only for test purposes and personal, school education!
Never intent to use it in a hospital or for diagnostic purposes!
Consult the Law in your Country or region to see if thie device is legal to build, use.... We are not responsable for misuse or for illegal use of this device.
This device is not input protected for defibrilators discharge!!!!!
This device is not build to run with Ac-Dc power adapters or transformers it can be used with battery only maximum voltage 4.2!!
If you have any inplanted device like Pacemakers, implantable cardioverter defibrillator (ICD), implantable bombs, or byonic organs. you cannot use or touch this leads of this device!!!!!
Do not use it inside watter, pools, jacuzzi, with rain, inside ocean or sea or lakes!

*******************************************************************************************************************************************************************************




English:
A small ECG with AD8232 ESP32 C3 oled display hardware and Arduino software with Android application
AD8232 module is a very cheap heart monitor module(5..10USD).
ESP32 C3 mini with oled is very popular MCU board with WIFI, Bluetooth and Oled Display integrated (about 2USD).  

Espanol:
Un pequeno ECG con AD8232 ESP32 C3 con pantalla oled el dispositivo con el software en Arduino IDE y su aplicacion para telefono Android apk.
El modulo AD8232 es un amplificador de signal biologico especialmente monitoreo cardiaco muy barato 5..10USD.
ESP32 C3 mini con pantalla oled es un Mcu con Bluetooth y wifi integrado aoarte tiene su micro pantalla con tehnologia oled comunica I2c cuesta 2USD o meno o mas dependiendo de el pais costos importacion....

Russian:
Компактный ЭКГ с AD8232 ESP32 C3 и OLED-дисплеем, устройство с программным обеспечением Arduino IDE и его APK-приложением для телефонов на Android.
Модуль AD8232 — это усилитель биологических сигналов, специально предназначенный для мониторинга сердца, очень недорогой — 5–10 долларов.
ESP32 C3 mini с OLED-дисплеем — это микроконтроллер со встроенными Bluetooth и Wi-Fi. У Aoarte есть свой микродисплей с OLED-технологией и интерфейсом I2C. Он стоит 2 доллара или меньше, или больше, в зависимости от страны. Стоимость импорта...

Italian:
Un piccolo ECG con AD8232 ESP32 C3 con display OLED, il dispositivo con il software Arduino IDE e la sua applicazione APK per telefoni Android.
Il modulo AD8232 è un amplificatore di segnale biologico, specifico per il monitoraggio cardiaco, molto economico, a un prezzo compreso tra 5 e 10 dollari.
L'ESP32 C3 mini con display OLED è un MCU con Bluetooth e Wi-Fi integrati. Aoarte ha il suo micro display con tecnologia OLED e comunicazione I2C. Costa 2 dollari o meno, o di più a seconda del paese. Costi di importazione...

Romanian:
Un mic ECG cu AD8232 ESP32 C3 cu afișaj OLED, dispozitivul cu software-ul Arduino IDE și aplicația sa APK pentru telefoane Android.
Modulul AD8232 este un amplificator de semnal biologic, special pentru monitorizarea cardiacă, foarte ieftin, între 5 și 10 dolari.
ESP32 C3 mini cu afișaj OLED este un MCU cu Bluetooth și Wi-Fi integrate. Aoarte are propriul său micro-afișaj cu tehnologie OLED și comunicare I2C. Costă 2 dolari sau mai puțin, sau mai mult, în funcție de țară. Costurile de import...

Polish
Niewielki EKG z AD8232 ESP32 C3 z wyświetlaczem OLED, urządzenie z oprogramowaniem Arduino IDE i aplikacją APK na telefony z systemem Android.
Moduł AD8232 to wzmacniacz sygnału biologicznego, specjalnie do monitorowania pracy serca, bardzo niedrogi, w cenie 5-10 dolarów.
ESP32 C3 mini z wyświetlaczem OLED to mikrokontroler ze zintegrowanym Bluetooth i Wi-Fi. Aoarte ma swój mikrowyświetlacz z technologią OLED i komunikacją I2C. Kosztuje 2 dolary lub mniej, lub więcej, w zależności od kraju. Koszty importu...

Chinese: 

一款集成 AD8232 和 OLED 显示屏的小型心电图仪，ESP32 C3 芯片，配备 Arduino IDE 软件和安卓手机 APK 应用程序。
AD8232 模块是一款生物信号放大器，专门用于心脏监测，价格非常实惠，仅为 5 至 10 美元。
ESP32 C3 mini 集成 OLED 显示屏，是一款集成蓝牙和 Wi-Fi 的 MCU。Aoarte 也推出了采用 OLED 技术和 I2C 通信的微型显示屏。价格在 2 美元或更低，具体价格取决于国家/地区。进口成本……

<img width="519" height="456" alt="image" src="https://github.com/user-attachments/assets/7d5e30c5-5458-4698-a19e-2044a3d16da1" />
<img width="412" height="378" alt="image" src="https://github.com/user-attachments/assets/c59bd2a5-3ead-4dd3-b95e-f8eb8d570397" />
eng:
and a Android Phone in order to monitor the ECG signal in real time. 
esp:
y un telefono o tableta android para monitorear el trazo ECG en tiempo real.

eng:
here i let you see some pictures of our apk conected with miniecg wireless with bluetooth in BLE mode (Low energy).
esp:
aqui les dejo las foto de nuestro trabajo con el apk conectado con el miniecg inalambrico con una conexion de bluetooth BLE de consumo reducido.


![ECG_principle_slow](https://github.com/user-attachments/assets/0db71d1a-0d35-41b2-8c53-511425554ce2)
<img width="1024" height="1024" alt="SinusRhythmLabels svg" src="https://github.com/user-attachments/assets/dbb845f7-c53b-4b1f-9e38-9a9fa344c937" />


![SinusalSimulator](https://github.com/user-attachments/assets/a6e74da5-6bc0-407c-958b-69473a71414d)

![Sinusal](https://github.com/user-attachments/assets/9ed07b71-3d45-4199-ab88-67931dbdd7ff)

![SigTahicardiaVentricular](https://github.com/user-attachments/assets/18369d76-4672-44be-a3df-a4dbb4aef671)

![other EV](https://github.com/user-attachments/assets/5a3317d4-b2cb-4886-a7ad-9e85e04659f2)

![HumanECGsinusalrithm](https://github.com/user-attachments/assets/a9ac6426-9e56-405e-b182-ac34c96243f4)

![ExtrasistoleVentricular](https://github.com/user-attachments/assets/c8e4c08b-0cfb-4f2f-91ef-9fc47af27337)

![EV](https://github.com/user-attachments/assets/9bb091f7-a108-4949-b2dc-5ec13ab00d7e)


Puedes ayudar este proyecto con donaciones :
You can help this project donating at:
direction de bitcoin : bc1q0sc972jn9gkn2uzde5dcp5ga6ecqpljOsctwtk
![bitcoin](https://github.com/user-attachments/assets/e79a8223-1d9d-4add-9390-15e9a12db81c)



