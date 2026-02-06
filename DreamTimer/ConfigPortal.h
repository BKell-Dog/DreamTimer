#ifndef CONFIG_PORTAL_H
#define CONFIG_PORTAL_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "ConfigStorage.h"

class ConfigPortal {
public:
  ConfigPortal(ConfigStorage* storage);
  
  // Start the configuration portal (AP mode + web server)
  void start();
  
  // Handle client requests (call in loop)
  void handleClient();
  
  // Stop the portal
  void stop();
  
  // Check if new configuration was saved
  bool configSaved();
  
  // Get the newly saved configuration
  DreamClockConfig getSavedConfig();
  
private:
  ConfigStorage* configStorage;
  WebServer* server;
  bool _configSaved;
  DreamClockConfig _savedConfig;
  
  // Web page handlers
  void handleRoot();
  void handleSave();
  void handleNotFound();
  
  // HTML pages
  String getConfigPage();
  String getSuccessPage();
};

#endif // CONFIG_PORTAL_H
