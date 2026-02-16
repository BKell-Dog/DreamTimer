#ifndef WIFI_HELPER_H
#define WIFI_HELPER_H

#include <Arduino.h>
#include <WiFi.h>
#include <time.h> // For calling NTP server
#include "HardwareConfig.h"

class WifiHelper {
public:
  WifiHelper();
  
  void wifiConnectBlocking();
  void wifiDisconnectSave();
  bool isConnected() const { return WiFi.status() == WL_CONNECTED; }
};

#endif