#include "EEPROMHelper.h"

/* ── Static: status to human-readable string ─────────────────────────────── */

const char* EEPROMHelper::statusToString(EEPROMStatus status)
{
    switch (status) {
        case EEPROM_OK:                return "OK";
        case EEPROM_ERR_WRITE:         return "ERROR: Write failed (I2C)";
        case EEPROM_ERR_READ:          return "ERROR: Read failed (I2C)";
        case EEPROM_ERR_NO_DATA:       return "ERROR: No valid data found (blank EEPROM?)";
        case EEPROM_ERR_CORRUPT:       return "ERROR: Data corrupt (bad stop magic)";
        case EEPROM_ERR_BUF_TOO_SMALL: return "ERROR: Read buffer too small";
        case EEPROM_ERR_TOO_LARGE:     return "ERROR: Payload too large for EEPROM";
        case EEPROM_ERR_INVALID_ARG:   return "ERROR: Invalid argument";
        default:                       return "ERROR: Unknown";
    }
}

void EEPROMHelper::scanBus()
{
    Serial.println("Scanning I2C bus...");

    int found = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.print("  Device found at 0x");
            Serial.print(addr, HEX);
            if (addr == 0x50)
                Serial.print("  <-- EEPROM (A0/A1/A2 = GND)");
            else if (addr >= 0x50 && addr <= 0x57)
                Serial.print("  <-- EEPROM (different A0/A1/A2 config)");
            Serial.println();
            found++;
        }
    }

    if (found == 0)
        Serial.println("  No I2C devices found. Check wiring and pull-up resistors.");
    else {
        Serial.print(found);
        Serial.println(" device(s) found.");
    }
}

EEPROMHelper::EEPROMHelper(uint8_t i2c_addr, uint16_t page_size, uint32_t max_bytes)
    : _addr(i2c_addr), _page_size(page_size), _max_bytes(max_bytes) {
        Wire.begin(EEPROM_SDA, EEPROM_SCL);
    }

/* ── Private: raw paged write ────────────────────────────────────────────── */

EEPROMStatus EEPROMHelper::writeRaw(uint16_t mem_addr, const uint8_t *buf, uint16_t len)
{
    uint16_t written = 0;

    while (written < len) {
        /* Clamp chunk to the remaining space in the current page */
        uint16_t page_offset   = (mem_addr + written) % _page_size;
        uint16_t space_in_page = _page_size - page_offset;
        uint16_t chunk         = min((uint16_t)(len - written), space_in_page);

        /* Arduino's Wire buffer is 32 bytes by default.
         * We further clamp to 30 to leave room for the 2-byte address. */
        chunk = min(chunk, (uint16_t)30);

        uint16_t target = mem_addr + written;

        Wire.beginTransmission(_addr);
        Wire.write((uint8_t)(target >> 8));     // high address byte
        Wire.write((uint8_t)(target & 0xFF));   // low  address byte
        Wire.write(buf + written, chunk);

        if (Wire.endTransmission() != 0)
            return EEPROM_ERR_WRITE;

        delay(WRITE_DELAY_MS);
        written += chunk;
    }

    return EEPROM_OK;
}

/* ── Private: raw sequential read ───────────────────────────────────────── */

EEPROMStatus EEPROMHelper::readRaw(uint16_t mem_addr, uint8_t *buf, uint16_t len)
{
    uint16_t received = 0;

    while (received < len) {
        /* Wire can request at most 32 bytes at a time */
        uint16_t chunk = min((uint16_t)(len - received), (uint16_t)32);
        uint16_t target = mem_addr + received;

        /* Set the read address */
        Wire.beginTransmission(_addr);
        Wire.write((uint8_t)(target >> 8));
        Wire.write((uint8_t)(target & 0xFF));
        if (Wire.endTransmission() != 0)
            return EEPROM_ERR_READ;

        if (Wire.requestFrom((uint8_t)_addr, (uint8_t)chunk) != chunk)
            return EEPROM_ERR_READ;

        for (uint16_t i = 0; i < chunk; i++)
            buf[received + i] = Wire.read();

        received += chunk;
    }

    return EEPROM_OK;
}

/* ── Public: write ───────────────────────────────────────────────────────── */

EEPROMStatus EEPROMHelper::write(uint16_t mem_addr, const uint8_t *data, uint16_t data_len)
{
    if (data == nullptr || data_len == 0)
        return EEPROM_ERR_INVALID_ARG;

    if ((uint32_t)mem_addr + data_len + FRAME_OVERHEAD > _max_bytes)
        return EEPROM_ERR_TOO_LARGE;

    uint8_t header[HEADER_SIZE] = {
        (uint8_t)(START_MAGIC >> 8), (uint8_t)(START_MAGIC & 0xFF),
        (uint8_t)(data_len   >> 8), (uint8_t)(data_len   & 0xFF)
    };

    uint8_t footer[FOOTER_SIZE] = {
        (uint8_t)(STOP_MAGIC >> 8), (uint8_t)(STOP_MAGIC & 0xFF)
    };

    EEPROMStatus ret;

    ret = writeRaw(mem_addr, header, HEADER_SIZE);
    if (ret != EEPROM_OK) return ret;

    ret = writeRaw(mem_addr + HEADER_SIZE, data, data_len);
    if (ret != EEPROM_OK) return ret;

    ret = writeRaw(mem_addr + HEADER_SIZE + data_len, footer, FOOTER_SIZE);
    return ret;
}

/* ── Public: read ────────────────────────────────────────────────────────── */

EEPROMStatus EEPROMHelper::read(uint16_t mem_addr, uint8_t *out_buf, uint16_t buf_len, uint16_t &out_len)
{
    if (out_buf == nullptr)
        return EEPROM_ERR_INVALID_ARG;

    uint8_t header[HEADER_SIZE];
    EEPROMStatus ret = readRaw(mem_addr, header, HEADER_SIZE);
    if (ret != EEPROM_OK) return ret;

    uint16_t start_magic = ((uint16_t)header[0] << 8) | header[1];
    if (start_magic != START_MAGIC)
        return EEPROM_ERR_NO_DATA;

    uint16_t data_len = ((uint16_t)header[2] << 8) | header[3];

    if (data_len == 0 || (uint32_t)mem_addr + data_len + FRAME_OVERHEAD > _max_bytes)
        return EEPROM_ERR_CORRUPT;

    if (data_len > buf_len)
        return EEPROM_ERR_BUF_TOO_SMALL;

    ret = readRaw(mem_addr + HEADER_SIZE, out_buf, data_len);
    if (ret != EEPROM_OK) return ret;

    uint8_t footer[FOOTER_SIZE];
    ret = readRaw(mem_addr + HEADER_SIZE + data_len, footer, FOOTER_SIZE);
    if (ret != EEPROM_OK) return ret;

    uint16_t stop_magic = ((uint16_t)footer[0] << 8) | footer[1];
    if (stop_magic != STOP_MAGIC)
        return EEPROM_ERR_CORRUPT;

    out_len = data_len;
    return EEPROM_OK;
}