#ifndef TIMER_MODE_H
#define TIMER_MODE_H

#include <Arduino.h>
#include "StateManager.h"
#include "HardwareConfig.h"
#include "DisplayHelper.h"

class TimerMode {
public:
  TimerMode(StateManager& sm, DisplayHelper& dh);
  
  void activate();
  void deactivate();
  void tick();
  
private:
  StateManager& stateManager;
  DisplayHelper& displayHelper;
  
  // Timer state
  unsigned long timerStartMillis;
  unsigned long timerAccumulatedSeconds;
  unsigned long lastDisplayUpdate;
  
  // Internal methods
  void updateDisplay();
  void updateLEDs();
  void resetTimer();
  void LEDsOff();
};

#endif