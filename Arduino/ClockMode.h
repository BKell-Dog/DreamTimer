#ifndef CLOCK_MODE_H
#define CLOCK_MODE_H

#include <Arduino.h>
#include <time.h>
#include "StateManager.h"
#include "HardwareConfig.h"
#include "DisplayHelper.h"
#include "WifiHelper.h"

class ClockMode {
public:
  ClockMode(StateManager& sm, DisplayHelper& dh, WifiHelper& wh);
  
  void activate();
  void deactivate();
  void tick();
  
private:
  StateManager& stateManager;
  DisplayHelper& displayHelper;
  WifiHelper& wifiHelper;

  // Clock state
  unsigned long long lastSyncedMillis;
  time_t lastSyncedEpoch;
  unsigned long lastNtpSyncAttempt;
  unsigned long lastDisplayUpdate;
  
  // Internal methods
  void updateDisplay();
  void tryNTPSync();
  time_t getCurrentTime();
};

#endif