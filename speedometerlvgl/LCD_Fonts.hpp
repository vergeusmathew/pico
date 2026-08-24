
#ifndef LCD_FONTS_HPP
#define LCD_FONTS_HPP

#include <stdint.h>

struct LCD_FontDef_t {
    uint8_t FontWidth;
    uint8_t FontHeight;
    const uint16_t *data;
};


// Declare the external font objects so other files can see them
extern const LCD_FontDef_t LCD_Font_7x10;
extern const LCD_FontDef_t LCD_Font_8x12;
extern const LCD_FontDef_t LCD_Font_11x18;
extern const LCD_FontDef_t LCD_Font_16x26;

#endif
