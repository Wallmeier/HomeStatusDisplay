# HomeStatusDisplay with or without Clock

Show status information sent via MQTT (e.g. from ioBroker or any other MQTT-speaking system) using RGB LEDs.
The code is also able to show the time on a 4-Digit Display (e.g. Grove) and synchronizes via NTP service.

![Status display in action](../assets/HSD_Clock.jpg)

## Requirements
### Hardware
This code was tested on a Wemos D1 mini ESP8266 board. It *should* run on any ESP8266 board. You need a number of WS2812B LEDs (e.g. a NeoPixel stripe) connected to `5V`, `GND` and a `DATA` pin of the ESP. Please notice the wiring tips from Adafruit- use a large capacitor (around 1000uF) between the `5V` and `GND` of the LEDs and put a 300-500 Ohm resistor into the `DATA` line connection to the LEDs. Here is a overview of the wiring - it is the standard wiring recommended for WS2812B stripes. The board can be powered via a the 5V power supply via the USB port. Of course it should be powerful enough to drive all the LEDs used:

![Circuit diagram](../assets/Schaltplan_HSD_Clock.png)

### Software
The code was developed using the Arduino IDE and the ESP8266 library. You need to install the following additional libraries to compile the code:
 - Adafruit_NeoPixel: https://github.com/adafruit/Adafruit_NeoPixel
 - PubSubClient: https://github.com/knolleary/pubsubclient
 - ArduinoJson (currently working version is 5.xx): https://github.com/bblanchon/ArduinoJson
 - TM1637: https://github.com/Seeed-Studio/Grove_4Digital_Display
 - ezTime: https://github.com/ropg/ezTime
 
All libraries can be installed using the library manager of the Arduino IDE.

You also need a running MQTT broker (e.g. ioBroker, or https://mosquitto.org), to which the system you want to monitor pushes its status information. For example, in ioBroker the status information which you want to display, can be configured using the ioBroker modules `MQTT Broker/Client` and `MQTT client`.

## How to use
Upon first usage, the system will create an access point. Connect to this access point with any WiFi-capable device and open the page 192.168.4.1.
There you will find a configuration page in which you have to enter your settings:
 - WLAN credentials
 - MQTT setup
 - LED setup
 - (Optional:) Clock setup

### Device mapping
In this section you define which MQTT message responds to which LED number.

### Color mapping
You can leave it as is, but you can edit or add new colors to the configuration.

### The HomeStatusDisplay logic
The StatusDisplay subscribes to the MQTT broker (e.g. ioBroker) and checks incoming messages with help of the device mapping which LED should have which color (with help of the color mapping).
 
Thanks to Bernd Schubart who developed the code base for this project: https://github.com/MTJoker/HomeStatusDisplay
