/*
The DREAM Clock

This script will control the Dream Clock device.
Device will exist in two modes: CLOCK and TIMER.
CLOCK will connect to Wifi periodically and to learn the correct time in EST, and show that time on the TM1637
display. It will count up offline, but reconnect one per period (e.g. 1 min) to fetch the correct time.
TIMER will simply count up from 0, in units of seconds, minutes, and hours, and can be reset to 0 via a button.
In TIMER there will also be two system states, indicated by activating a different LED: SLEEP and WAKE, which
indicate if the subject is asleep or awake. Pressing the aforementioned button will both reset the timer and
invert which LED is on.

The two system modes, CLOCK and TIMER, will be determined by a circuit switch which will either drive a pin HIGH
or LOW, and the TIMER state will be determined by an internal system variable and reset/switched by a button press
connected to a pin. There will also be an ON/OFF switch which breaks the main battery circuit.

MODE
 - CLOCK
  - Every 1 min, connect to Wifi and attempt to query time.
  - Display EST and count up every second.
  - Both LEDs OFF
 - TIMER
  - State
    - SLEEP
      - Sleep LED on.
      - Timer reset to 0.
      - Count up.
    - WAKE
      - Wake LED on.
      - Timer reset to 0.
      - Count up.
*/

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <TM1637TinyDisplay6.h>
#include "DisplayHelper.h"
#include "HardwareConfig.h"
#include "StateManager.h"
#include "Timer.h"

// U.S. Eastern time (EST/EDT)
const char* TZ_CONFIG = "EST5EDT,M3.2.0/2,M11.1.0/2";
const char* NTP_SERVER = "pool.ntp.org";

// Timing / behavior
const unsigned long BUTTON_SHORT_MS = 800;  // <= this = short press (toggle SLEEP/WAKE and reset)
const unsigned long BUTTON_LONG_MS  = 1500; // > this = long press (reset timer)
// ===================================

StateManager systemState;

TM1637TinyDisplay6 display(CLK_PIN, DIO_PIN);
DisplayHelper displayHelper(&display);
uint8_t digits[6];

Timer timer(systemState, displayHelper);

unsigned long lastDisplayUpdate = 0;
unsigned long lastNtpSyncAttempt = 0;
bool wifiConnected = false;

// CLOCK sync bookkeeping
time_t lastSyncedEpoch = 0;          // last epoch seconds we received from NTP
unsigned long long lastSyncedMillis = 0ULL; // millis() at the moment we recorded lastSyncedEpoch

// Forward declarations
void wifiConnectBlocking();
void wifiDisconnectSave();
void tryNtpSync(); // attempt one sync and record baseline if successful
time_t getCurrentClockTime(); // return computed local clock (lastSyncedEpoch + elapsed)
void updateDisplayClock();
void updateDisplayTimer();

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  delay(50);
  Serial.println("\n=== DREAM Clock Starting ===");

  // Initialize pins
  pinMode(MODE_PIN, INPUT_PULLUP); // Button & switch are ACTIVE LOW
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(SLEEP_LED_PIN, OUTPUT);
  pinMode(WAKE_LED_PIN, OUTPUT);

  // Initialize display
  display.begin();
  display.setBrightness(BRIGHT_HIGH);
  display.clear();

  systemState.begin();

  // Initialize mode at boot
  systemState.checkModeSwitch();

  // If starting in CLOCK mode, begin Wi-Fi + NTP
  if (systemState.getCurrentMode() == SystemMode::CLOCK) {
    Serial.println("[INIT] Starting in CLOCK mode -> connecting Wi-Fi & NTP");
    wifiConnectBlocking();
    if (wifiConnected) {
      configTzTime(TZ_CONFIG, NTP_SERVER);
      tryNtpSync(); // immediate sync attempt, records baseline if successful
      lastNtpSyncAttempt = millis();
    } else {
      Serial.println("[INIT] Wi-Fi not connected; will attempt periodic connect/sync.");
    }
  } else {
    Serial.println("[INIT] Starting in TIMER mode -> Wi-Fi disabled to save power");
    wifiDisconnectSave();
    timer.activate();
  }
}

// ------------------------- Main Loop -------------------------
void loop() {
  // Pre-step: check if the hardware pin changed and if so, change the mode.
  systemState.checkModeSwitch();

  if (systemState.getCurrentMode() == SystemMode::CLOCK) {
    clockTick();
  } else {
    // SystemMode = TIMER
    timer.tick();
  }

  // tiny delay to yield to background (Wi-Fi / tasks)
  delay(5);
}

void clockTick() {
  unsigned long now = millis();

  // If disconnected, try to reconnect every 5s
  if (!wifiConnected && (now - lastNtpSyncAttempt >= 5000UL)) {
    Serial.println("[WiFi] Attempting reconnect (CLOCK mode)");
    wifiConnectBlocking();
    if (wifiConnected) {
      configTzTime(TZ_CONFIG, NTP_SERVER);
    }
    lastNtpSyncAttempt = now;
  }

  // If connected, try NTP sync every NTP_SYNC_INTERVAL_MS
  if (wifiConnected && (now - lastNtpSyncAttempt >= NTP_SYNC_INTERVAL_MS)) {
    Serial.println("[NTP] Periodic sync attempt");
    tryNtpSync();
    lastNtpSyncAttempt = now;
  }

  // Update display every second
  if (now - lastDisplayUpdate >= DISPLAY_UPDATE_MS) {
    lastDisplayUpdate = now;
    updateDisplayClock();
  }
}

// ------------------------- Wi-Fi & NTP -------------------------
void wifiConnectBlocking() {
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("[WiFi] Already connected");
    return;
  }

  Serial.printf("[WiFi] Connecting to '%s' ...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < WIFI_CONNECT_TIMEOUT_MS) {
    delay(200);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.print("[WiFi] Connected, IP=");
    Serial.println(WiFi.localIP());
  } else {
    wifiConnected = false;
    Serial.println("[WiFi] Failed to connect (timeout)");
    // optionally disable Wi-Fi to save power if not connected
    WiFi.disconnect(true);
  }
}

void wifiDisconnectSave() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WiFi] Disconnecting to save power (TIMER mode)");
  }
  wifiConnected = false;
  WiFi.disconnect(true); // disconnect and erase credentials from driver to reduce radio activity
  WiFi.mode(WIFI_OFF);
}

// Attempt a synchronous NTP sync and record baseline (lastSyncedEpoch + lastSyncedMillis)
void tryNtpSync() {
  Serial.print("[NTP] Trying to sync");
  time_t now = time(nullptr);
  unsigned long start = millis();
  // Wait briefly until time() becomes "reasonable" (SNTP background may update)
  while (now < 24 * 3600 && (millis() - start) < 8000) { // wait up to 8s
    delay(200);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println();

  if (now < 24 * 3600) {
    Serial.println("[NTP] Sync failed / no valid time received");
    // keep lastSyncedEpoch untouched so clock will continue from last known value
  } else {
    lastSyncedEpoch = now;
    lastSyncedMillis = (unsigned long long)millis();
    struct tm tmInfo;
    localtime_r(&now, &tmInfo);
    Serial.printf("[NTP] Synced: %04d-%02d-%02d %02d:%02d:%02d (recording baseline)\n",
                  tmInfo.tm_year + 1900, tmInfo.tm_mon + 1, tmInfo.tm_mday,
                  tmInfo.tm_hour, tmInfo.tm_min, tmInfo.tm_sec);
  }
}

// Compute current clock time using lastSyncedEpoch + elapsed seconds (millis-based)
time_t getCurrentClockTime() {
  if (lastSyncedEpoch == 0) {
    // No baseline yet — try to return system time() (may be wrong) or 0
    time_t t = time(nullptr);
    if (t >= 24 * 3600) return t;
    return 0;
  }
  unsigned long long nowMillis = (unsigned long long)millis();
  unsigned long long elapsedMs = nowMillis - lastSyncedMillis;
  time_t computed = lastSyncedEpoch + (time_t)(elapsedMs / 1000ULL);
  return computed;
}

// ------------------------- Display functions -------------------------
void updateDisplayClock() {
  time_t computed = getCurrentClockTime();
  if (computed == 0) {
    // No valid time yet — show dashes
    Serial.println("[CLOCK] No valid time baseline; showing ----");
    for (int i = 0; i < 6; ++i) digits[i] = display.encodeDigit(10); // attempt blank (may show nothing)
    display.setSegments(digits, 6, 0);
    return;
  }

  struct tm tmInfo;
  // Convert computed epoch to local broken-down time respecting TZ (we used configTzTime earlier)
  // localtime_r expects the system timezone to be set; timegm/time zone handling is via configTzTime
  time_t t_for_localtime = computed;
  localtime_r(&t_for_localtime, &tmInfo);

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