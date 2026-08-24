
#ifndef EEPROM_HPP
#define EEPROM_HPP

#include "pico/stdlib.h"
#include "hardware/i2c.h"

class Eeprom {
private:
    i2c_inst_t* i2c_port;
    uint8_t eeprom_address;

public:
    // Constructor matching your specific 0x57 address configurations
    Eeprom(i2c_inst_t* port = i2c0, uint8_t address = 0x57);

    // Optimized multi-byte read and write hardware blocking loops
    void Read(uint8_t memory_location, uint8_t* buffer, uint8_t length);
    void Write(uint16_t memory_address, uint16_t data, uint8_t data_length);
    
    // Convenience helper methods for reading and writing single 8-bit bytes
    void WriteByte(uint8_t memory_location, uint8_t value);
    uint8_t ReadByte(uint8_t memory_location);
};

// Share a single global instance across your other .cpp files
extern Eeprom eeprom;

#endif
