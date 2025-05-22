#include "wifi_manager.h"
#include <Arduino.h>

WiFiManager::WiFiManager(const String &apSSID, const String &apPassword) {
    this->apSSID = apSSID;
    this->apPassword = apPassword;
    this->isHotspotActive = false;
    this->lastClientActivity = 0;
    this->setupMode = false;
    this->ssid = "";
    this->password = "";
}

WiFiManager::WiFiManager() {
    this->apSSID = "GartenIrrigationSystem";
    this->apPassword = "gardening123";
    this->isHotspotActive = false;
    this->lastClientActivity = 0;
    this->setupMode = false;
    this->ssid = "";
    this->password = "";
}

bool WiFiManager::begin() {
    // First explicitly turn WiFi off to clear any previous state
    WiFi.mode(WIFI_OFF);
    delay(100);
    
    // Initialize in station mode, but disconnected
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true); // true = also disconnect from and forget any AP
    delay(100);
    
    Serial.println("WiFi initialized in disconnected state");
    return true;
}

void WiFiManager::startHotspot() {
    // Check if already active
    if (isHotspotActive) {
        Serial.println("Hotspot already active, skipping startup");
        return;
    }
    
    Serial.println("Starting WiFi hotspot...");
    
    // Completely reset WiFi state
    WiFi.mode(WIFI_OFF);
    delay(100);
    WiFi.disconnect(true);
    delay(100);
    
    // Set up access point with credentials from class members
    WiFi.mode(WIFI_AP);
    
    // Make sure SSID is valid
    if (apSSID.length() == 0) {
        apSSID = "GartenIrrigationSystem";  // Default fallback
        Serial.println("WARNING: Empty AP SSID detected, using default");
    }
    
    // Start the access point
    bool result = WiFi.softAP(apSSID.c_str(), apPassword.c_str());
    
    if (!result) {
        Serial.println("ERROR: Failed to start access point!");
        Serial.print("SSID: ");
        Serial.println(apSSID);
        Serial.print("Password length: ");
        Serial.println(apPassword.length());
        return;
    }
    
    // Start DNS server for captive portal
    IPAddress apIP = WiFi.softAPIP();
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(53, "*", apIP);
    
    // Set activity flag and timestamp
    isHotspotActive = true;
    lastClientActivity = millis();
    
    Serial.println("Hotspot started successfully");
    Serial.print("AP SSID: ");
    Serial.println(apSSID);
    Serial.print("AP IP address: ");
    Serial.println(apIP);
}

void WiFiManager::stopHotspot() {
    if (isHotspotActive) {
        Serial.println("Stopping hotspot...");
        dnsServer.stop();
        WiFi.softAPdisconnect(true);
        isHotspotActive = false;
        
        // Return to disconnected station mode
        WiFi.mode(WIFI_OFF);
        delay(100);
        WiFi.mode(WIFI_STA);
        WiFi.disconnect(true);
        
        Serial.println("Hotspot stopped");
    }
}

bool WiFiManager::connectToWiFi(const String &ssid, const String &password, int timeout) {
    // Only proceed if we have valid SSID
    if (ssid.length() == 0) {
        Serial.println("Cannot connect: SSID is empty");
        return false;
    }
    
    this->ssid = ssid;
    this->password = password;
    
    Serial.print("Connecting to WiFi network: ");
    Serial.println(ssid);
    
    WiFi.disconnect(true);
    delay(100);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    Serial.print("Connecting to WiFi");
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < timeout) {
        delay(1000);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println("IP address: " + getIPAddress());
        return true;
    } else {
        Serial.println("");
        Serial.println("WiFi connection failed");
        // Ensure we're properly disconnected
        WiFi.disconnect(true);
        return false;
    }
}

void WiFiManager::handleReconnection() {
    // Only attempt reconnection if we have a valid SSID
    if (!isHotspotActive && WiFi.status() != WL_CONNECTED && ssid.length() > 0) {
        Serial.println("WiFi disconnected, attempting to reconnect to: " + ssid);
        WiFi.begin(ssid.c_str(), password.c_str());
    }
}

bool WiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

String WiFiManager::getIPAddress() {
    return WiFi.localIP().toString();
}

String WiFiManager::getAPIPAddress() {
    return WiFi.softAPIP().toString();
}

void WiFiManager::checkHotspotTimeout() {
    if (isHotspotActive && !setupMode) {
        if (millis() - lastClientActivity > hotspotTimeout) {
            Serial.println("Hotspot timeout reached, shutting down hotspot");
            stopHotspot();
        }
    }
}

void WiFiManager::resetClientActivityTimer() {
    lastClientActivity = millis();
    Serial.println("Client activity detected, resetting timeout timer");
}

void WiFiManager::processDNS() {
    if (isHotspotActive) {
        dnsServer.processNextRequest();
    }
}

void WiFiManager::setSetupMode(bool enabled) {
    setupMode = enabled;
    
    if (enabled) {
        // When entering setup mode, ensure we clear any previous WiFi state
        WiFi.mode(WIFI_OFF);
        delay(100);
        WiFi.disconnect(true);
        delay(100);
        Serial.println("Entered setup mode");
    } else {
        Serial.println("Exited setup mode");
    }
}

bool WiFiManager::isInSetupMode() {
    return setupMode;
}

bool WiFiManager::isHotspotRunning() {
    return isHotspotActive;
}