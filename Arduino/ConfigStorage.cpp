#include "ConfigStorage.h"

void ConfigStorage::begin() {
  Wire.begin(25, 26);  // SDA=25, SCL=26 (avoiding conflict with CONFIG_PIN on GPIO 22)
  Serial.println("[CONFIG] EEPROM initialized");
}

bool ConfigStorage::isConfigured() {
  return readByte(EEPROM_ADDR_MAGIC) == CONFIG_MAGIC_BYTE;
}

void ConfigStorage::markConfigured() {
  writeByte(EEPROM_ADDR_MAGIC, CONFIG_MAGIC_BYTE);
}

void ConfigStorage::clearConfig() {
  writeByte(EEPROM_ADDR_MAGIC, 0x00);
  Serial.println("[CONFIG] Configuration cleared");
}

void ConfigStorage::saveConfig(const DreamClockConfig& config) {
  Serial.println("[CONFIG] Saving configuration to EEPROM...");
  
  writeString(EEPROM_ADDR_SSID, config.ssid);
  writeString(EEPROM_ADDR_PASSWORD, config.password);
  writeString(EEPROM_ADDR_TIMEZONE, config.timezone);
  markConfigured();
  
  Serial.println("[CONFIG] Configuration saved successfully");
}

DreamClockConfig ConfigStorage::loadConfig() {
  DreamClockConfig config;
  
  Serial.println("[CONFIG] Loading configuration from EEPROM...");
  
  config.ssid = readString(EEPROM_ADDR_SSID, 32);
  config.password = readString(EEPROM_ADDR_PASSWORD, 64);
  config.timezone = readString(EEPROM_ADDR_TIMEZONE, 50);
  
  Serial.println("[CONFIG] Loaded:");
  Serial.println("  SSID: " + config.ssid);
  Serial.println("  Timezone: " + config.timezone);
  
  return config;
}

// Low-level EEPROM operations
void ConfigStorage::writeByte(uint16_t address, uint8_t data) {
  Wire.beginTransmission(EEPROM_I2C_ADDRESS);
  Wire.write((uint8_t)(address >> 8));    // High byte
  Wire.write((uint8_t)(address & 0xFF));  // Low byte
  Wire.write(data);
  Wire.endTransmission();
  delay(5);  // Write cycle time
}

uint8_t ConfigStorage::readByte(uint16_t address) {
  Wire.beginTransmission(EEPROM_I2C_ADDRESS);
  Wire.write((uint8_t)(address >> 8));
  Wire.write((uint8_t)(address & 0xFF));
  Wire.endTransmission();
  
  Wire.requestFrom(EEPROM_I2C_ADDRESS, 1);
  if (Wire.available()) {
    return Wire.read();
  }
  return 0;
}

void ConfigStorage::writeString(uint16_t address, const String& data) {
  uint8_t len = min((int)data.length(), 255);
  writeByte(address, len);  // Store length first
  
  for (uint8_t i = 0; i < len; i++) {
    writeByte(address + 1 + i, data[i]);
  }
}

String ConfigStorage::readString(uint16_t address, uint8_t maxLength) {
  uint8_t len = readByte(address);
  
  // Sanity check
  if (len == 0 || len > maxLength) {
    return "";
  }
  
  String result = "";
  result.reserve(len);
  
  for (uint8_t i = 0; i < len; i++) {
    char c = (char)readByte(address + 1 + i);
    result += c;
  }
  
  return result;
}
