#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "HardwareConfig.h"

/*
 * EEPROMHelper
 *
 * Wraps read/write access to an I2C EEPROM (AT24Cxx series) with a simple
 * framed format so data integrity can be verified on read-back.
 *
 * Frame layout (written from address 0x0000):
 *   [ START_MAGIC (2 bytes) | data_len (2 bytes) | payload (N bytes) | STOP_MAGIC (2 bytes) ]
 */

/* Return codes */
enum EEPROMStatus {
    EEPROM_OK               =  0,
    EEPROM_ERR_WRITE        = -1,   // I2C write failed
    EEPROM_ERR_READ         = -2,   // I2C read failed
    EEPROM_ERR_NO_DATA      = -3,   // Start magic not found (blank memory?)
    EEPROM_ERR_CORRUPT      = -4,   // Stop magic missing or length invalid
    EEPROM_ERR_BUF_TOO_SMALL= -5,   // Caller's buffer too small for payload
    EEPROM_ERR_TOO_LARGE    = -6,   // Payload + overhead exceeds EEPROM capacity
    EEPROM_ERR_INVALID_ARG  = -7,   // Null pointer or zero-length argument
};

class EEPROMHelper {
public:
    /*
     * @param i2c_addr   7-bit I2C address of the EEPROM (default 0x50, all addr pins low)
     * @param page_size  Page size in bytes for 24LC256 is 64 by default.
     * @param max_bytes  Total capacity in bytes
     */
    EEPROMHelper(uint8_t i2c_addr = 0x50,
                 uint16_t page_size = 64,
                 uint32_t max_bytes = 32768);

    /*
     * Write `data_len` bytes from `data` into EEPROM, framed with
     * start/stop delimiters. Always overwrites from address 0.
     *
     * @return EEPROM_OK on success, negative EEPROMStatus on failure.
     */
    EEPROMStatus write(uint16_t mem_addr, const uint8_t *data, uint16_t data_len);

    /*
     * Read the framed payload back from EEPROM into `out_buf`.
     *
     * @param out_buf   Caller-supplied buffer to receive the payload.
     * @param buf_len   Size of `out_buf` in bytes.
     * @param out_len   Set to the actual number of bytes written into `out_buf`.
     * @return          EEPROM_OK on success, negative EEPROMStatus on failure.
     */
    EEPROMStatus read(uint16_t mem_addr, uint8_t *out_buf, uint16_t buf_len, uint16_t &out_len);

    /*
     * Serialize any plain struct into EEPROM, always at address 0.
     * e.g. eeprom.writeConfig(myConfig);
     */
    template<typename T>
    EEPROMStatus writeConfig(const T& config) {
        return write(0, (const uint8_t*)&config, sizeof(T));
    }

    /*
     * Deserialize a plain struct back from EEPROM, always from address 0.
     * Returns EEPROM_ERR_CORRUPT if the stored size doesn't match sizeof(T).
     * e.g. eeprom.readConfig(myConfig);
     */
    template<typename T>
    EEPROMStatus readConfig(T& config) {
        uint16_t len = 0;
        EEPROMStatus status = read(0, (uint8_t*)&config, sizeof(T), len);
        if (status != EEPROM_OK) return status;
        if (len != sizeof(T))    return EEPROM_ERR_CORRUPT;
        return EEPROM_OK;
    }

    /*
     * Returns the first address after the config region — i.e. the start of
     * free space for other data. Use this as the mem_addr argument to write()
     * and read() when storing non-config data.
     *
     * e.g. eeprom.write(EEPROMHelper::configEndAddress<DeviceConfig>(), buf, len);
     */
    template<typename T>
    static constexpr uint16_t configEndAddress() {
        return static_cast<uint16_t>(sizeof(T) + FRAME_OVERHEAD);
    }

    /*
     * Converts an EEPROMStatus code to a human-readable string.
     * Static so it can be called without an instance: EEPROMHelper::statusToString(s)
     */
    static const char* statusToString(EEPROMStatus status);

     /*
     * Scans the I2C bus and prints all responding addresses to Serial.
     * Useful for sanity-checking wiring and confirming the EEPROM is visible.
     * Static so it can be called without an instance, e.g. EEPROMHelper::scanBus()
     */
    static void scanBus();

private:
    uint8_t  _addr;
    uint16_t _page_size;
    uint32_t _max_bytes;

    static constexpr uint16_t START_MAGIC    = 0xABCD;
    static constexpr uint16_t STOP_MAGIC     = 0xDCBA;
    static constexpr uint8_t  HEADER_SIZE    = 4;   // 2-byte magic + 2-byte length
    static constexpr uint8_t  FOOTER_SIZE    = 2;
    static constexpr uint8_t  FRAME_OVERHEAD = HEADER_SIZE + FOOTER_SIZE;
    static constexpr uint8_t  WRITE_DELAY_MS = 5;

    EEPROMStatus writeRaw(uint16_t mem_addr, const uint8_t *buf, uint16_t len);
    EEPROMStatus readRaw (uint16_t mem_addr, uint8_t *buf,       uint16_t len);
};
