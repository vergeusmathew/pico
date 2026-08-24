#include "ds3231.hpp"
#include "lcd.hpp"
#include "LCD_Fonts.hpp"
#include <stdio.h>

void ShowDigitalClock(void)
{
    char large_clock[16]; 
    int display_hour;
    static int last_display_hour = -1; 

    // 1. Fetch a fresh, fully decoded time snapshot object from your C++ RTC class
    RtcTime t = rtc.GetTime(); 

    // 2. Compute your working 12-hour format structure logic
    display_hour = t.hour % 12;
    if (display_hour == 0) display_hour = 12; 
    
    // 3. Toggle your blinking colon separator based on real seconds
    char separator = (t.sec % 2 == 0) ? ':' : ' ';

    // 4. Render and clear character slots based on layout dimensions using your new 16x26 font
    if (display_hour < 10) {
        sprintf(large_clock, "%d%c%02d", display_hour, separator, t.min);
        
        if (last_display_hour >= 10 || last_display_hour == -1) {
            // Sweep away leftover pixel artifacts on rollover transitions
            lcd.ClearScreen(ILI9341_COLOR_BLACK,0,0,320,240);
        }
        lcd.Puts_Scaled(64, 81, large_clock, &LCD_Font_16x26, ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK, 3);
    } 
    else {
        sprintf(large_clock, "%d%c%02d", display_hour, separator, t.min);
        lcd.Puts_Scaled(40, 81, large_clock, &LCD_Font_16x26, ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK, 3);
    }

    last_display_hour = display_hour;
    
    char small_time_str[12];
    char small_date_str[12];

    // Format strings to match standard 2-digit patterns with zero-padding
    sprintf(small_time_str, "%02d:%02d:%02d", t.hour, t.min, t.sec);
    sprintf(small_date_str, "%02d/%02d/%02d", t.day, t.month, (t.year % 100));

    // Render the Date line directly above the time line
    // Font: &LCD_Font_7x10, Scale: 1, Colors: White text on Black background
    lcd.Puts_Scaled(259, 225, small_date_str, &LCD_Font_7x10, ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK, 1);

    // Render the Time line along the absolute bottom margin edge
    lcd.Puts_Scaled(259, 213, small_time_str, &LCD_Font_7x10, ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK, 1);
}
