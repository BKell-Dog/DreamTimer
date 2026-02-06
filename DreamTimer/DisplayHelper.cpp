#include "DisplayHelper.h"

DisplayHelper::DisplayHelper(TM1637TinyDisplay6* disp) : display(disp) {}

void DisplayHelper::displayTime12Hour(int hour24, int minute, int second) {
  // Convert 24-hour to 12-hour format
  int hour12 = hour24 % 12;
  if (hour12 == 0) hour12 = 12;  // Midnight/noon = 12
  
  // Extract digits: HHMMSS
  int d0 = (hour12 / 10) % 10;
  int d1 = hour12 % 10;
  int d2 = (minute / 10) % 10;
  int d3 = minute % 10;
  int d4 = (second / 10) % 10;
  int d5 = second % 10;
  
  setDigitsWithLeadingZeroSuppression(d0, d1, d2, d3, d4, d5);
}

void DisplayHelper::displayTimer(unsigned int hours, unsigned int minutes, unsigned int seconds) {
  // Extract digits: HHMMSS
  int d0 = (hours / 10) % 10;
  int d1 = hours % 10;
  int d2 = (minutes / 10) % 10;
  int d3 = minutes % 10;
  int d4 = (seconds / 10) % 10;
  int d5 = seconds % 10;
  
  setDigitsWithLeadingZeroSuppression(d0, d1, d2, d3, d4, d5);
}

void DisplayHelper::displayNumber(unsigned long number) {
  // Extract 6 digits
  int d5 = number % 10;
  int d4 = (number / 10) % 10;
  int d3 = (number / 100) % 10;
  int d2 = (number / 1000) % 10;
  int d1 = (number / 10000) % 10;
  int d0 = (number / 100000) % 10;
  
  setDigitsWithLeadingZeroSuppression(d0, d1, d2, d3, d4, d5);
}

void DisplayHelper::showConfigMode() {
  // Display "CONFIG" pattern - could be animated or static
  // For now, show "------" (dashes)
  for (int i = 0; i < 6; i++) {
    digits[i] = 0b01000000;  // Middle segment (dash)
  }
  display->setSegments(digits, 6, 0);
}

void DisplayHelper::flashMessage(const char* messages[], int messageCount, unsigned long currentMillis, unsigned long flashInterval) {
  // Calculate which message to show based on time
  unsigned long cycleTime = currentMillis / flashInterval;
  int messageIndex = cycleTime % messageCount;
  
  // Display the current message
  displayString(messages[messageIndex]);
}

void DisplayHelper::clear() {
  display->clear();
}

void DisplayHelper::setDigitsWithLeadingZeroSuppression(int d0, int d1, int d2, int d3, int d4, int d5) {
  // Find first non-zero digit
  bool foundNonZero = false;
  
  // Digit 0 (leftmost)
  if (d0 != 0) {
    digits[0] = display->encodeDigit(d0);
    foundNonZero = true;
  } else {
    digits[0] = 0x00;  // Blank
  }
  
  // Digit 1
  if (foundNonZero || d1 != 0) {
    digits[1] = display->encodeDigit(d1);
    foundNonZero = true;
  } else {
    digits[1] = 0x00;  // Blank
  }
  
  // Digit 2
  if (foundNonZero || d2 != 0) {
    digits[2] = display->encodeDigit(d2);
    foundNonZero = true;
  } else {
    digits[2] = 0x00;  // Blank
  }
  
  // Digit 3
  if (foundNonZero || d3 != 0) {
    digits[3] = display->encodeDigit(d3);
    foundNonZero = true;
  } else {
    digits[3] = 0x00;  // Blank
  }
  
  // Digit 4
  if (foundNonZero || d4 != 0) {
    digits[4] = display->encodeDigit(d4);
    foundNonZero = true;
  } else {
    digits[4] = 0x00;  // Blank
  }
  
  // Digit 5 (rightmost) - always show, even if 0
  digits[5] = display->encodeDigit(d5);
  
  display->setSegments(digits, 6, 0);
}

void DisplayHelper::displayString(const char* str) {
  // Display a string on the 6-digit display
  // Converts characters to 7-segment encoding
  
  for (int i = 0; i < 6; i++) {
    if (str[i] == '\0') {
      // End of string - blank remaining digits
      digits[i] = 0x00;
    } else {
      char c = str[i];
      
      // Convert character to segment encoding
      if (c >= '0' && c <= '9') {
        digits[i] = display->encodeDigit(c - '0');
      } else if (c == '-') {
        digits[i] = 0b01000000;  // Dash
      } else if (c == ' ') {
        digits[i] = 0x00;  // Blank
      } else {
        // Letters - basic 7-segment encoding
        switch (c) {
          case 'A': case 'a': digits[i] = 0b01110111; break;
          case 'B': case 'b': digits[i] = 0b01111100; break;
          case 'C': case 'c': digits[i] = 0b00111001; break;
          case 'D': case 'd': digits[i] = 0b01011110; break;
          case 'E': case 'e': digits[i] = 0b01111001; break;
          case 'F': case 'f': digits[i] = 0b01110001; break;
          case 'G': case 'g': digits[i] = 0b00111101; break;
          case 'H': case 'h': digits[i] = 0b01110110; break;
          case 'I': case 'i': digits[i] = 0b00110000; break;
          case 'J': case 'j': digits[i] = 0b00011110; break;
          case 'L': case 'l': digits[i] = 0b00111000; break;
          case 'M': case 'm': digits[i] = 0b01010101; break;
          case 'N': case 'n': digits[i] = 0b01010100; break;
          case 'O': case 'o': digits[i] = 0b00111111; break;
          case 'P': case 'p': digits[i] = 0b01110011; break;
          case 'R': case 'r': digits[i] = 0b01010000; break;
          case 'S': case 's': digits[i] = 0b01101101; break;
          case 'T': case 't': digits[i] = 0b01111000; break;
          case 'U': case 'u': digits[i] = 0b00111110; break;
          case 'V': case 'v': digits[i] = 0b00011100; break;
          case 'W': case 'w': digits[i] = 0b00101010; break;
          case 'Y': case 'y': digits[i] = 0b01101110; break;
          default: digits[i] = 0x00; break;  // Unknown char = blank
        }
      }
    }
  }
  
  display->setSegments(digits, 6, 0);
}