
bool clockTick(void *)
{
  //every minute update nixie readings according to RTC
  getRTCdatetime();
  
  //why? why not? dunno
  if(millis() > 86400000L){
    Serial.println("Daily reboot");
    ESP.restart();
  }

  int second = bcd2dec(mByte[0]);

  //align next clock update to minute
  timer.in(60000 - second*1000, clockTick);

  //funny poisoning protection
  switch(random(3)){
    case 0:
      NixieScroll(SCROLL_LEFT);
      break;
    case 1:
      NixieScroll(SCROLL_RIGHT);
      break;
    case 2:
      NixieFadeIn();
      break;
  }

  int hour = bcd2dec(mByte[2]);
  int minute = bcd2dec(mByte[1]);
  NixieDisplay(hour/10%10, hour%10, minute/10%10, minute%10);

  return false;
}

bool backlightLeds(void *)
{
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
  return true;
}

bool motionCheck(void *)
{
  //delay for motion trigger
  if(digitalRead(MOTION_SENSOR)){
    motionSensorLag = 0;
  }else{
    if(motionSensorLag < motionSensorLagLimit)motionSensorLag++;
  }
  return true;
}
