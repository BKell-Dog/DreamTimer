#include "WifiHelper.h"

WifiHelper::WifiHelper() {}

void WifiHelper::wifiConnect(const char* ssid, const char* password) {
  _connect(ssid, password);
}

void WifiHelper::wifiConnect() {
  _connect(WIFI_SSID, WIFI_PASSWORD); // Variables from HardwareConfig.h
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

// =============================================================================
// Diagnostics
// =============================================================================

void WifiHelper::scanNetworks() {
  Serial.println("[WiFi] ---- Network Scan ------------------------------------------------");

  // Ensure the radio is on; a disconnected save() sets WIFI_OFF
  if (WiFi.getMode() == WIFI_OFF) {
    WiFi.mode(WIFI_STA);
  }

  Serial.println("[WiFi] Scanning for networks (this takes ~2-3s)...");
  int found = WiFi.scanNetworks(/*async=*/false, /*show_hidden=*/true);

  if (found == WIFI_SCAN_FAILED) {
    Serial.println("[WiFi] Scan FAILED — radio may not be ready");
    return;
  }

  if (found == 0) {
    Serial.println("[WiFi] No networks found");
    return;
  }

  Serial.printf("[WiFi] Found %d network(s):\n", found);
  Serial.println("[WiFi]  # | RSSI | Quality   | Ch | Enc          | SSID");
  Serial.println("[WiFi] ---+------+-----------+----+--------------+----------------------");

  for (int i = 0; i < found; i++) {
    int rssi        = WiFi.RSSI(i);
    int channel     = WiFi.channel(i);
    String ssid     = WiFi.SSID(i);
    bool hidden     = ssid.isEmpty();
    wifi_auth_mode_t auth = (wifi_auth_mode_t)WiFi.encryptionType(i);

    Serial.printf("[WiFi] %2d | %4d | %-9s | %2d | %-12s | %s\n",
                  i + 1,
                  rssi,
                  _rssiQuality(rssi),
                  channel,
                  _encryptionTypeName(auth),
                  hidden ? "<hidden>" : ssid.c_str());
  }

  // Free the scan results from the driver's memory
  WiFi.scanDelete();
  Serial.println("[WiFi] ---- End of Scan -------------------------------------------------");
}

void WifiHelper::printConnectionInfo() {
  Serial.println("[WiFi] ---- Connection Info --------------------------------------------");

  if (!isConnected()) {
    Serial.printf("[WiFi] Not connected  (status = %d)\n", (int)WiFi.status());
    Serial.println("[WiFi] ---------------------------------------------------------------");
    return;
  }

  int rssi = WiFi.RSSI();

  Serial.printf("[WiFi]  SSID        : %s\n",   WiFi.SSID().c_str());
  Serial.printf("[WiFi]  BSSID (AP)  : %s\n",   WiFi.BSSIDstr().c_str());
  Serial.printf("[WiFi]  Channel     : %d (2.4 GHz)\n", WiFi.channel());
  Serial.printf("[WiFi]  RSSI        : %d dBm  (%s)\n", rssi, _rssiQuality(rssi));
  Serial.println("[WiFi]  ---");
  Serial.printf("[WiFi]  IP Address  : %s\n",   WiFi.localIP().toString().c_str());
  Serial.printf("[WiFi]  Subnet Mask : %s\n",   WiFi.subnetMask().toString().c_str());
  Serial.printf("[WiFi]  Gateway     : %s\n",   WiFi.gatewayIP().toString().c_str());
  Serial.printf("[WiFi]  DNS (0)     : %s\n",   WiFi.dnsIP(0).toString().c_str());
  Serial.printf("[WiFi]  DNS (1)     : %s\n",   WiFi.dnsIP(1).toString().c_str());
  Serial.println("[WiFi]  ---");
  Serial.printf("[WiFi]  MAC Address : %s\n",   WiFi.macAddress().c_str());
  Serial.println("[WiFi] ---- End of Connection Info ------------------------------------");
}

void WifiHelper::printDiagnostics() {
  Serial.println("[WiFi] ======== FULL DIAGNOSTICS ======================================");
  printConnectionInfo();
  scanNetworks();
  Serial.println("[WiFi] ======== END DIAGNOSTICS =======================================");
}

// =============================================================================
// Private helpers
// =============================================================================

const char* WifiHelper::_rssiQuality(int rssi) {
  // Thresholds loosely follow common 802.11 signal quality guidance
  if (rssi >= -50) return "Excellent";
  if (rssi >= -60) return "Good";
  if (rssi >= -70) return "Fair";
  if (rssi >= -80) return "Weak";
  return "Very Weak";
}

const char* WifiHelper::_encryptionTypeName(wifi_auth_mode_t authMode) {
  switch (authMode) {
    case WIFI_AUTH_OPEN:           return "Open";
    case WIFI_AUTH_WEP:            return "WEP";
    case WIFI_AUTH_WPA_PSK:        return "WPA-PSK";
    case WIFI_AUTH_WPA2_PSK:       return "WPA2-PSK";
    case WIFI_AUTH_WPA_WPA2_PSK:   return "WPA/WPA2-PSK";
    case WIFI_AUTH_WPA2_ENTERPRISE:return "WPA2-Ent";
    case WIFI_AUTH_WPA3_PSK:       return "WPA3-PSK";
    case WIFI_AUTH_WPA2_WPA3_PSK:  return "WPA2/WPA3";
    default:                       return "Unknown";
  }
}