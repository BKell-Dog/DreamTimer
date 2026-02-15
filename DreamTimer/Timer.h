#ifndef TIMER_H
#define TIMER_H

#include <Arduino.h>
#include "StateManager.h"
#include "HardwareConfig.h"
#include "DisplayHelper.h"

class Timer {
public:
  Timer(StateManager& sm, DisplayHelper& dh);
  
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