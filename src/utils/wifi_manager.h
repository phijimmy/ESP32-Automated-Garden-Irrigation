#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>

class WiFiManager {
private:
    String ssid;
    String password;
    String apSSID;
    String apPassword;
    bool isHotspotActive;
    unsigned long lastClientActivity;
    const unsigned long hotspotTimeout = 900000; // 15 minutes in milliseconds
    DNSServer dnsServer;
    bool setupMode;

public:
    WiFiManager();
    WiFiManager(const String &apSSID, const String &apPassword);
    bool begin();
    void startHotspot();
    void stopHotspot();
    bool connectToWiFi(const String &ssid, const String &password, int timeout = 20);
    void handleReconnection();
    bool isConnected();
    String getIPAddress();
    String getAPIPAddress();
    void checkHotspotTimeout();
    void resetClientActivityTimer();
    void processDNS();
    
    // Additional method for setup mode
    void setSetupMode(bool enabled);
    bool isInSetupMode();
    bool isHotspotRunning();
    // Add to WiFiManager class
    void setAPCredentials(const String &apSSID, const String &apPassword) {
    this->apSSID = apSSID;
    this->apPassword = apPassword;
}
};

#endif // WIFI_MANAGER_H