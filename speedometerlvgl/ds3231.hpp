#ifndef DS3231_HPP
#define DS3231_HPP

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <string>

#define DS3231_ADDRESS 0x68

// Clean C++ structure to hold unpacked time parameters safely
struct RtcTime {
    uint8_t sec;
    uint8_t min;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
};

class Ds3231 {
private:
    i2c_inst_t* i2c_port;
    uint8_t sda_pin;
    uint8_t scl_pin;
    uint8_t raw_buffer[7]; // Holds raw BCD bytes read from the chip

    // Internal BCD conversion math helpers
    uint8_t bcdToDec(uint8_t val) { return ((val / 16) * 10) + (val % 16); }
    uint8_t decToBcd(uint8_t val) { return ((val / 10) * 16) + (val % 10); }

public:
    // Constructor mapping default hardware pins from your schematic
    Ds3231(i2c_inst_t* port = i2c0, uint8_t sda = 20, uint8_t scl = 21);

    void Init();
    void SetTime(uint8_t hour, uint8_t min, uint8_t sec);
    void SetDate(uint8_t day, uint8_t month, uint16_t year);
    
    // Core function to read the chip and return a fully decoded C++ object layout
    RtcTime GetTime(); 
};

// Share a single global instance across your other .cpp project files
extern Ds3231 rtc;

#endif

