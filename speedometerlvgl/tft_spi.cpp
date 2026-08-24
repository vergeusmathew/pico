#include "pico/stdlib.h"
#include "hardware/spi.h"

// Define the matching schematic pins exactly
#define ILI9341_DC             15  // LCD_DC Control Line
#define ILI9341_CS             17  // LCD_CS Chip Select Line
#define HARDWARE_SPI           1   // Assertions flag constant mapping

// Wrap these functions in an extern "C" block inside tft_spi.cpp 
// so the C++ compiler exports simple, clean function names for lcd.cpp to link with
extern "C" {

void cs_select(uint dc) {
    asm volatile("nop \n nop \n nop");
    if(dc == 0) { // Command
        gpio_put(ILI9341_DC, 0);
    } else {      // Data
        gpio_put(ILI9341_DC, 1);
    }    
    asm volatile("nop \n nop \n nop");
    gpio_put(ILI9341_CS, 0);  // Active-Low CS drive drop
    asm volatile("nop \n nop \n nop");
}

void cs_deselect() {
    asm volatile("nop \n nop \n nop");
    gpio_put(ILI9341_CS, 1);  // De-assert CS line high
    asm volatile("nop \n nop \n nop");
}


int __not_in_flash_func(myspi_write16_blocking)(spi_inst_t *spi, const uint32_t src, size_t len) {
    // Paste your exact high-speed myspi_write16_blocking code block here...
    while (!spi_is_writable(spi)) tight_loop_contents();
    spi_get_hw(spi)->dr = (uint32_t)src;
    while (spi_is_readable(spi)) (void)spi_get_hw(spi)->dr;
    while (spi_get_hw(spi)->sr & SPI_SSPSR_BSY_BITS) tight_loop_contents();
    while (spi_is_readable(spi)) (void)spi_get_hw(spi)->dr;
    spi_get_hw(spi)->icr = SPI_SSPICR_RORIC_BITS;
    return (int)len;
}


int __not_in_flash_func(myspi_write_blocking)(spi_inst_t *spi, const uint8_t src, size_t len) {
    // Paste your exact myspi_write_blocking code block here...
    while (!spi_is_writable(spi)) tight_loop_contents();
    spi_get_hw(spi)->dr = (uint32_t)src;
    while (spi_is_readable(spi)) (void)spi_get_hw(spi)->dr;
    while (spi_get_hw(spi)->sr & SPI_SSPSR_BSY_BITS) tight_loop_contents();
    while (spi_is_readable(spi)) (void)spi_get_hw(spi)->dr;
    spi_get_hw(spi)->icr = SPI_SSPICR_RORIC_BITS;
    return (int)len;
}

// ADD THIS INSIDE THE EXISTNG extern "C" WRAPPER BLOCK IN tft_spi.cpp
void __not_in_flash_func(myspi_write16_array_blocking)(spi_inst_t *spi, const uint16_t *src, size_t len) {
    for (size_t i = 0; i < len; i++) {
        // Wait for the hardware transmitter FIFO slot to open up
        while (!spi_is_writable(spi)) tight_loop_contents();
        
        // Write the unique i-th pixel from your LVGL canvas memory straight to the SPI register
        spi_get_hw(spi)->dr = (uint32_t)src[i]; 
        
        // Clear out the read buffer instantly to avoid overflow register locks
        while (spi_is_readable(spi)) (void)spi_get_hw(spi)->dr;
    }
    
    // Wait until the final bit stream completely clears the physical wire before finishing
    while (spi_get_hw(spi)->sr & SPI_SSPSR_BSY_BITS) tight_loop_contents();
    while (spi_is_readable(spi)) (void)spi_get_hw(spi)->dr;
    spi_get_hw(spi)->icr = SPI_SSPICR_RORIC_BITS;
}


void write_register(uint8_t data, uint8_t dc) {
    cs_select(dc);
    myspi_write_blocking(spi0, data, 1); // Point directly to your hardware spi0 engine instances
    cs_deselect();
}

void write_register16(uint32_t data, uint8_t dc) {
    cs_select(dc);
    myspi_write16_blocking(spi0, data, 1);
    cs_deselect();
}

} // End of extern "C"

