#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

// ========== HARDCODED WIFI CREDENTIALS ==========
#define WIFI_SSID     "Kelly Highway"
#define WIFI_PASSWORD "w@yn3_1c3_v@ult"

#define TIMEZONE "EST5EDT,M3.2.0/2,M11.1.0/2"  // Eastern U.S. Time
#define NTP_SERVER "pool.ntp.org"

// ========== PIN DEFINITIONS ==========
#define CLK_PIN        18   // TM1637 CLK
#define DIO_PIN        19   // TM1637 DIO
#define MODE_PIN       21   // Mode switch: HIGH = CLOCK, LOW = TIMER
#define BUTTON_PIN     23   // Pushbutton (active LOW)
#define SLEEP_LED_PIN  16   // LED for ASLEEP state (BLUE)
#define WAKE_LED_PIN   17   // LED for AWAKE state (YELLOW)

// ========== TIMING CONSTANTS ==========
const unsigned long DISPLAY_UPDATE_MS = 1000UL;       // Display refresh rate
const unsigned long WIFI_CONNECT_TIMEOUT_MS = 10000;  // WiFi connection timeout
const unsigned long NTP_SYNC_INTERVAL_MS = 300000UL;  // NTP sync every 5 minutes
const unsigned long BUTTON_DEBOUNCE_MS = 50;          // Button debounce delay
const unsigned long ERROR_DISPLAY_MS = 3000;          // How long to show error messages
const int MAX_NTP_RETRIES = 5;                        // Maximum NTP fetch retries

#endif