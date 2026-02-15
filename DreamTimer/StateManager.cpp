#include "StateManager.h"

StateManager::StateManager() 
  : currentMode(SystemMode::CLOCK),
    previousMode(SystemMode::CLOCK),
    lastButtonState(HIGH),
    lastStableButtonState(HIGH),
    lastDebounceTime(0),
    modeChangedFlag(false) {
}

void StateManager::begin() {
  // Initialize pins
  pinMode(MODE_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void StateManager::checkModeSwitch() {
  int modePin = digitalRead(MODE_PIN);
  SystemMode desired = (modePin == HIGH) ? SystemMode::CLOCK : SystemMode::TIMER;
  
  if (desired != currentMode) {
    Serial.printf("[SYSTEM] Switching to %s mode\n", 
                  (desired == SystemMode::CLOCK) ? "CLOCK" : "TIMER");
    previousMode = currentMode;
    currentMode = desired;
    modeChangedFlag = true;
  }
}

void StateManager::checkStateSwitch() {
  int readValue = digitalRead(BUTTON_PIN);
  
  // Reset debounce timer if reading changed
  if (readValue != lastButtonState) {
    lastDebounceTime = millis();
  }

  // Check if reading has been stable long enough
  if ((millis() - lastDebounceTime) > BUTTON_DEBOUNCE_MS) {
    
    // If the stable reading is different from our last STABLE state
    if (readValue != lastStableButtonState) {
      
      // Update stable state
      lastStableButtonState = readValue;
      
      // Only trigger on button press (HIGH -> LOW due to pull-up)
      if (readValue == LOW) {
        previousState = currentState;
        currentState = (currentState == SystemState::AWAKE) 
                       ? SystemState::ASLEEP 
                       : SystemState::AWAKE;
        
        stateChangedFlag = true;
        
        Serial.printf("[SYSTEM] Switching to %s state\n", (currentState == SystemState::AWAKE) ? "AWAKE" : "ASLEEP");
      }
    }
  }
  
  lastButtonState = readValue;
  // See example debouncing logic here: https://docs.arduino.cc/built-in-examples/digital/Debounce/
}

bool StateManager::modeChanged() {
  bool changed = modeChangedFlag;
  modeChangedFlag = false;
  return changed;
}

bool StateManager::stateChanged() {
  bool changed = stateChangedFlag;
  stateChangedFlag = false;
  return changed;
}