#include <Wire.h>
#include <WiFi.h>

#include <NTPClient.h>
#include <WiFiUdp.h>

#include <ESPmDNS.h>
#include <ArduinoOTA.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

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

  xTaskCreate(DotsTask, "DotsTask", 10000, NULL, 1, &hDotsTask);
  
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
  pinMode(REG_OUTPUT, OUTPUT);//only one register connected there
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

  xTaskCreate(OTATask, "OTATask", 10000, NULL, 5, &hOTATask);

  rgbLed(0, 0, PWM_MAX); //BLUE backlight - init almost done

  //Inits NTP client and update RTC counters via that
  Serial.println("timeClient.begin()");
  timeClient.begin();
  timeClient.setTimeOffset(TIMEZONE_OFFSET);
  updateRTC();

  xTaskCreate(ButtonsTask, "ButtonsTask", 10000, NULL, 100, &hButtonsTask);
  xTaskCreate(ClockTask, "ClockTask", 10000, NULL, 10, &hClockTask);

  xTaskCreate(MotionSensTask, "MotionSensTask", 10000, NULL, 100, &hMotionSensTask);
  xTaskCreate(BacklightTask, "BacklightTask", 10000, NULL, 100, &hBacklightTask);

/*
  xTaskCreate([](void*) -> void { 
    for(;;){
      rgbFade();
      vTaskDelay(30);
    }
  }, "BacklightColor", 10000, NULL, 100, &hBacklightColorTask);
*/
  
}

void loop() 
{
  //delete built-in task 
  vTaskDelete(nullptr);
}
