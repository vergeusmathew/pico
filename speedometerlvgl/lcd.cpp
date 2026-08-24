#include "lcd.hpp"
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "lvgl.h" // Include your new graphics UI framework handle

// Define the absolute Pin Macros matching the KiCad Schematic
#define LCD_PIN_LED   9   // Schematic net: LED (Pin 11)
#define LCD_PIN_RST  14   // Schematic net: LCD_RST (Pin 19)
#define LCD_PIN_DC   15   // Schematic net: LCD_DC (Pin 20)
#define LCD_PIN_CS   17   // Schematic net: LCD_CS (Pin 22)
#define LCD_PIN_SCK  18   // Schematic net: LCD_SCK (Pin 24)
#define LCD_PIN_TX   19   // Schematic net: LCD_DI (Pin 25)

LcdDisplay lcd(320, 240); 

extern "C" {
    void cs_select(uint dc);
    void cs_deselect();
    void write_register(uint8_t data, uint8_t dc);
    void write_register16(uint32_t data, uint8_t dc);
    // FIXED: Added the pointer asterisk (*) to match your hardware assembly script parameters perfectly
	int myspi_write16_blocking(spi_inst_t *spi, const uint32_t src, size_t len);
	void myspi_write16_array_blocking(spi_inst_t *spi, const uint16_t *src, size_t len);


}

LcdDisplay::LcdDisplay(uint16_t screen_width, uint16_t screen_height) {
    width = screen_width;
    height = screen_height;
}

void LcdDisplay::Inithw() {
    spi_init(spi0, 24 * 1000 * 1000); // 24 MHz SPI clock acceleration
    gpio_set_function(LCD_PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(LCD_PIN_TX, GPIO_FUNC_SPI);
    
    gpio_init(LCD_PIN_CS); gpio_set_dir(LCD_PIN_CS, GPIO_OUT); gpio_put(LCD_PIN_CS, 1);
    gpio_init(LCD_PIN_DC); gpio_set_dir(LCD_PIN_DC, GPIO_OUT);
    gpio_init(LCD_PIN_RST); gpio_set_dir(LCD_PIN_RST, GPIO_OUT);
    
    gpio_init(LCD_PIN_LED); gpio_set_dir(LCD_PIN_LED, GPIO_OUT);
    gpio_disable_pulls(LCD_PIN_LED); gpio_put(LCD_PIN_LED, 1); // Active-Low Backlight OFF

    Init();
}

void LcdDisplay::Init() {
    HWReset(); 

    // Pushes your verified ILI9341 initialization sequence
    write_register(0xEF, 0); write_register(0x03, 1); write_register(0x80, 1); write_register(0x02, 1);
    write_register(0xCF, 0); write_register(0x00, 1); write_register(0xC1, 1); write_register(0x30, 1);
    write_register(0xED, 0); write_register(0x64, 1); write_register(0x03, 1); write_register(0x12, 1); write_register(0x81, 1);
    write_register(0xE8, 0); write_register(0x85, 1); write_register(0x00, 1); write_register(0x78, 1);
    write_register(0xCB, 0); write_register(0x39, 1); write_register(0x2C, 1); write_register(0x00, 1); write_register(0x34, 1); write_register(0x02, 1);
    write_register(0xF7, 0); write_register(0x20, 1);
    write_register(0xEA, 0); write_register(0x00, 1); write_register(0x00, 1);
    write_register(0xC0, 0); write_register(0x23, 1); // Power control 1
    write_register(0xC1, 0); write_register(0x10, 1); // Power control 2
    write_register(0xC5, 0); write_register(0x3E, 1); write_register(0x28, 1); // VCM control 1
    write_register(0xC7, 0); write_register(0x86, 1); // VCM control 2
    write_register(0x36, 0); write_register(0xE8, 1); // Memory Access Control: Horizontal Top Orientation
    write_register(0x3A, 0); write_register(0x55, 1); // Pixel format set RGB565
    write_register(0xB1, 0); write_register(0x00, 1); write_register(0x13, 1); // 100Hz Frame Rate
    write_register(0xB6, 0); write_register(0x08, 1); write_register(0x82, 1); write_register(0x27, 1);
    write_register(0xF2, 0); write_register(0x00, 1);
    write_register(0x26, 0); write_register(0x01, 1);
    
    // Positive Gamma
    write_register(0xE0, 0);    
    write_register(0x0F, 1); write_register(0x31, 1); write_register(0x2B, 1); write_register(0x0C, 1);
    write_register(0x0E, 1); write_register(0x08, 1); write_register(0x4E, 1); write_register(0xF1, 1);
    write_register(0x37, 1); write_register(0x07, 1); write_register(0x10, 1); write_register(0x03, 1);
    write_register(0x0E, 1); write_register(0x09, 1); write_register(0x00, 1);

    // Negative Gamma
    write_register(0xE1, 0);    
    write_register(0x00, 1); write_register(0x0E, 1); write_register(0x14, 1); write_register(0x03, 1);
    write_register(0x11, 1); write_register(0x07, 1); write_register(0x31, 1); write_register(0xC1, 1);
    write_register(0x48, 1); write_register(0x08, 1); write_register(0x0F, 1); write_register(0x0C, 1);
    write_register(0x31, 1); write_register(0x36, 1); write_register(0x0F, 1);

    write_register(0x11, 0); // Exit Sleep Mode (SLPOUT)
    sleep_ms(120);              
    write_register(0x29, 0); // Display ON

    printf("Display Engine Hardware Initialized.\n");
    ClearScreen(0x0000, 0, 0, 320, 240); // Flash clean black on boot
}

void LcdDisplay::HWReset() {
    gpio_put(LCD_PIN_RST, 0); sleep_ms(10);
    gpio_put(LCD_PIN_RST, 1); sleep_ms(120);
}

char LcdDisplay::SetWindow(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye) {
    write_register(0x2A, 0);
    write_register((xs >> 8), 1); write_register((xs & 0x00FF), 1);
    write_register((xe >> 8), 1); write_register((xe & 0x00FF), 1);
    
    write_register(0x2B, 0);
    write_register((ys >> 8), 1); write_register((ys & 0x00FF), 1);
    write_register((ye >> 8), 1); write_register((ye & 0x00FF), 1);
    return 1;
}

void LcdDisplay::ClearScreen(uint16_t color, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    uint8_t color1 = color;
    int32_t sizeofbuff = x1 * y1 * 2;
    SetWindow(x0, y0, x1 - 1, y1 - 1);
    write_register(0x2C, 0);
    
    cs_select(1);
    const uint chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_dreq(&c, spi_get_dreq(spi_default, true));
    channel_config_set_read_increment(&c, false);
    
    dma_channel_configure(chan, &c, &spi_get_hw(spi_default)->dr, &color1, sizeofbuff, true);
    dma_channel_wait_for_finish_blocking(chan);
    cs_deselect();
    dma_channel_unclaim(chan);
}

// ----------------------------------------------------------------------------
// 🌟 NEW HIGH-SPEED LVGL DISPLAY FLUSH DRIVER METHOD
// ----------------------------------------------------------------------------

#include <stdio.h>

// Bring in the global tracker handle declared in main.cpp
extern "C" {
    extern lv_obj_t * lbl_coordinates_tracker;
}

void LcdDisplay::FlushLvglDisplay(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    uint16_t x0 = area->x1; 
    uint16_t y0 = area->y1;
    uint16_t x1 = area->x2; 
    uint16_t y1 = area->y2;

    // Calculate total pixels in this frame slice
    uint32_t pixel_count = (uint32_t)(x1 - x0 + 1) * (uint32_t)(y1 - y0 + 1);
    const uint16_t *pixel_array = (const uint16_t *)px_map;

    // 🌟 DIAGNOSTIC LOGGER: Print the initial 7 raw pixel hex codes to Minicom
    //printf("\n\r[LVGL DEBUG SAMPLES] Area: (%d,%d) to (%d,%d)\n\r", x0, y0, x1, y1);
    //printf("First 7 Pixels: ");
    //for(int i = 0; i < 7; i++) {
    //    if (i < pixel_count) {
    //        printf("P%d:0x%04X ", i, pixel_array[i]);
    //    }
    //}
    //printf("\n\r");

    // 1. Point the ILI9341 display window directly to the active dirty box region
    SetWindow(x0, y0, x1, y1);
    write_register(0x2C, 0); // RAM Write Command Entry Point

    // 2. The Loop Streaming Core
    cs_select(1);
    for (uint32_t i = 0; i < pixel_count; i++) {
        uint16_t current_pixel = pixel_array[i];
        
        // Wait for physical hardware transmit FIFO slots to open up
        while (!spi_is_writable(spi0)) tight_loop_contents();
        spi_get_hw(spi0)->dr = (uint32_t)(current_pixel >> 8);   // Upper Byte First
        
        while (!spi_is_writable(spi0)) tight_loop_contents();
        spi_get_hw(spi0)->dr = (uint32_t)(current_pixel & 0xFF); // Lower Byte Second
        
        while (spi_is_readable(spi0)) (void)spi_get_hw(spi0)->dr;
    }
                                 
    while (spi_get_hw(spi0)->sr & SPI_SSPICR_RORIC_BITS) tight_loop_contents();
    while (spi_is_readable(spi0)) (void)spi_get_hw(spi0)->dr;
    spi_get_hw(spi0)->icr = SPI_SSPICR_RORIC_BITS;
    cs_deselect();

    lv_display_flush_ready(disp);
}

