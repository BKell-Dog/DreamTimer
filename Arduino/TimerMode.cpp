#include "TimerMode.h"

// Forward Declarations
void updateDisplay();
void updateLEDs();
void LEDsOff();

TimerMode::TimerMode(StateManager& sm, DisplayHelper& dh)
  : stateManager(sm),
    displayHelper(dh),
    timerStartMillis(0),
    timerAccumulatedSeconds(0),
    lastDisplayUpdate(0) {
}

void TimerMode::activate() {
  Serial.println("[TIMER] Activating mode");
  
  // Reset timer state
  timerStartMillis = millis();
  timerAccumulatedSeconds = 0;
  lastDisplayUpdate = millis();
  
  // Update LEDs and display
  updateLEDs();
  updateDisplay();
}

void TimerMode::deactivate() {
  Serial.println("[TIMER] Deactivating mode");
  LEDsOff();
}

void TimerMode::tick() {  
  // Handle button press and state change.
  stateManager.checkStateSwitch();

  if (stateManager.stateChanged()) {
    // Reset timer
    resetTimer();
    
    Serial.printf("[TIMER] Toggled to %s, timer reset\n",
                  stateManager.getCurrentState() == SystemState::AWAKE ? "AWAKE" : "ASLEEP");
    
    // Update LEDs and display
    updateLEDs();
    updateDisplay(); 
  }
  
  // Update display every second
  if (millis() - lastDisplayUpdate >= DISPLAY_UPDATE_MS) {
    updateDisplay();
    lastDisplayUpdate = millis();
  }
}

void TimerMode::resetTimer() {
  timerAccumulatedSeconds = 0;
  timerStartMillis = millis();
}

void TimerMode::updateDisplay() {
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
                stateManager.getCurrentState() == SystemState::AWAKE ? "AWAKE" : "ASLEEP");
}

void TimerMode::updateLEDs() {
  // Yellow LED for AWAKE, Blue LED for ASLEEP
  if (stateManager.getCurrentState() == SystemState::AWAKE) {
    // Yellow ON, Blue OFF
    digitalWrite(WAKE_LED_PIN, HIGH);
    digitalWrite(SLEEP_LED_PIN, LOW); 
  } else {
    // Yellow OFF, Blue ON
    digitalWrite(WAKE_LED_PIN, LOW);
    digitalWrite(SLEEP_LED_PIN, HIGH); 
  }
}

void TimerMode::LEDsOff() {
  digitalWrite(WAKE_LED_PIN, LOW);
  digitalWrite(SLEEP_LED_PIN, LOW);
}