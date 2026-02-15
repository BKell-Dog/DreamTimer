#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include <Arduino.h>
#include "HardwareConfig.h"

// ========== SYSTEM ENUMS ==========
enum class SystemMode { CLOCK, TIMER };
enum class SystemState { AWAKE, ASLEEP };

class StateManager {
public:
  StateManager();
  
  void begin();
  void checkModeSwitch();
  bool modeChanged();
  void checkStateSwitch();
  bool stateChanged();
  SystemMode getCurrentMode() const { return currentMode; }
  SystemMode getPreviousMode() const { return previousMode; }
  SystemState getCurrentState() const { return currentState; }
  SystemState getPreviousState() const { return previousState; }
  
private:
  // Mode tracking
  SystemMode currentMode;
  SystemMode previousMode;
  SystemState currentState;
  SystemState previousState;
  bool modeChangedFlag;
  bool stateChangedFlag;

  // Button handling
  int lastButtonState;
  int lastStableButtonState;
  unsigned long lastDebounceTime;
};

#endif