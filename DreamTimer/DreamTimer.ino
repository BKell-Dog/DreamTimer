/*
 * DREAM Timer
 * 
 * A sleep tracking device with three modes:
 * - CLOCK: Displays current time (12-hour format) synced via NTP
 * - TIMER: Counts elapsed time with AWAKE/ASLEEP states
 * - CONFIG: Configuration portal for WiFi and timezone setup
 * 
 * Hardware:
 * - ESP32
 * - TM1637 6-digit display
 * - 24LC256 EEPROM (I2C)
 * - 2 LEDs (AWAKE/ASLEEP indicators)
 * - 2 switches (MODE, CONFIG)
 * - 1 button (state toggle/reset)
 */

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <TM1637TinyDisplay6.h>
#include "ConfigStorage.h"
#include "DisplayHelper.h"
#include "ConfigPortal.h"

// ========== PIN DEFINITIONS ==========
#define CLK_PIN        18   // TM1637 CLK
#define DIO_PIN        19   // TM1637 DIO
#define MODE_PIN       21   // Mode switch: HIGH = CLOCK, LOW = TIMER
#define CONFIG_PIN     22   // Config switch: LOW = CONFIG mode (overrides MODE_PIN)
#define BUTTON_PIN     23   // Pushbutton (active LOW)
#define SLEEP_LED_PIN  16   // LED for ASLEEP state
#define WAKE_LED_PIN   17   // LED for AWAKE state

// Note: I2C pins for EEPROM (SDA=21, SCL=22) are defined in ConfigStorage.cpp

// ========== TIMING CONSTANTS ==========
const unsigned long DISPLAY_UPDATE_MS = 1000UL;      // Display refresh rate
const unsigned long WIFI_CONNECT_TIMEOUT_MS = 10000; // WiFi connection timeout
const unsigned long NTP_SYNC_INTERVAL_MS = 300000UL;  // NTP sync every 300 seconds = 5 minutes
const unsigned long BUTTON_DEBOUNCE_MS = 50;         // Button debounce time

// ========== SYSTEM ENUMS ==========
enum SystemMode { MODE_CLOCK, MODE_TIMER, MODE_CONFIG };
enum SystemState { AWAKE, ASLEEP };

// ========== GLOBAL OBJECTS ==========
TM1637TinyDisplay6 display(CLK_PIN, DIO_PIN);
DisplayHelper displayHelper(&display);
ConfigStorage configStorage;
ConfigPortal* configPortal = nullptr;

// ========== CONFIGURATION ==========
DreamClockConfig config;
const char* NTP_SERVER = "pool.ntp.org";

// ========== RUNTIME STATE ==========
SystemMode systemMode = MODE_CLOCK;
SystemState systemState = AWAKE;
unsigned long lastDisplayUpdate = 0;
unsigned long lastNtpSyncAttempt = 0;
bool wifiConnected = false;

// Timer state
unsigned long timerStartMillis = 0;
unsigned long timerAccumulatedSeconds = 0;

// Button handling
int lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long buttonPressedAt = 0;
bool buttonWasDown = false;

// Clock sync tracking
time_t lastSyncedEpoch = 0;
unsigned long long lastSyncedMillis = 0ULL;

// ========== FORWARD DECLARATIONS ==========
void checkModeSwitches();
void setMode(SystemMode m);
void clockTick();
void timerTick();
void configTick();
void wifiConnectBlocking();
void wifiDisconnectSave();
void tryNtpSync();
time_t getCurrentClockTime();
void updateDisplayClock();
void updateDisplayTimer();
void handleButton();
void applyLedState();

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n========================================");
  Serial.println("    DREAM Timer    ");
  Serial.println("========================================\n");

  // Initialize pins
  pinMode(MODE_PIN, INPUT_PULLUP);
  pinMode(CONFIG_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(SLEEP_LED_PIN, OUTPUT);
  pinMode(WAKE_LED_PIN, OUTPUT);

  // Initialize display
  display.begin();
  display.setBrightness(BRIGHT_HIGH);
  display.clear();
  Serial.println("[INIT] Display initialized");

  // Initialize EEPROM
  configStorage.begin();

  // Load configuration
  if (configStorage.isConfigured()) {
    config = configStorage.loadConfig();
    Serial.println("[INIT] Configuration loaded from EEPROM");
  } else {
    // Default configuration
    Serial.println("[INIT] No configuration found, using defaults");
    config.ssid = "YourWiFiSSID";
    config.password = "YourWiFiPassword";
    config.timezone = "EST5EDT,M3.2.0/2,M11.1.0/2";
  }

  // Check initial mode
  checkModeSwitches();

  // Initialize based on mode
  if (systemMode == MODE_CONFIG) {
    // Config mode - handled in loop
    Serial.println("[INIT] Starting in CONFIG mode");
  } else if (systemMode == MODE_CLOCK) {
    Serial.println("[INIT] Starting in CLOCK mode");
    wifiConnectBlocking();
    if (wifiConnected) {
      configTzTime(config.timezone.c_str(), NTP_SERVER);
      tryNtpSync();
      lastNtpSyncAttempt = millis();
    }
  } else {
    Serial.println("[INIT] Starting in TIMER mode");
    wifiDisconnectSave();
    timerStartMillis = millis();
    timerAccumulatedSeconds = 0;
  }

  applyLedState();
  lastDisplayUpdate = millis();
  
  Serial.println("[INIT] Setup complete\n");
}

// ========== MAIN LOOP ==========
void loop() {
  // Check if mode switches have changed
  checkModeSwitches();

  // Run appropriate mode
  switch (systemMode) {
    case MODE_CLOCK:
      clockTick();
      break;
    case MODE_TIMER:
      timerTick();
      break;
    case MODE_CONFIG:
      configTick();
      break;
  }

  delay(5);  // Small delay for background tasks
}

// ========== MODE MANAGEMENT ==========
void checkModeSwitches() {
  int configPin = digitalRead(CONFIG_PIN);
  
  // CONFIG mode overrides everything (active LOW)
  if (configPin == LOW) {
    if (systemMode != MODE_CONFIG) {
      Serial.println("[MODE] Config switch activated");
      setMode(MODE_CONFIG);
    }
    return;
  }
  
  // Normal mode switching (CLOCK/TIMER)
  int modePin = digitalRead(MODE_PIN);
  SystemMode desired = (modePin == HIGH) ? MODE_CLOCK : MODE_TIMER;
  
  if (desired != systemMode && systemMode != MODE_CONFIG) {
    Serial.printf("[MODE] Switching to %s mode\n", 
                  (desired == MODE_CLOCK) ? "CLOCK" : "TIMER");
    setMode(desired);
  }
}

void setMode(SystemMode m) {
  SystemMode oldMode = systemMode;
  systemMode = m;

  if (m == MODE_CONFIG) {
    // Entering CONFIG mode
    Serial.println("[MODE] Entering CONFIG mode - starting portal");
    
    // Stop any existing WiFi connections
    WiFi.disconnect(true);
    
    // Create and start config portal
    if (configPortal == nullptr) {
      configPortal = new ConfigPortal(&configStorage);
    }
    configPortal->start();
    
    // Show config mode on display
    displayHelper.showConfigMode();
    
    // Turn off LEDs
    digitalWrite(WAKE_LED_PIN, LOW);
    digitalWrite(SLEEP_LED_PIN, LOW);
    
  } else if (m == MODE_CLOCK) {
    // Entering CLOCK mode
    if (oldMode == MODE_CONFIG && configPortal != nullptr) {
      configPortal->stop();
      delete configPortal;
      configPortal = nullptr;
      
      // Reload config if it was saved
      if (configStorage.isConfigured()) {
        config = configStorage.loadConfig();
      }
    }
    
    Serial.println("[MODE] Entering CLOCK mode");
    wifiConnectBlocking();
    if (wifiConnected) {
      configTzTime(config.timezone.c_str(), NTP_SERVER);
      tryNtpSync();
      lastNtpSyncAttempt = millis();
    }
    
    // Turn off LEDs
    digitalWrite(WAKE_LED_PIN, LOW);
    digitalWrite(SLEEP_LED_PIN, LOW);
    
  } else if (m == MODE_TIMER) {
    // Entering TIMER mode
    if (oldMode == MODE_CONFIG && configPortal != nullptr) {
      configPortal->stop();
      delete configPortal;
      configPortal = nullptr;
    }
    
    Serial.println("[MODE] Entering TIMER mode");
    wifiDisconnectSave();
    
    // Reset timer
    timerStartMillis = millis();
    timerAccumulatedSeconds = 0;
    
    // Apply LED state
    applyLedState();
    updateDisplayTimer();
  }
}

// ========== MODE TICK FUNCTIONS ==========
void clockTick() {
  unsigned long now = millis();

  // Reconnect if disconnected
  if (!wifiConnected && (now - lastNtpSyncAttempt >= 5000UL)) {
    Serial.println("[WiFi] Reconnecting...");
    wifiConnectBlocking();
    if (wifiConnected) {
      configTzTime(config.timezone.c_str(), NTP_SERVER);
    }
    lastNtpSyncAttempt = now;
  }

  // Periodic NTP sync
  if (wifiConnected && (now - lastNtpSyncAttempt >= NTP_SYNC_INTERVAL_MS)) {
    Serial.println("[NTP] Periodic sync");
    tryNtpSync();
    lastNtpSyncAttempt = now;
  }

  // Update display
  if (now - lastDisplayUpdate >= DISPLAY_UPDATE_MS) {
    lastDisplayUpdate = now;
    updateDisplayClock();
  }
}

void timerTick() {
  unsigned long now = millis();

  // Handle button
  handleButton();

  // Update display
  if (now - lastDisplayUpdate >= DISPLAY_UPDATE_MS) {
    lastDisplayUpdate = now;
    updateDisplayTimer();
  }
}

void configTick() {
  if (configPortal) {
    configPortal->handleClient();
    
    // Check if config was saved
    if (configPortal->configSaved()) {
      Serial.println("[CONFIG] Configuration saved, restarting in 3 seconds...");
      delay(3000);
      ESP.restart();
    }
  }
}

// ========== WIFI & NTP ==========
void wifiConnectBlocking() {
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("[WiFi] Already connected");
    return;
  }

  Serial.printf("[WiFi] Connecting to '%s'...\n", config.ssid.c_str());
  Serial.printf("[WiFi] Using password '%s'...\n", config.password.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(config.ssid.c_str(), config.password.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < WIFI_CONNECT_TIMEOUT_MS) {
    delay(200);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.print("[WiFi] Connected! IP: ");
    Serial.println(WiFi.localIP());
  } else {
    wifiConnected = false;
    Serial.println("[WiFi] Connection failed");
    WiFi.disconnect(true);
  }
}

void wifiDisconnectSave() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WiFi] Disconnecting to save power");
  }
  wifiConnected = false;
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

void tryNtpSync() {
  Serial.print("[NTP] Syncing");
  time_t now = time(nullptr);
  unsigned long start = millis();
  
  while (now < 24 * 3600 && (millis() - start) < 8000) {
    delay(200);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println();

  if (now < 24 * 3600) {
    Serial.println("[NTP] Sync failed");
  } else {
    lastSyncedEpoch = now;
    lastSyncedMillis = millis();
    
    struct tm tmInfo;
    localtime_r(&now, &tmInfo);
    Serial.printf("[NTP] Synced: %04d-%02d-%02d %02d:%02d:%02d\n",
                  tmInfo.tm_year + 1900, tmInfo.tm_mon + 1, tmInfo.tm_mday,
                  tmInfo.tm_hour, tmInfo.tm_min, tmInfo.tm_sec);
  }
}

time_t getCurrentClockTime() {
  if (lastSyncedEpoch == 0) {
    time_t t = time(nullptr);
    if (t >= 24 * 3600) return t;
    return 0;
  }
  
  unsigned long long nowMillis = millis();
  unsigned long long elapsedMs = nowMillis - lastSyncedMillis;
  time_t computed = lastSyncedEpoch + (time_t)(elapsedMs / 1000ULL);
  return computed;
}

// ========== DISPLAY FUNCTIONS ==========
void updateDisplayClock() {
  time_t computed = getCurrentClockTime();
  
  if (computed == 0) {
    Serial.println("[CLOCK] No valid time");
    displayHelper.showConfigMode();  // Show dashes
    return;
  }

  struct tm tmInfo;
  localtime_r(&computed, &tmInfo);

  int h = tmInfo.tm_hour;
  int m = tmInfo.tm_min;
  int s = tmInfo.tm_sec;

  // Display in 12-hour format with leading zero suppression
  displayHelper.displayTime12Hour(h, m, s);
  
  // Determine AM/PM for logging
  const char* ampm = (h >= 12) ? "PM" : "AM";
  int h12 = h % 12;
  if (h12 == 0) h12 = 12;
  
  Serial.printf("[CLOCK] %d:%02d:%02d %s\n", h12, m, s, ampm);
}

void updateDisplayTimer() {
  unsigned long elapsedSeconds = timerAccumulatedSeconds;
  unsigned long delta = (millis() - timerStartMillis) / 1000UL;
  elapsedSeconds += delta;

  unsigned int hours = elapsedSeconds / 3600;
  unsigned int minutes = (elapsedSeconds % 3600) / 60;
  unsigned int seconds = elapsedSeconds % 60;

  if (hours > 99) hours = hours % 100;  // Wrap to 2 digits

  // Display with leading zero suppression
  displayHelper.displayTimer(hours, minutes, seconds);

  Serial.printf("[TIMER] %02u:%02u:%02u - %s\n",
                hours, minutes, seconds,
                systemState == AWAKE ? "AWAKE" : "ASLEEP");
}

// ========== BUTTON HANDLING ==========
void handleButton() {
  int raw = digitalRead(BUTTON_PIN);
  unsigned long now = millis();

  // Debounce
  if (raw != lastButtonState) {
    lastDebounceTime = now;
    lastButtonState = raw;
  }

  if ((now - lastDebounceTime) > BUTTON_DEBOUNCE_MS) {
    // Button pressed
    if (raw == LOW && !buttonWasDown) {
      buttonPressedAt = now;
      buttonWasDown = true;
    }

    // Button released
    if (raw == HIGH && buttonWasDown) {
      buttonWasDown = false;

      // Toggle state and reset timer
      systemState = (systemState == AWAKE) ? ASLEEP : AWAKE;
      timerAccumulatedSeconds = 0;
      timerStartMillis = now;
      
      Serial.printf("[BUTTON] Toggled to %s, timer reset\n",
                    systemState == AWAKE ? "AWAKE" : "ASLEEP");
      
      applyLedState();
      updateDisplayTimer();
    }
  }
}

// ========== LED CONTROL ==========
void applyLedState() {
  if (systemMode == MODE_TIMER) {
    if (systemState == AWAKE) {
      digitalWrite(WAKE_LED_PIN, HIGH);
      digitalWrite(SLEEP_LED_PIN, LOW);
    } else {
      digitalWrite(WAKE_LED_PIN, LOW);
      digitalWrite(SLEEP_LED_PIN, HIGH);
    }
  } else {
    digitalWrite(WAKE_LED_PIN, LOW);
    digitalWrite(SLEEP_LED_PIN, LOW);
  }
}
