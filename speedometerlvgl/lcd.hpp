
#ifndef LCD_HPP
#define LCD_HPP

#include <stdint.h>
#include "lvgl.h" // Include your graphics framework engine header reference
#include "hardware/spi.h"
#include "LCD_Fonts.hpp"

class LcdDisplay {
private:
    uint16_t width;
    uint16_t height;

public:
    LcdDisplay(uint16_t screen_width = 320, uint16_t screen_height = 240);
    
    void Inithw();
    void Init();
    void HWReset();
    char SetWindow(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye);
    void ClearScreen(uint16_t color, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

    // 🌟 ADD THIS COMPILATION WRAPPER FOR LVGL INTEGRATION
    void FlushLvglDisplay(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
};

extern LcdDisplay lcd;

#endif
