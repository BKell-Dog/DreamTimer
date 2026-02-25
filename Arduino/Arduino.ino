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
#include <TM1637TinyDisplay6.h>
#include <Wire.h>
#include "DisplayHelper.h"
#include "HardwareConfig.h"
#include "StateManager.h"
#include "TimerMode.h"
#include "ClockMode.h"
#include "EEPROMHelper.h"
#include "Config.h"

StateManager stateManager;

TM1637TinyDisplay6 display(CLK_PIN, DIO_PIN);
DisplayHelper displayHelper(&display);
WifiHelper wifiHelper;

EEPROMHelper eepromHelper;

TimerMode timerMode(stateManager, displayHelper);
ClockMode clockMode(stateManager, displayHelper, wifiHelper, eepromHelper);

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  delay(3000); // Long delay to allow Serial.
  Serial.println("\n=== DREAM Clock Starting ===");

  // Hardcode wifi credentials into the EEPROM
  DeviceConfig cfg;
  strncpy(cfg.wifi_ssid,     "WIFI_SSID",     sizeof(cfg.wifi_ssid)     - 1);
  strncpy(cfg.wifi_password, "WIFI_PASS", sizeof(cfg.wifi_password) - 1);
  eepromHelper.writeConfig(cfg);

  // Initialize pins
  pinMode(MODE_PIN, INPUT_PULLUP); // Button & switch are ACTIVE LOW
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(SLEEP_LED_PIN, OUTPUT);
  pinMode(WAKE_LED_PIN, OUTPUT);

  // Initialize display
  display.begin();
  display.setBrightness(BRIGHT_HIGH);
  display.clear();

  stateManager.begin();

  // Initialize mode at boot
  stateManager.checkModeSwitch();

  // If starting in CLOCK mode, begin Wi-Fi + NTP
  if (stateManager.getCurrentMode() == SystemMode::CLOCK) {
    timerMode.deactivate();
    clockMode.activate();
  } else {
    // SystemMode == TIMER
    clockMode.deactivate();
    timerMode.activate();
  }
}

// ------------------------- Main Loop -------------------------
void loop() {
  // Pre-step: check if the hardware pin changed and if so, change the mode.
  stateManager.checkModeSwitch();

  if (stateManager.modeChanged()) {
    if (stateManager.getCurrentMode() == SystemMode::CLOCK) {
      timerMode.deactivate();
      clockMode.activate();
    } else if (stateManager.getCurrentMode() == SystemMode::TIMER) {
      clockMode.deactivate();
      timerMode.activate();
    }
  }

  if (stateManager.getCurrentMode() == SystemMode::CLOCK) {
    clockMode.tick();
  } else {
    // SystemMode == TIMER
    timerMode.tick();
  }

  // tiny delay to yield to background (Wi-Fi / tasks)
  delay(5);
}