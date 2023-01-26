
#define bcd2dec(bcd_in) (bcd_in >> 4) * 10 + (bcd_in & 0x0f)
#define dec2bcd(dec_in) ((dec_in / 10) << 4) + (dec_in % 10)

#define SCROLL_RIGHT 0
#define SCROLL_LEFT 1

//for RGB fade animation
float RGB_A[3];
float RGB_B[3];

#define PWM_RESOLUTION 8
#define PWM_FREQUENCY 5000
#define PWM_MAX pow(2, PWM_RESOLUTION)-1

bool nixieTubesOn = false;
byte ledsBrightness = 0;

byte mByte[0x07]; // holds the array from the DS3231 register
byte tByte[0x07]; // holds the array from the NTP server

//Current register states 
#define TOTALPOINTS 42
boolean nixieDisplayArray[TOTALPOINTS];

//Out-of-bound index to switch off the tube
#define OOBIDX 40

//Connection matrix between shift registers and nixie tubes
//               0   1   2   3   4   5   6   7   8   9  off
byte nixie1[]={ 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, OOBIDX };
byte nixie2[]={ 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, OOBIDX };
byte nixie3[]={ 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, OOBIDX };
byte nixie4[]={  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, OOBIDX };    

//Order of nixie tube inner layers 
byte fadeArr[] = {10, 10, 1, 6, 2, 7, 5, 0, 4, 9, 8, 3, 10, 10};


void NixieDisplay(byte digit1, byte digit2, byte digit3, byte digit4);
void ShiftOutData();


void delayt(unsigned long ms)
{
  //timer.tick(); segfault
  delay(ms);
}

void rgbFadeInit()
{
  memset(RGB_A, 0, 3); 
  memset(RGB_B, 0, 3); 
}

#define FADEOFFSET 30
#define FADESTEPS 100

void rgbFade()
{
  //Choice not dark color
  while((RGB_B[0] + RGB_B[1] + RGB_B[2] < FADEOFFSET) || 
    ((fabs(RGB_A[0] - RGB_B[0]) < 1) && (fabs(RGB_A[1] - RGB_B[1]) < 1) && (fabs(RGB_A[2] - RGB_B[2]) < 1))
  ){
    RGB_B[0] = random(PWM_MAX);
    RGB_B[1] = random(PWM_MAX);
    RGB_B[2] = random(PWM_MAX);
  }
    
  RGB_A[0] -= (RGB_A[0] - RGB_B[0]) / FADESTEPS;
  RGB_A[1] -= (RGB_A[1] - RGB_B[1]) / FADESTEPS;
  RGB_A[2] -= (RGB_A[2] - RGB_B[2]) / FADESTEPS;

  ledcWrite(1, RGB_A[0]);
  ledcWrite(2, RGB_A[1]);
  ledcWrite(3, RGB_A[2]);
}

void NixieRandom()
{
  for(int i=2; i<14; i++){
    NixieDisplay(random(10), random(10), random(10), random(10));
    delayt(80);
  }  
}

void NixieScroll(bool toleft)
{
  if(toleft){
    for(int i=0; i<14; i++){
      NixieDisplay(i-4, i-3, i-2, i-1);
      delayt(80);
    }
  }else{
    for(int i=14; i>0; i--){
      NixieDisplay(i-4, i-3, i-2, i-1);
      delayt(80);
    }
  }
}

void NixieFadeIn()
{
  memset(nixieDisplayArray, 0, TOTALPOINTS); 

  //Turn on couple of segments simultaneously
  for(int i=2; i<14; i++){
    nixieDisplayArray[nixie1[fadeArr[i-2]]] = 0;
    nixieDisplayArray[nixie2[fadeArr[i-2]]] = 0;
    nixieDisplayArray[nixie3[fadeArr[i-2]]] = 0;
    nixieDisplayArray[nixie4[fadeArr[i-2]]] = 0;

    nixieDisplayArray[nixie1[fadeArr[i-1]]] = 1;
    nixieDisplayArray[nixie2[fadeArr[i-1]]] = 1;
    nixieDisplayArray[nixie3[fadeArr[i-1]]] = 1;
    nixieDisplayArray[nixie4[fadeArr[i-1]]] = 1;

    nixieDisplayArray[nixie1[fadeArr[i]]] = 1;
    nixieDisplayArray[nixie2[fadeArr[i]]] = 1;
    nixieDisplayArray[nixie3[fadeArr[i]]] = 1;
    nixieDisplayArray[nixie4[fadeArr[i]]] = 1;
    
    ShiftOutData();
    delayt(80);
  }
}

void NixieSleep()
{
  nixieTubesOn = false;
  ShiftOutData();
}

void NixieWakeup()
{
  nixieTubesOn = true;
  ShiftOutData();
}

void NixieDisplay(byte digit1, byte digit2, byte digit3, byte digit4)
{
  if(digit1 < 0 || digit1 > 10)digit1 = 10;
  if(digit2 < 0 || digit2 > 10)digit2 = 10;
  if(digit3 < 0 || digit3 > 10)digit3 = 10;
  if(digit4 < 0 || digit4 > 10)digit4 = 10;
  
  memset(nixieDisplayArray, 0, TOTALPOINTS); 
  nixieDisplayArray[nixie1[digit1]] = 1;
  nixieDisplayArray[nixie2[digit2]] = 1;
  nixieDisplayArray[nixie3[digit3]] = 1;
  nixieDisplayArray[nixie4[digit4]] = 1;
  
  ShiftOutData();
}

void ShiftOutData()
{
  // Ground EN pin and hold low for as long as you are transmitting
  digitalWrite(REG_LATCH, 0);

  // Clear everything out just in case to
  // prepare shift register for bit shifting
  digitalWrite(REG_DATA, 0);
  digitalWrite(REG_SHIFT, 0);

  // Send data to the nixie drivers 
  for (int i = TOTALPOINTS-1; i >= 0; i--)
  {    
    // Set high only the bit that corresponds to the current nixie digit
    if(nixieTubesOn){
      digitalWrite(REG_DATA, nixieDisplayArray[i]);
    }else{
      digitalWrite(REG_DATA, 0);
    }
    
    // Register shifts bits on upstroke of CLK pin 
    digitalWrite(REG_SHIFT, 1);
    // Set low the data pin after shift to prevent bleed through
    digitalWrite(REG_SHIFT, 0);
  }   

  // Return the EN pin high to signal chip that it 
  // no longer needs to listen for data
  digitalWrite(REG_LATCH, 1);
}

void rgbLed(byte R, byte G, byte B)
{
  ledcWrite(1, (R/100*ledsBrightness));
  ledcWrite(2, (G/100*ledsBrightness));
  ledcWrite(3, (B/100*ledsBrightness));
}

void updateRTC()
{
  if(!timeClient.forceUpdate()){
    Serial.println("NTP server unavailable");
    //TODO: fetch NTP server from DHCP, try that instead

    timer.in(30000, [](void*) -> bool { 
      updateRTC();
      return false; 
    });
    
    return;
  }
  
  unsigned long epochTime = timeClient.getEpochTime();
  tByte[0] = (int)timeClient.getSeconds();
  tByte[1] = (int)timeClient.getMinutes();
  tByte[2] = (int)timeClient.getHours();
  tByte[3] = (int)timeClient.getDay();

  struct tm *ptm = gmtime ((time_t *)&epochTime);
  tByte[4] = (int)ptm->tm_mday;
  tByte[5] = (int)ptm->tm_mon+1;
  tByte[6] = (int)ptm->tm_year-100;

  Serial.println(timeClient.getFormattedTime());

  if(mByte != tByte){
    Wire.beginTransmission(DS3231_ADDRESS);
    // Set device to start read reg 0
    Wire.write(0x00);
      for (int idx = 0; idx < 7; idx++){
        Wire.write(dec2bcd(tByte[idx]));
      }
    Wire.endTransmission();
  }
}

bool getRTCdatetime()
{
  Wire.beginTransmission(DS3231_ADDRESS);
  // Set device to start read reg 0
  Wire.write(0x00);
  Wire.endTransmission();
  
  // request 7 bytes from the DS3231 and release the I2C bus
  Wire.requestFrom(DS3231_ADDRESS, 0x07, true);
  
  int idx = 0;
  bool timeChanged = 0;

  // read the first seven bytes from the DS3231 module into the array
  while(Wire.available()) {
    byte input = Wire.read();
    if(mByte[idx] != input)
      timeChanged = 1;
    mByte[idx++] = input; 
  }

  return timeChanged;
}
