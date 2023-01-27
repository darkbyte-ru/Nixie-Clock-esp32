
void ClockTask(void *)
{
  TickType_t xClockTick = xTaskGetTickCount();
  
  for(;;){
    //every minute update nixie readings according to RTC
    getRTCdatetime();
    
    //why? why not? dunno
    if(millis() > 86400000L){
      Serial.println("Daily reboot");
      ESP.restart();
    }
  
    int second = bcd2dec(mByte[0]);
    unsigned long lastUpdate = millis();
  
    //funny poisoning protection
    switch(random(4)){
      case 0:
        NixieScroll(SCROLL_LEFT);
        break;
      case 1:
        NixieScroll(SCROLL_RIGHT);
        break;
      case 2:
        NixieFadeIn();
        break;
      case 3:
        NixieRandom();
        break;
    }
  
    int hour = bcd2dec(mByte[2]);
    int minute = bcd2dec(mByte[1]);
    NixieDisplay(hour/10%10, hour%10, minute/10%10, minute%10);

    //align next clock update to minute with one second lag
    xTaskDelayUntil(&xClockTick, 60000 - second*1000 + 1000);
  }
}

void BacklightTask(void *)
{
  for(;;){
    //decrease backlight if no motion detected and switch off tubes at the end.
    //do backwards while motion detected
    if(motionSensorLag < motionSensorLagLimit){
      if(ledsBrightness < 100){
        if(++ledsBrightness == 1){
          NixieWakeup();
        }
        rgbLed(0x00, 0xF9, 0xFF);
      }
    }else{
      if(ledsBrightness > 0){
        if(--ledsBrightness == 0){
          NixieSleep();
        }
        rgbLed(0x00, 0xF9, 0xFF);
      }
    }
    
    vTaskDelay(backlightStepPer);
  }
}

void MotionSensTask(void *)
{
  for(;;){
    //delay for motion trigger
    if(digitalRead(MOTION_SENSOR)){
      motionSensorLag = 0;
    }else{
      if(motionSensorLag < motionSensorLagLimit)motionSensorLag++;
    }

    vTaskDelay(motionCheckPer);
  }
}

void ButtonsTask(void *)
{
  for(;;){
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

    vTaskDelay(100);
  } 
}

void DotsTask(void *)
{
  pinMode(NIX_DOT, OUTPUT);

  TickType_t xLastWakeTime = xTaskGetTickCount();
  
  for(;;){
    digitalWrite(NIX_DOT, !digitalRead(NIX_DOT));
    xTaskDelayUntil(&xLastWakeTime, 500);
  }
}

void OTATask(void *)
{
  Serial.println("ArduinoOTA");
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      //suspend all threads except OTA
      //vTaskSuspendAll();
      //vTaskResume(nullptr);
      //segfault 

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
      NixieDisplay((100-percent)/10%10, (100-percent)%10, percent/10%10, percent%10);
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
    
  for(;;){
    ArduinoOTA.handle();
    vTaskDelay(100);
  }
}
