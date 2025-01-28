# Security system
Having my sister constantly break into my room, take my things and never return them, I had to find a way to stop her.  
This is where this security system I made comes in.

## Overview
This is a IoT project that is powered by a ESP32.  
You can control it from anywhere using Adafruit's IO platform (free).
Required components:
- ESP32 dev module; to control everything
- 0.96" OLED display I2C; to display info to the user
- Motion sensor; to detect motion of course
- Ultrasonic sensor (3.3V version); to see if the door is opened
- IR receiver; to read commands from a controller
- RTC module; for keeping accurate time
- DALLAS 18B20 temperature sensor; to record temperature
- Optional: SD module; to log everything for later analysis
- Optional: Power module to run the project off a battery

## Features
- Arming and disarming the system using IoT or IR remote
- Armed state utilises motion sensor and ultrasonic sensor
- Disarmed state uses only ultrasonic sensor
- Monitor temperature
- Keep track of time
- Turning off the alarm system, and display only the current time and temperature
- Muting/unmuting the buzzer to be sneaky

## Installation
1. Clone the repository into a folder:
```sh
git clone https://github.com/ivg1/security_system.git
```
2. Follow the library installation process below
3. Replace the placeholder values with your own (Wifi details, IR values and AdafruitIO API (for IoT))
4. Wire the project up (wiring diagram soon)
5. Upload the .ino file to the ESP32

## Installing libraries
Everything below is carried out in Arduino IDE
Firstly, set up the ESP32 core by going to:  
```sh
Boards manager -> search "ESP32" -> download the one "by espressif"
```
Then, go to:
```sh
File -> preferences -> find "Additional boards manager URLs" -> then paste "https://dl.espressif.com/dl/package_esp32_index.json"
```
After you've done that, go to Library manager, search and download (exact names):
1. Adafruit IO Arduino
2. Adafruit SSD1306
3. Adafruit GFX Library (if not prompted form SSD1306 library)
4. RTClib
5. OneWire
6. DallasTemperature
7. SD
Everything else that hasnt been listed is already included when doing the boards manager step

## Usage
To be able to use this project with all its features and minimum problems, you need to replace all the placeholders in the .ino file

Then just plug the ESP32 to power and be secure