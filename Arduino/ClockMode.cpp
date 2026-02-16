#include "ClockMode.h"

ClockMode::ClockMode(StateManager& sm, DisplayHelper& dh, WifiHelper& wh)
  : stateManager(sm),
    displayHelper(dh),
    wifiHelper(wh),
    lastSyncedMillis(0ULL),
    lastSyncedEpoch(0),
    lastNtpSyncAttempt(0),
    lastDisplayUpdate(0) {}

void ClockMode::activate() {
	Serial.println("[CLOCK] Activating mode");

	Serial.println("[CLOCK] Connecting Wi-Fi & NTP");
    wifiHelper.wifiConnectBlocking();
    if (wifiHelper.isConnected()) {
      configTzTime(TIMEZONE, NTP_SERVER);
      tryNTPSync(); // immediate sync attempt, records baseline if successful
      lastNtpSyncAttempt = millis();
    } else {
      Serial.println("[CLOCK] Wi-Fi not connected; will attempt periodic connect/sync.");
    }
}

void ClockMode::deactivate() {
	Serial.println("[CLOCK] Deactivating mode");

	if (wifiHelper.isConnected())
		wifiHelper.wifiDisconnectSave();
}

void ClockMode::tick() {
  unsigned long now = millis();

  // If disconnected, try to reconnect every 5s
  if (!wifiHelper.isConnected() && (now - lastNtpSyncAttempt >= 5000UL)) {
    Serial.println("[CLOCK] Attempting WiFi reconnect...");
    wifiHelper.wifiConnectBlocking();
    if (wifiHelper.isConnected()) {
      configTzTime(TIMEZONE, NTP_SERVER);
    }
    lastNtpSyncAttempt = now;
  }

  // If connected, try NTP sync every NTP_SYNC_INTERVAL_MS
  if (wifiHelper.isConnected() && (now - lastNtpSyncAttempt >= NTP_SYNC_INTERVAL_MS)) {
    Serial.println("[CLOCK] Periodic NTP sync attempt");
    tryNTPSync();
    lastNtpSyncAttempt = now;
  }

  // Update display every second
  if (now - lastDisplayUpdate >= DISPLAY_UPDATE_MS) {
    lastDisplayUpdate = now;
    updateDisplay();
  }
}

void ClockMode::updateDisplay() {
  time_t computed = getCurrentTime();
  if (computed == 0) {
    // No valid time yet — show dashes
    Serial.println("[CLOCK] No valid time baseline; showing ----");
    const char* dashes[] = { "------" };
    displayHelper.flashMessage(dashes, 1, millis(), 500);
    return;
  }

  struct tm tmInfo;
  // Convert computed epoch to local broken-down time respecting TZ (we used configTzTime earlier)
  // localtime_r expects the system timezone to be set; timegm/time zone handling is via configTzTime
  time_t t_for_localtime = computed;
  localtime_r(&t_for_localtime, &tmInfo);

  int h = tmInfo.tm_hour;
  int m = tmInfo.tm_min;
  int s = tmInfo.tm_sec;

  // Display in 12-hour format with leading zero suppression
  displayHelper.displayTime12Hour(h, m, s);
  
  // Determine AM/PM for logging
  const char* ampm = (h >= 12) ? "PM" : "AM";
  int h12 = h % 12;
  if (h12 == 0) h12 = 12;
  
  Serial.printf("[CLOCK] %d:%02d:%02d %s\n", h12, m, s, ampm);
}

// Compute current clock time using lastSyncedEpoch + elapsed seconds (millis-based)
time_t ClockMode::getCurrentTime() {
  if (lastSyncedEpoch == 0) {
    // No baseline yet — try to return system time() (may be wrong) or 0
    time_t t = time(nullptr);
    if (t >= 24 * 3600)
    	return t;
    return 0;
  }
  unsigned long long nowMillis = (unsigned long long)millis();
  unsigned long long elapsedMs = nowMillis - lastSyncedMillis;
  time_t computed = lastSyncedEpoch + (time_t)(elapsedMs / 1000ULL);
  return computed;
}

// Attempt a synchronous NTP sync and record baseline (lastSyncedEpoch + lastSyncedMillis)
void ClockMode::tryNTPSync() {
  Serial.print("[CLOCK] Trying to sync to NTP");
  time_t now = time(nullptr);
  unsigned long start = millis();
  // Wait briefly until time() becomes "reasonable" (SNTP background may update)
  while (now < 24 * 3600 && (millis() - start) < 8000) { // wait up to 8s
    delay(200);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println();

  if (now < 24 * 3600) {
    Serial.println("[NTP] Sync failed / no valid time received");
    // keep lastSyncedEpoch untouched so clock will continue from last known value
  } else {
    lastSyncedEpoch = now;
    lastSyncedMillis = (unsigned long long)millis();
    struct tm tmInfo;
    localtime_r(&now, &tmInfo);
    Serial.printf("[NTP] Synced: %04d-%02d-%02d %02d:%02d:%02d (recording baseline)\n",
                  tmInfo.tm_year + 1900, tmInfo.tm_mon + 1, tmInfo.tm_mday,
                  tmInfo.tm_hour, tmInfo.tm_min, tmInfo.tm_sec);
  }
}