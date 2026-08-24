
#include "ds3231.hpp"

// Instantiate the global object instance exactly once in your project
Ds3231 rtc(i2c0, 20, 21);

Ds3231::Ds3231(i2c_inst_t* port, uint8_t sda, uint8_t scl) 
    : i2c_port(port), sda_pin(sda), scl_pin(scl) {
    for(int i = 0; i < 7; i++) raw_buffer[i] = 0;
}

void Ds3231::Init() {
    // Initialize the physical I2C peripheral hardware engine at standard 100 kHz speed
    i2c_init(i2c_port, 100 * 1000);
    gpio_set_function(sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(sda_pin);
    gpio_pull_up(scl_pin);
}

void Ds3231::SetTime(uint8_t hour, uint8_t min, uint8_t sec) {
    uint8_t tx_buf[2];
    
    tx_buf[0] = 0x00; // Seconds address register
    tx_buf[1] = decToBcd(sec);
    i2c_write_blocking(i2c_port, DS3231_ADDRESS, tx_buf, 2, false);

    tx_buf[0] = 0x01; // Minutes address register
    tx_buf[1] = decToBcd(min);
    i2c_write_blocking(i2c_port, DS3231_ADDRESS, tx_buf, 2, false);

    tx_buf[0] = 0x02; // Hours address register
    tx_buf[1] = decToBcd(hour); // Default config forces native 24-hour mode layout
    i2c_write_blocking(i2c_port, DS3231_ADDRESS, tx_buf, 2, false);
}

void Ds3231::SetDate(uint8_t day, uint8_t month, uint16_t year) {
    uint8_t tx_buf[2];
    uint8_t short_year = (uint8_t)(year % 100); // e.g., 2026 becomes 26

    tx_buf[0] = 0x04; // Day address register
    tx_buf[1] = decToBcd(day);
    i2c_write_blocking(i2c_port, DS3231_ADDRESS, tx_buf, 2, false);

    tx_buf[0] = 0x05; // Month address register
    tx_buf[1] = decToBcd(month);
    i2c_write_blocking(i2c_port, DS3231_ADDRESS, tx_buf, 2, false);

    tx_buf[0] = 0x06; // Year address register
    tx_buf[1] = decToBcd(short_year);
    i2c_write_blocking(i2c_port, DS3231_ADDRESS, tx_buf, 2, false);
}

RtcTime Ds3231::GetTime() {
    uint8_t reg_addr = 0x00;  
    RtcTime parsed_time;

    // Point the internal address cursor register back to index 0 (Seconds)
    i2c_write_blocking(i2c_port, DS3231_ADDRESS, &reg_addr, 1, true);
    // Stream all 7 data bytes out of the chip sequentially into local memory
    i2c_read_blocking(i2c_port, DS3231_ADDRESS, raw_buffer, 7, false);

    // Unpack, convert, and format the results natively
    parsed_time.sec   = bcdToDec(raw_buffer[0] & 0x7F);
    parsed_time.min   = bcdToDec(raw_buffer[1] & 0x7F);
    parsed_time.hour  = bcdToDec(raw_buffer[2] & 0x3F); // Unmasks 24-hour structure data bits
    parsed_time.day   = bcdToDec(raw_buffer[4] & 0x3F);
    parsed_time.month = bcdToDec(raw_buffer[5] & 0x1F);
    parsed_time.year  = bcdToDec(raw_buffer[6]) + 2000; // Adjusts base offset back to 4 digits

    return parsed_time;
}
