#include "WifiHelper.h"

WifiHelper::WifiHelper() {}

void WifiHelper::wifiConnectBlocking() {
  if (isConnected()) {
    Serial.println("[WiFi] Already connected");
    return;
  }

  Serial.printf("[WiFi] Connecting to '%s' ...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

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
    // disable Wi-Fi to save power if not connected
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