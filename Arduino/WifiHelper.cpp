#include "WifiHelper.h"

WifiHelper::WifiHelper() {}

void WifiHelper::wifiConnectBlocking(const char* ssid, const char* password) {
  _connect(ssid, password);
}

void WifiHelper::wifiConnectBlocking() {
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

  unsigned long start = millis();
  while (!isConnected() && (millis() - start) < WIFI_CONNECT_TIMEOUT_MS) {
    delay(200);
    Serial.print(".");
  }
  Serial.println();

  if (isConnected()) {
    Serial.print("[WiFi] Connected, local IP=");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("[WiFi] Failed to connect (timeout)");
    wifiDisconnectSave();
  }
}

void WifiHelper::wifiDisconnectSave() {
  if (isConnected()) {
    Serial.println("[WiFi] Disconnecting to save power (TIMER mode)");
  }
  WiFi.disconnect(true); // disconnect and erase credentials from driver to reduce radio activity
  WiFi.mode(WIFI_OFF);
}