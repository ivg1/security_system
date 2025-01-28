#include <Wire.h>
#include <Adafruit_GFX.h> 
#include <Adafruit_SSD1306.h>
#include <IRremote.h>
#include <RTClib.h>
#include <WiFi.h>
#include "AdafruitIO_WiFi.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SD.h>
#include <SPI.h>

//oled screen
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

//find it from adafruit io feed
#define IO_USERNAME  "{your username}"
#define IO_KEY       "{your API key}"

#define wifi_ssid "{your wifi ssid}"
#define wifi_pass "{your wifi password}"

#define sys_on_ir {assign your own code}
#define sys_off_ir {assign your own code}
#define arm_on_ir {assign your own code}
#define arm_off_ir {assign your own code}
#define force_close_door_ir {assign your own code}
#define buzz_on_ir {assign your own code}
#define buzz_off_ir {assign your own code}

const int motion_pin = 15, ir_receive_pin = 4, proximity_trig = 17, proximity_echo = 16, buzz_pin = 27, temp_pin = 26, sd_cs_pin = 5;
unsigned long ir_data = 0;
bool device_on = false;
bool armed = false;
bool prev_detected_mov = false;
bool door_state = false;
bool buzz_state = true;

unsigned long prev_time = 0;
const unsigned long interval_time = 10000; //every 10sec to upload the temp val to feed, my data rate is 30/min
float temperature = 0;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);
RTC_DS3231 rtc;

AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, wifi_ssid, wifi_pass);
AdafruitIO_Feed *alarm_feed = io.feed("alarm");
AdafruitIO_Feed *temp_feed = io.feed("temp");
AdafruitIO_Feed *door_feed = io.feed("door");

OneWire oneWire(temp_pin); 
DallasTemperature sensors(&oneWire);

File log_file;

void control_alarm(AdafruitIO_Data *data) {
  Serial.print("Received from alarm feed: ");
  String val = data->value();
  Serial.println(val);

  if (val == "arm-on" && !armed) {
    armed = true;
    if (buzz_state) tone(buzz_pin, 1200, 100);
  } else if (val == "arm-off") {
    armed = false;
    if (buzz_state) tone(buzz_pin, 1300, 100);
  }

  if (val == "sys-on" && !device_on) {
    device_on = true;
    if (buzz_state) tone(buzz_pin, 1000, 100);
    delay(200);
    if (buzz_state) tone(buzz_pin, 1000, 100);
  } else if (val == "sys-off") {
    device_on = false;
    if (buzz_state) tone(buzz_pin, 1500, 100);
  }

  if (val == "sound-off" && buzz_state) {
    buzz_state = false;
    tone(buzz_pin, 2000, 50);
  } else if (val == "sound-on" && !buzz_state) {
    buzz_state = true;
    tone(buzz_pin, 2500, 50);
  }
}

void change_door_state(AdafruitIO_Data *data) {
  Serial.print("Received from door feed: ");
  String val = data->value();
  Serial.println(val);
  if (val == "force-shut-door" && door_state) {
    door_state = false;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(motion_pin, INPUT);
  pinMode(ir_receive_pin, INPUT);
  pinMode(proximity_trig, OUTPUT);
  pinMode(proximity_echo, INPUT);
  pinMode(buzz_pin, OUTPUT);
  pinMode(temp_pin, INPUT);

  //init oled display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 init failed"));
    while (1);
  }

  //init rtc module
  if (!rtc.begin()) {
    Serial.println("Didnt find RTC");
    while (1);
  }
  //if i removed battery for no reason as to sync it back, it takes the time i compiled it on
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    Serial.println("RTC time set");
  }

  //init ir receiver
  IrReceiver.begin(ir_receive_pin, ENABLE_LED_FEEDBACK);

  //wifi for iot
  Serial.print("Connecting to WiFi... ");
  WiFi.begin(wifi_ssid, wifi_pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println("connected.");

  //connect to ada io for iot
  Serial.print("Connecting to Adafruit IO... ");
  io.connect();
  alarm_feed->onMessage(control_alarm);
  door_feed->onMessage(change_door_state);
  while (io.status() < AIO_CONNECTED) {
    delay(500);
  } 
  Serial.println("connected.");

  //sd init
  if (!SD.begin(sd_cs_pin)) {
    Serial.println("SD init failed.");
  } else {
    Serial.println("SD init successfully.");
  }

  //init temp sensor
  sensors.begin();

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  sensors.requestTemperatures();
  temperature = sensors.getTempCByIndex(0);
}

void loop() {
  unsigned long curr_time = millis();
  io.run();
  display.clearDisplay();
  display.setCursor(0, 0);

  //read command from remote control
  if (IrReceiver.decode()) {
    ir_data = IrReceiver.decodedIRData.decodedRawData;

    if (ir_data == sys_on_ir && !device_on) {
      if (buzz_state) tone(buzz_pin, 1000, 100);
      delay(200);
      if (buzz_state) tone(buzz_pin, 1000, 100);
      device_on = true;
    } else if (ir_data == sys_off_ir && device_on) {
      if (buzz_state) tone(buzz_pin, 1500, 100);
      device_on = false;
      armed = false;
    }

    if (ir_data == arm_on_ir && device_on && !armed) {
      armed = true;
      if (buzz_state) tone(buzz_pin, 1200, 100);
    } else if (ir_data == arm_off_ir && device_on && armed) {
        armed = false;
        if (buzz_state) tone(buzz_pin, 1300, 100);
    }

    if (ir_data == force_close_door_ir && door_state) {
      door_state = false;
      if (buzz_state) tone(buzz_pin, 1800, 100);
    }

    //buzz on/off
    if (ir_data == buzz_off_ir && buzz_state) {
      buzz_state = false;
     tone(buzz_pin, 2000, 50);
    } else if (ir_data == buzz_on_ir && !buzz_state){
      buzz_state = true;
      tone(buzz_pin, 2500, 50);
    }

    IrReceiver.resume();
  }

  //if the thing is said to be on
  if (device_on) {
    display.setCursor(0, 0);
    display.print("System ON");
    
    //display curr time
    DateTime time = rtc.now();
    String timestamp = String(time.year()) + "-" + String(time.month()) + "-" + String(time.day()) + " " + String(time.hour()) + ":" + String(time.minute()) + ":" + String(time.second());
    display.setCursor(SCREEN_WIDTH - 32, 0);
    display.print(time.hour());
    display.print(":");
    display.print(time.minute());

    display.setCursor(0, 16);

    if (armed) {
      display.setTextSize(2);
      //little joke
      if (buzz_state) {
        display.println("ARMED");
        display.setTextSize(1);
      } else {
        display.setTextSize(1);
        display.println("Silent but snitching");
      }

      if (digitalRead(motion_pin) == HIGH) {
        display.println("Motion:  Detected");
        if (!prev_detected_mov) {
          if (buzz_state) tone(buzz_pin, 4000, 5000);

          alarm_feed->save("motion");
          
          String log = "{\"timestamp\":\"" + timestamp + "\",\"event\":\"motion detected\"}";
          log_file = SD.open("log.json", FILE_WRITE);
          if (log_file) {
            log_file.println(log);
            log_file.close();
            Serial.println("Logged motion to sd.");
          }
          prev_detected_mov = true;
        }
      } else {
        display.println("Motion:  None");
        prev_detected_mov = false;
      }
    } else {
      display.setTextSize(2);
      display.println("DISARMED");
      display.setTextSize(1);
    }

    digitalWrite(proximity_trig, LOW);
    delayMicroseconds(10);
    digitalWrite(proximity_trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(proximity_trig, LOW);

    long duration = pulseIn(proximity_echo, HIGH);
    float distance = (duration / 2.0) * 0.0343;
    display.print("Distance: ");
    display.print(distance);
    display.println(" cm");

    if (distance >= 170 && !door_state) {
      door_state = true;
      if (buzz_state) tone(buzz_pin, 1700, 5000);
      door_feed->save("door-opened");
      String log = "{\"timestamp\":\"" + timestamp + "\",\"event\":\"door opened\"}";
      log_file = SD.open("log.json", FILE_WRITE);
      if (log_file) {
        log_file.println(log);
        log_file.close();
        Serial.println("Logged door opened to sd.");
      }
    }

    //get temp value every 10sec
    if (curr_time - prev_time >= interval_time) {
      prev_time = curr_time;
      sensors.requestTemperatures();
      temperature = sensors.getTempCByIndex(0);
      temp_feed->save(temperature);

      String log = "{\"timestamp\":\"" + timestamp + "\",\"temperature\":\"" + temperature + "\"}";
      log_file = SD.open("log.json", FILE_WRITE);
      if (log_file) {
        log_file.println(log);
        log_file.close();
        Serial.println("Logged temperature to sd.");
      }
    }
    display.print("Temp: ");
    display.print(temperature);
    display.println(" C");

    display.setCursor(0, SCREEN_HEIGHT - 7);
    display.print("IR Code: ");
    display.println(ir_data, HEX);

  } else {
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.print("System OFF");

    //display curr time
    DateTime time = rtc.now();
    display.setCursor(SCREEN_WIDTH - 32, 0);
    display.print(time.hour());
    display.print(":");
    display.print(time.minute());

    display.setCursor(0, 16);
    
    if (curr_time - prev_time >= interval_time) {
      prev_time = curr_time;
      sensors.requestTemperatures();
      temperature = sensors.getTempCByIndex(0);
      temp_feed->save(temperature);
    }
    display.println("Temp:");
    display.println(" ");
    display.setTextSize(2);
    display.print(temperature);
    display.println(" C");
    display.setTextSize(1);
  }

  display.display();

  delay(1);
}
