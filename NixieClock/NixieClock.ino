#include <Wire.h>
#include <WiFi.h>

#include <NTPClient.h>
#include <WiFiUdp.h>

#include <ESPmDNS.h>
#include <ArduinoOTA.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

#include <arduino-timer.h> 
auto timer = timer_create_default();

byte keyUpPressed = 0;
byte keyDownPressed = 0;
byte motionSensorTriggered = 0;
unsigned int motionSensorLag = 0;

#include "config.h"
#include "helpers.h"
#include "workers.h"

void IRAM_ATTR ISR_KEY_UP() {
  keyUpPressed++;
}

void IRAM_ATTR ISR_KEY_DOWN() {
  keyDownPressed++;
}

void setup() 
{
  Serial.begin(115200);
  Serial.println("setup()");
  randomSeed(analogRead(0));

  pinMode(NIX_DOT, OUTPUT);
  nixieTubesOn = true;

  //RGB led attach
  ledcAttachPin(BL_R, 1);
  ledcAttachPin(BL_G, 2);
  ledcAttachPin(BL_B, 3);

  ledcSetup(1, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcSetup(2, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcSetup(3, PWM_FREQUENCY, PWM_RESOLUTION);

  ledsBrightness = 99;
  rgbLed(PWM_MAX, 0, 0); //RED backlight indicate initial step

  //Nixie registers
  pinMode(REG_OUTPUT, OUTPUT);//only one register connected
  pinMode(REG_DATA, OUTPUT); 
  pinMode(REG_SHIFT, OUTPUT);
  pinMode(REG_LATCH, OUTPUT);

  //Keys and motion inputs 
  pinMode(KEY_UP, INPUT_PULLUP);
  pinMode(KEY_DOWN, INPUT_PULLUP);
  pinMode(MOTION_SENSOR, INPUT_PULLDOWN);
  
  attachInterrupt(KEY_UP, ISR_KEY_UP, FALLING);
  attachInterrupt(KEY_DOWN, ISR_KEY_DOWN, FALLING);
  
  //DS3231 RTC via i2c
  Wire.begin(RTC_SDA, RTC_SCL);

  Serial.println("WiFi.begin()");
  WiFi.begin(ssid, password);

  //TODO: if RTC running - skip wifi dependency and delay NTP update
  int countdown = 10;
  while (WiFi.status() != WL_CONNECTED) {
    NixieDisplay(countdown, countdown, countdown, countdown);
    if(!countdown--){
      Serial.println("No wifi, reboots now");
      //TODO: failback AP
      ESP.restart();
    }
    delay(1000);
    Serial.print(".");
  }
  rgbLed(0, PWM_MAX, 0); //GREEN backlight - WiFi connected

  //10 disables all nixie outputs
  NixieDisplay(10, 10, 10, 10);

  Serial.println("ArduinoOTA");
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
      NixieDisplay(10, 10, 10, 10);
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      byte percent = (progress / (total / 100));
      Serial.printf("Progress: %u%%\r", percent);
      NixieDisplay(10, 10, percent/10%10, percent%10);
      digitalWrite(NIX_DOT, !digitalRead(NIX_DOT));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      switch(error){
        case OTA_AUTH_ERROR: 
          Serial.println("Auth Failed");
          break;
        case OTA_BEGIN_ERROR:
          Serial.println("Begin Failed");
          break;
        case OTA_CONNECT_ERROR:
          Serial.println("Connect Failed");
          break;
        case OTA_RECEIVE_ERROR:
          Serial.println("Receive Failed");
          break;
        case OTA_END_ERROR:
          Serial.println("End Failed");
          break;
      }
      rgbLed(PWM_MAX, 0, 0);
    });

  ArduinoOTA.begin();

  rgbLed(0, 0, PWM_MAX); //BLUE backlight - init almost done

  //Inits NTP client and update RTC counters via that
  Serial.println("timeClient.begin()");
  timeClient.begin();
  timeClient.setTimeOffset(TIMEZONE_OFFSET);
  updateRTC();

  timer.every(backlightStepPer, backlightLeds);
  timer.every(motionCheckPer, motionCheck);
  timer.in(250, clockTick);

  timer.every(DOTS_INTERVAL, [](void*) -> bool { 
    digitalWrite(NIX_DOT, !digitalRead(NIX_DOT));
    return true; 
  });
  
/*
 * backlight random color change
  rgbFadeInit();
  timer.every(50, [](void*) -> bool { 
    rgbFade();
    return true; 
  });
*/

}

void loop() 
{
  //buttons handlers
  //TODO: change backlight color? 
  if(keyUpPressed){
    Serial.print("Key UP pressed ");
    Serial.println(keyUpPressed);
    keyUpPressed = 0;
  }

  if(keyDownPressed){
    Serial.print("Key DOWN pressed ");
    Serial.println(keyDownPressed);
    keyDownPressed = 0;
  }

  timer.tick(); 
  ArduinoOTA.handle();
}
