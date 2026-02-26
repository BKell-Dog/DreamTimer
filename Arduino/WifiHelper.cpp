#include "WifiHelper.h"

WifiHelper::WifiHelper() {}

void WifiHelper::wifiConnect(const char* ssid, const char* password) {
  _connect(ssid, password);
}

void WifiHelper::wifiConnect() {
  _connect(WIFI_SSID, WIFI_PASSWORD);
}

void WifiHelper::_connect(const char* ssid, const char* password) {
  if (isConnected()) {
    Serial.println("[WiFi] Already connected");
    return;
  }

  Serial.printf("[WiFi] Connecting to '%s' ...\n", ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
}

void WifiHelper::wifiDisconnectSave() {
  if (isConnected()) {
    Serial.println("[WiFi] Disconnecting to save power (TIMER mode)");
  }
  WiFi.disconnect(true); // disconnect and erase credentials from driver to reduce radio activity
  WiFi.mode(WIFI_OFF);
}