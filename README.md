# Idea

Original Nixie Clock KIT board contain **STM8S** (STM8S003F3) MCU, **74HC595** (SNx4HC595) shift registers, **ULN2003** darlington drivers, **DS3231** RTC with battery and plexiglass front-back covers. Also there are two buttons for setting the time and change some settings, which are resets after power cycle. 

First thought to keep an MCU and add external GPS/WiFi module to be able to sync time. But then I thought that I could simplify the task a little and completely replace MCU with an ESP32. S2 mini board perfectly fit for size and mounting holes. So a soldered out STM8 MCU and connect ESP32 board using lacquered copper wires.

# In-the-box picture

![nixie-stm8-pinout](https://github.com/darkbyte-ru/Nixie-Clock-esp32/blob/main/Image/nixie-clock-in12-dark.jpg?raw=true)

# Hardware

- Nixie Clock IN12 DIY KIT (NixieClock_IN12 v4.2 board from [aliexpress](https://aliexpress.ru/wholesale?SearchText=nixie+clock+in12))
- IN-12 tubes (4pcs) with digits
- 3D printed case (I used [this one](https://www.printables.com/model/69286-in-12-nixie-clock-by-delucalabs))
- WeMos/Lolin S2 MINI ESP32 board
- RCWL-0516 microwave motion sensor

# Assembly

After researching original connection scheme, prepared next pinout: 

![nixie-stm8-pinout](https://github.com/darkbyte-ru/Nixie-Clock-esp32/blob/main/Image/nixie-stm8-connection.png?raw=true)

I connected the most inputs to the one side of esp32 board and only KEYs wires goes to GPIO10 & 12. Used by me GPIOs can be found in the config.h file.

For test purpose I also connected 5V bus from ESP32 with 5V input of nixie board. Without that tube did not powered up when the only esp board powered.

To extend lamp life and save some electricity there is also **RCWL-0516** microwave motion sensor added (powered from 5V bus). By default it switch-off the tubes after 30 seconds of inactivity.

By default **DOTs** driven via separate transistor, but HC595 registers has left couple unused outputs, so thats possible to connect them there. There is also only one HC595 have connected **Output Enable** pin to MCU, on all other registers that pin just grounded.

Insides looks a bit [ugly](https://github.com/darkbyte-ru/Nixie-Clock-esp32/blob/main/Image/heart-transplant.jpg), but who will see it? 🤷

# Software

To nicely arrange worker threads I used an [arduino-timer](https://github.com/contrem/arduino-timer) library. Also used [NTPClient](https://github.com/arduino-libraries/NTPClient) and [ArduinoOTA](https://github.com/espressif/arduino-esp32/tree/master/libraries/ArduinoOTA).

Currently Wi-Fi connection required to startup (but having RTC onboard with backup battery allows to skip this step in feature), so make sure to supply correct Wi-Fi creds in the config.h. While connecting backlight color will be **RED** and timer countdown from 10. If no connection happens - esp reboots. 

After connecting to wireless network clocks will try to sync time with default NTP server (pool.ntp.org). While that backlight color changes to **BLUE**. If NTP server accessable - new time writes to RTC. If timeout occur - this step skipped for now and will be retried after 30 seconds.

Regardless of the previous step result clock will request current time from RTC every minute and output that via nixie tubes. To prevent nixie poisoning before each update there will be scroll through all nixie segments.

# TODO

- Some functional logic for buttons (backlight color change?)
- Failback access point to be able so setup WiFI creds
- Make Wi-Fi connection optional and in the background if RTC running
- Use NTP server from DHCP advertisement instead of default if failed
