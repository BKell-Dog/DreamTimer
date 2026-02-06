#include "ConfigPortal.h"

ConfigPortal::ConfigPortal(ConfigStorage* storage) 
  : configStorage(storage), server(nullptr), _configSaved(false) {}

void ConfigPortal::start() {
  Serial.println("[CONFIG PORTAL] Starting Access Point...");
  
  // Start AP mode
  WiFi.mode(WIFI_AP);
  WiFi.softAP("DREAM_Clock_Setup");
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("[CONFIG PORTAL] AP Started. Connect to: DREAM_Clock_Setup");
  Serial.print(" and go to: http://");
  Serial.println(IP);
  
  // Create web server
  server = new WebServer(80);
  
  // Setup routes
  server->on("/", [this]() { this->handleRoot(); });
  server->on("/save", HTTP_POST, [this]() { this->handleSave(); });
  server->onNotFound([this]() { this->handleNotFound(); });
  
  server->begin();
  Serial.println("[CONFIG PORTAL] Web server started");
}

void ConfigPortal::handleClient() {
  if (server) {
    server->handleClient();
  }
}

void ConfigPortal::stop() {
  if (server) {
    server->stop();
    delete server;
    server = nullptr;
  }
  WiFi.softAPdisconnect(true);
  Serial.println("[CONFIG PORTAL] Stopped");
}

bool ConfigPortal::configSaved() {
  return _configSaved;
}

DreamClockConfig ConfigPortal::getSavedConfig() {
  return _savedConfig;
}

void ConfigPortal::handleRoot() {
  server->send(200, "text/html", getConfigPage());
}

void ConfigPortal::handleSave() {
  // Get form data
  _savedConfig.ssid = server->arg("ssid");
  _savedConfig.password = server->arg("password");
  _savedConfig.timezone = server->arg("timezone");
  
  // Save to EEPROM
  configStorage->saveConfig(_savedConfig);
  _configSaved = true;
  
  // Send success page
  server->send(200, "text/html", getSuccessPage());
  
  Serial.println("[CONFIG PORTAL] Configuration saved, device will restart in 3 seconds...");
}

void ConfigPortal::handleNotFound() {
  // Redirect to root
  server->sendHeader("Location", "/", true);
  server->send(302, "text/plain", "");
}

String ConfigPortal::getConfigPage() {
  return R"(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>DREAM Clock Setup</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Arial, sans-serif;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 20px;
    }
    .container {
      background: white;
      border-radius: 20px;
      box-shadow: 0 20px 60px rgba(0,0,0,0.3);
      max-width: 500px;
      width: 100%;
      padding: 40px;
    }
    h1 {
      color: #333;
      font-size: 28px;
      margin-bottom: 10px;
      text-align: center;
    }
    .subtitle {
      color: #666;
      text-align: center;
      margin-bottom: 30px;
      font-size: 14px;
    }
    label {
      display: block;
      color: #333;
      font-weight: 600;
      margin-bottom: 8px;
      font-size: 14px;
    }
    input, select {
      width: 100%;
      padding: 12px 15px;
      margin-bottom: 20px;
      border: 2px solid #e0e0e0;
      border-radius: 10px;
      font-size: 16px;
      transition: border-color 0.3s;
    }
    input:focus, select:focus {
      outline: none;
      border-color: #667eea;
    }
    button {
      width: 100%;
      padding: 15px;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
      border: none;
      border-radius: 10px;
      font-size: 18px;
      font-weight: 600;
      cursor: pointer;
      transition: transform 0.2s, box-shadow 0.2s;
    }
    button:hover {
      transform: translateY(-2px);
      box-shadow: 0 10px 20px rgba(102, 126, 234, 0.4);
    }
    button:active {
      transform: translateY(0);
    }
    .info {
      background: #f0f4ff;
      border-left: 4px solid #667eea;
      padding: 12px 15px;
      margin-bottom: 20px;
      border-radius: 5px;
      font-size: 13px;
      color: #555;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>⏰ DREAM Clock Setup</h1>
    <p class="subtitle">Configure your sleep tracking clock</p>
    
    <div class="info">
      💡 Enter your WiFi credentials and timezone to get started
    </div>
    
    <form action="/save" method="POST">
      <label for="ssid">WiFi Network Name (SSID)</label>
      <input type="text" id="ssid" name="ssid" required placeholder="Enter your WiFi name">
      
      <label for="password">WiFi Password</label>
      <input type="password" id="password" name="password" required placeholder="Enter your WiFi password">
      
      <label for="timezone">Timezone</label>
      <select id="timezone" name="timezone" required>
        <option value="EST5EDT,M3.2.0/2,M11.1.0/2">Eastern Time (US)</option>
        <option value="CST6CDT,M3.2.0/2,M11.1.0/2">Central Time (US)</option>
        <option value="MST7MDT,M3.2.0/2,M11.1.0/2">Mountain Time (US)</option>
        <option value="PST8PDT,M3.2.0/2,M11.1.0/2">Pacific Time (US)</option>
        <option value="AKST9AKDT,M3.2.0/2,M11.1.0/2">Alaska Time (US)</option>
        <option value="HST10">Hawaii Time (US)</option>
        <option value="GMT0BST,M3.5.0/1,M10.5.0">London (UK)</option>
        <option value="CET-1CEST,M3.5.0,M10.5.0/3">Central Europe</option>
        <option value="EET-2EEST,M3.5.0/3,M10.5.0/4">Eastern Europe</option>
        <option value="JST-9">Japan</option>
        <option value="AEST-10AEDT,M10.1.0,M4.1.0/3">Sydney (Australia)</option>
      </select>
      
      <button type="submit">💾 Save Configuration</button>
    </form>
  </div>
</body>
</html>
)";
}

String ConfigPortal::getSuccessPage() {
  return R"(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Setup Complete</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Arial, sans-serif;
      background: linear-gradient(135deg, #11998e 0%, #38ef7d 100%);
      min-height: 100vh;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 20px;
    }
    .container {
      background: white;
      border-radius: 20px;
      box-shadow: 0 20px 60px rgba(0,0,0,0.3);
      max-width: 500px;
      width: 100%;
      padding: 40px;
      text-align: center;
    }
    .checkmark {
      font-size: 80px;
      margin-bottom: 20px;
      animation: pop 0.5s ease-out;
    }
    @keyframes pop {
      0% { transform: scale(0); }
      50% { transform: scale(1.2); }
      100% { transform: scale(1); }
    }
    h1 {
      color: #333;
      font-size: 28px;
      margin-bottom: 15px;
    }
    p {
      color: #666;
      line-height: 1.6;
      margin-bottom: 10px;
    }
    .countdown {
      font-size: 48px;
      color: #11998e;
      font-weight: bold;
      margin: 20px 0;
    }
  </style>
  <script>
    let seconds = 3;
    setInterval(() => {
      document.getElementById('countdown').textContent = seconds;
      if (seconds < 0) {
        seconds--;
        document.getElementById('message').textContent = 'Restarting now...';
      }
    }, 1000);
  </script>
</head>
<body>
  <div class="container">
    <div class="checkmark">✅</div>
    <h1>Configuration Saved!</h1>
    <p>Your DREAM Clock is now configured.</p>
    <p id="message">Device will restart in:</p>
    <div class="countdown" id="countdown">3</div>
    <p style="font-size: 12px; color: #999; margin-top: 20px;">
      Please reconnect to your WiFi network
    </p>
  </div>
</body>
</html>
)";
}
