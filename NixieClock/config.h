const char* ssid     = "HomeWiFi";
const char* password = "1234567890";

//const char* ap_ssid     = "NixieClock";
//const char* ap_password = "1234567890";

//Inputs GPIO
#define KEY_UP 10
#define KEY_DOWN 12
#define MOTION_SENSOR 21

//RGB backlight leds 
#define BL_R 35
#define BL_G 34
#define BL_B 36

//Nixie shift registers 
#define REG_SHIFT 16
#define REG_OUTPUT 17
#define REG_LATCH 18
#define REG_DATA 33

//DS3231 clock connected via i2c
#define RTC_SDA 37
#define RTC_SCL 38
#define DS3231_ADDRESS 0x68

//GMT+05
#define TIMEZONE_OFFSET 5 * 60*60

//Neon DOT leds
#define NIX_DOT 40
#define DOTS_INTERVAL 500

//disable tubes and backlight after 30sec (100*300)
#define motionCheckPer 100
#define motionSensorLagLimit 300

//decay in 1sec (100 steps of brightness in 10ms resolution) 
#define backlightStepPer 10
