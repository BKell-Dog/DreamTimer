#ifndef DISPLAY_HELPER_H
#define DISPLAY_HELPER_H

#include <Arduino.h>
#include <TM1637TinyDisplay6.h>

class DisplayHelper {
public:
  DisplayHelper(TM1637TinyDisplay6* disp);
  
  // Display time in 12-hour format with leading zero suppression
  // Displays as HHMMSS (e.g., 123456 = 12:34:56)
  void displayTime12Hour(int hour24, int minute, int second);
  
  // Display timer with leading zero suppression
  // Displays as HHMMSS (e.g., 000012 = 12, 000503 = 503)
  void displayTimer(unsigned int hours, unsigned int minutes, unsigned int seconds);
  
  // Display a 6-digit number with leading zero suppression
  void displayNumber(unsigned long number);
  
  // Show "CONFIG" message or pattern
  void showConfigMode();
  
  // Clear display
  void clear();

private:
  TM1637TinyDisplay6* display;
  uint8_t digits[6];
  
  // Helper to set digits with leading zero suppression
  void setDigitsWithLeadingZeroSuppression(int d0, int d1, int d2, int d3, int d4, int d5);
};

#endif // DISPLAY_HELPER_H
