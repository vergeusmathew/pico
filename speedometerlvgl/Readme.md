August 28,2026 18:33

1. Home screen with 4 circular icons, speedometer, clock, stats, settings.
2. Push button top to bottom is named as Enter (Key 3), Navigate (Key 2), Esc (Key 1).
3. By default a Yellow ring appears around the speedometer icon.
4. On Pressing Navigate (Key 2), the yellow ring moves to other icon.
5. If yellow ring is on clock icon and Enter (Key 3) pressed, digital watch comes-up, replacing the Home screen.
6. If now Esc (Key 1) is pressed, Home screen shows up.
7. If yellow ring is on speedometer icon and Enter (key 3) is pressed, speedomter screen pops-up.
8. On pressing Esc, Home screeb shows up.
9. The speedometer, shows kmp on picking magnetic cuts, @ GPIO 10, the magnetic pickups is received due to
   magnet attached on tyre spokes, passing over the magnetic hall sensor 3144 
10.ESP-01 on the board connects to the android mobile hotspot and sends AT commands to receive current   
   date and time from ntp server, the recevied data and time is updated in RTC ds3231
11. The GUI is built around LVGL, and the speedometer is designed using EEZ studio, 
    ref https://www.youtube.com/watch?v=9ILaqyp_fQA.    
