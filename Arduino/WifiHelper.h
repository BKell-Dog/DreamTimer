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

  // --- Diagnostic Helper Functions ---
  /*
    * Performs an active scan and prints every visible network with its
    * SSID, RSSI (dBm), channel, and encryption type.  Blocks for the
    * duration of the scan (~2–3 s).  Safe to call whether or not the
    * ESP32 is currently connected.
    */
  void scanNetworks();

  /*
    * Prints detailed information about the current connection:
    *   - SSID / BSSID of the AP we are associated with
    *   - Channel and band
    *   - RSSI with a human-readable quality label
    *   - Assigned IP, subnet mask, gateway, and DNS servers
    *   - MAC address of this station
    * Logs a warning and returns early if not connected.
    */
  void printConnectionInfo();

  /*
   * Convenience wrapper: prints connection info (if connected) and then
   * runs a full network scan.  One call to get the full picture.
   */
  void printDiagnostics();

private:
  void _connect(const char* ssid, const char* password);

  // --- Diagnostics ---

  // Returns a short human-readable label for a given RSSI value.
  static const char* _rssiQuality(int rssi);

  // Returns a short string naming the encryption/auth type.
  static const char* _encryptionTypeName(wifi_auth_mode_t authMode);
};

#endif