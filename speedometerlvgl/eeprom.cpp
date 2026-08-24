
#include "eeprom.hpp"

// Instantiate the global EEPROM handle using your i2c0 configuration
Eeprom eeprom(i2c0, 0x57);

Eeprom::Eeprom(i2c_inst_t* port, uint8_t address) 
    : i2c_port(port), eeprom_address(address) {}

void Eeprom::Read(uint8_t memory_location, uint8_t* buffer, uint8_t length) {
    uint8_t buf[2];
    buf[0] = 0x00; // High byte memory location offset boundary
    buf[1] = memory_location; // Low byte memory target
    
    // Write address cursor pointer, keeping master control of bus
    i2c_write_blocking(i2c_port, eeprom_address, buf, 2, true); 
    // Read the sequential data blocks
    i2c_read_blocking(i2c_port, eeprom_address, buffer, length, false);        
    sleep_ms(7); // Allow I2C bus stability window
}

void Eeprom::Write(uint16_t memory_address, uint16_t data, uint8_t data_length) {
    uint8_t buf[3];
    buf[0] = 0x00;
    buf[1] = (uint8_t)memory_address; // Unpack lower byte offset boundaries
    buf[2] = (uint8_t)data;           // Map data payload byte
    
    i2c_write_blocking(i2c_port, eeprom_address, buf, 3, false); // Stream write array
    sleep_ms(10); // Crucial! EEPROM hardware internal write cycle timing window delay
}

void Eeprom::WriteByte(uint8_t memory_location, uint8_t value) {
    Write(memory_location, value, 1);
}

uint8_t Eeprom::ReadByte(uint8_t memory_location) {
    uint8_t result = 0;
    Read(memory_location, &result, 1);
    return result;
}
