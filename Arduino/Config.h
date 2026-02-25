#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/*
 * DeviceConfig
 *
 * Persistent device configuration stored in EEPROM.
 * Any other settings that should survive a reboot will be stored here.
 */
struct DeviceConfig {
    char wifi_ssid[64];
    char wifi_password[64];
};

#endif