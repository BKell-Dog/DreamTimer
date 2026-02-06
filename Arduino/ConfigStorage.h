#ifndef CONFIG_STORAGE_H
#define CONFIG_STORAGE_H

#include <Arduino.h>
#include <Wire.h>

// EEPROM I2C address
#define EEPROM_I2C_ADDRESS 0x50

// EEPROM memory map
#define EEPROM_ADDR_MAGIC      0    // Magic byte to check if configured
#define EEPROM_ADDR_SSID       10   // WiFi SSID (max 32 chars)
#define EEPROM_ADDR_PASSWORD   100  // WiFi Password (max 64 chars)
#define EEPROM_ADDR_TIMEZONE   200  // Timezone string (max 50 chars)

#define CONFIG_MAGIC_BYTE 0xAA      // Magic number to mark as configured

// Configuration structure
struct DreamClockConfig {
  String ssid;
  String password;
  String timezone;
};

class ConfigStorage {
public:
  // Initialize I2C for EEPROM
  void begin();
  
  // Check if device has been configured
  bool isConfigured();
  
  // Mark device as configured
  void markConfigured();
  
  // Clear configuration (factory reset)
  void clearConfig();
  
  // Save configuration to EEPROM
  void saveConfig(const DreamClockConfig& config);
  
  // Load configuration from EEPROM
  DreamClockConfig loadConfig();
  
private:
  // Low-level EEPROM operations
  void writeByte(uint16_t address, uint8_t data);
  uint8_t readByte(uint16_t address);
  void writeString(uint16_t address, const String& data);
  String readString(uint16_t address, uint8_t maxLength);
};

#endif // CONFIG_STORAGE_H
