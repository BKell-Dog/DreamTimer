#ifndef WIFI_HELPER_H
#define WIFI_HELPER_H

#include <Arduino.h>
#include <WiFi.h>
#include <time.h> // For calling NTP server
#include "HardwareConfig.h"

class WifiHelper {
public:
  WifiHelper();
  
  // Connect using explicit credentials.
  void wifiConnect(const char* ssid, const char* password);

  /*
   * Connect using the hardcoded credentials from HardwareConfig.h.
   * Kept as a fallback for when no EEPROM config has been written yet.
   */
  void wifiConnect();
  
  void wifiDisconnectSave();
  bool isConnected() const { return WiFi.status() == WL_CONNECTED; }

private:
  void _connect(const char* ssid, const char* password);
};

#endif