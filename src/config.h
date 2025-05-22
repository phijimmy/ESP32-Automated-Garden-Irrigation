#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

class Config {
private:
    // Flag to determine if this is first run
    bool isFirstRun = true;
    
    // Device settings
    String deviceName = "GartenIrrigationSystem";
    String username = "user";
    String password = "pass";
    int touchSensorThreshold = 40;
    int soilMoisturePin = 36;
    int soilMoisturePowerPin = 27;
    int touchPin = 4;
    int i2cSdaPin = 21;
    int i2cSclPin = 22;
    int relay1Pin = 25;
    int relay2Pin = 26;
    int relay3Pin = 32;
    int relay4Pin = 33;
    String relay1Name = "Aux";
    String relay2Name = "Pump 1";
    String relay3Name = "Pump 2";
    String relay4Name = "Sensor";
    int soilMoistureDry = 2350;
    int soilMoistureWet = 815;
    int wateringDuration = 60;
    int wateringStartHour = 8;
    int wateringStartMinute = 0;
    int wateringEndHour = 9;
    int wateringEndMinute = 0;
    float soilMoistureThreshold = 50.0;
    const char* configFile = "/config.json";
    
public:
    // Initialize configuration system
    bool begin() {
        // Initialize filesystem
        if (!LittleFS.begin(true)) {
            Serial.println("Failed to mount filesystem");
            return false;
        }
        
        // Check if config file exists
        if (LittleFS.exists(configFile)) {
            if (loadConfig()) {
                Serial.println("Configuration loaded successfully");
                return true;
            }
        }
        
        // If we reach here, either file doesn't exist or couldn't be loaded
        Serial.println("No valid configuration found, using defaults");
        return true;
    }
    
    // Save configuration to filesystem
    bool saveConfig() {
        DynamicJsonDocument doc(2048);
        
        // CRITICAL: Save the first-time setup flag
        doc["isFirstRun"] = isFirstRun;
        
        // Add all settings to JSON document
        doc["deviceName"] = deviceName;
        doc["username"] = username;
        doc["password"] = password;
        
        // Pin assignments
        doc["soilMoisturePin"] = soilMoisturePin;
        doc["soilMoisturePowerPin"] = soilMoisturePowerPin;
        doc["touchPin"] = touchPin;
        doc["i2cSdaPin"] = i2cSdaPin;
        doc["i2cSclPin"] = i2cSclPin;
        doc["relay1Pin"] = relay1Pin;
        doc["relay2Pin"] = relay2Pin;
        doc["relay3Pin"] = relay3Pin;
        doc["relay4Pin"] = relay4Pin;
        
        // Relay names
        doc["relay1Name"] = relay1Name;
        doc["relay2Name"] = relay2Name;
        doc["relay3Name"] = relay3Name;
        doc["relay4Name"] = relay4Name;
        
        // Soil moisture calibration
        doc["soilMoistureDry"] = soilMoistureDry;
        doc["soilMoistureWet"] = soilMoistureWet;
        
        // Watering settings
        doc["wateringDuration"] = wateringDuration;
        doc["wateringStartHour"] = wateringStartHour;
        doc["wateringStartMinute"] = wateringStartMinute;
        doc["wateringEndHour"] = wateringEndHour;
        doc["wateringEndMinute"] = wateringEndMinute;
        doc["soilMoistureThreshold"] = soilMoistureThreshold;
        
        // Open file for writing
        File file = LittleFS.open(configFile, "w");
        if (!file) {
            Serial.println("Failed to create config file");
            return false;
        }
        
        // Serialize JSON to file
        if (serializeJson(doc, file) == 0) {
            Serial.println("Failed to write config file");
            file.close();
            return false;
        }
        
        file.close();
        Serial.println("Configuration saved successfully");
        return true;
    }
    
    // Load configuration from filesystem
    bool loadConfig() {
        File file = LittleFS.open(configFile, "r");
        if (!file) {
            Serial.println("No config file found");
            return false;
        }
        
        // Parse JSON
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, file);
        file.close();
        
        if (error) {
            Serial.println("Failed to parse config file");
            return false;
        }
        
        // CRITICAL: Load the first-time setup flag
        isFirstRun = doc["isFirstRun"] | isFirstRun;
        
        // Load all settings from JSON
        deviceName = doc["deviceName"] | deviceName;
        username = doc["username"] | username;
        password = doc["password"] | password;
        
        // Pin assignments
        soilMoisturePin = doc["soilMoisturePin"] | soilMoisturePin;
        soilMoisturePowerPin = doc["soilMoisturePowerPin"] | soilMoisturePowerPin;
        touchPin = doc["touchPin"] | touchPin;
        i2cSdaPin = doc["i2cSdaPin"] | i2cSdaPin;
        i2cSclPin = doc["i2cSclPin"] | i2cSclPin;
        relay1Pin = doc["relay1Pin"] | relay1Pin;
        relay2Pin = doc["relay2Pin"] | relay2Pin;
        relay3Pin = doc["relay3Pin"] | relay3Pin;
        relay4Pin = doc["relay4Pin"] | relay4Pin;
        
        // Relay names
        relay1Name = doc["relay1Name"] | relay1Name;
        relay2Name = doc["relay2Name"] | relay2Name;
        relay3Name = doc["relay3Name"] | relay3Name;
        relay4Name = doc["relay4Name"] | relay4Name;
        
        // Soil moisture calibration
        soilMoistureDry = doc["soilMoistureDry"] | soilMoistureDry;
        soilMoistureWet = doc["soilMoistureWet"] | soilMoistureWet;
        
        // Watering settings
        wateringDuration = doc["wateringDuration"] | wateringDuration;
        
        // Support both old and new format for backward compatibility
        if (doc.containsKey("wateringTimeStart")) {
            // Old format
            wateringStartHour = doc["wateringTimeStart"] | wateringStartHour;
            wateringStartMinute = 0;
        } else {
            // New format
            wateringStartHour = doc["wateringStartHour"] | wateringStartHour;
            wateringStartMinute = doc["wateringStartMinute"] | wateringStartMinute;
        }
        
        if (doc.containsKey("wateringTimeEnd")) {
            // Old format
            wateringEndHour = doc["wateringTimeEnd"] | wateringEndHour;
            wateringEndMinute = 0;
        } else {
            // New format
            wateringEndHour = doc["wateringEndHour"] | wateringEndHour;
            wateringEndMinute = doc["wateringEndMinute"] | wateringEndMinute;
        }
        
        if (doc.containsKey("waterAtPercent")) {
            // Old name
            soilMoistureThreshold = doc["waterAtPercent"] | soilMoistureThreshold;
        } else {
            // New name
            soilMoistureThreshold = doc["soilMoistureThreshold"] | soilMoistureThreshold;
        }
        
        return true;
    }
    
    // Reset to default settings and mark as first run
    void resetToDefaults() {
        LittleFS.remove(configFile);
        isFirstRun = true;
        
        // Reset all values to defaults
        deviceName = "GartenIrrigationSystem";
        username = "user";
        password = "pass";
        soilMoisturePin = 36;
        soilMoisturePowerPin = 27;
        touchPin = 4;
        i2cSdaPin = 21;
        i2cSclPin = 22;
        relay1Pin = 25;
        relay2Pin = 26;
        relay3Pin = 32;
        relay4Pin = 33;
        relay1Name = "Aux";
        relay2Name = "Pump 1";
        relay3Name = "Pump 2";
        relay4Name = "Sensor";
        soilMoistureDry = 2350;
        soilMoistureWet = 815;
        wateringDuration = 60;
        wateringStartHour = 8;
        wateringStartMinute = 0;
        wateringEndHour = 9;
        wateringEndMinute = 0;
        soilMoistureThreshold = 50.0;
    }
    
    // Getters and setters for all configuration parameters
    bool isFirstTimeSetup() { return isFirstRun; }
    void setFirstTimeSetup(bool value) { isFirstRun = value; }
    
    // Device name
    String getDeviceName() { return deviceName; }
    void setDeviceName(const String &value) { deviceName = value; }
    
    // Authentication
    String getUsername() { return username; }
    void setUsername(const String &value) { username = value; }
    String getPassword() { return password; }
    void setPassword(const String &value) { password = value; }
    
    // Pin assignments
    int getSoilMoistureSensorPin() { return soilMoisturePin; }
    void setSoilMoistureSensorPin(int value) { soilMoisturePin = value; }
    int getSoilMoisturePowerPin() { return soilMoisturePowerPin; }
    void setSoilMoisturePowerPin(int value) { soilMoisturePowerPin = value; }
    int getTouchSensorPin() { return touchPin; }
    void setTouchSensorPin(int value) { touchPin = value; }
    int getTouchSensorThreshold() { return touchSensorThreshold; }
    void setTouchSensorThreshold(int value) { touchSensorThreshold = value; }
    int getI2cSdaPin() { return i2cSdaPin; }
    void setI2cSdaPin(int value) { i2cSdaPin = value; }
    int getI2cSclPin() { return i2cSclPin; }
    void setI2cSclPin(int value) { i2cSclPin = value; }
    
    // Relay pins
    int getRelay1Pin() { return relay1Pin; }
    void setRelay1Pin(int value) { relay1Pin = value; }
    int getRelay2Pin() { return relay2Pin; }
    void setRelay2Pin(int value) { relay2Pin = value; }
    int getRelay3Pin() { return relay3Pin; }
    void setRelay3Pin(int value) { relay3Pin = value; }
    int getRelay4Pin() { return relay4Pin; }
    void setRelay4Pin(int value) { relay4Pin = value; }
    
    // Relay names
    String getRelay1Name() { return relay1Name; }
    void setRelay1Name(const String &value) { relay1Name = value; }
    String getRelay2Name() { return relay2Name; }
    void setRelay2Name(const String &value) { relay2Name = value; }
    String getRelay3Name() { return relay3Name; }
    void setRelay3Name(const String &value) { relay3Name = value; }
    String getRelay4Name() { return relay4Name; }
    void setRelay4Name(const String &value) { relay4Name = value; }
    
    // Soil moisture calibration
    int getSoilMoistureDryValue() { return soilMoistureDry; }
    void setSoilMoistureDryValue(int value) { soilMoistureDry = value; }
    int getSoilMoistureWetValue() { return soilMoistureWet; }
    void setSoilMoistureWetValue(int value) { soilMoistureWet = value; }
    
    // Watering settings
    int getWateringDuration() { return wateringDuration; }
    void setWateringDuration(int value) { wateringDuration = value; }
    
    // Watering time getters/setters - hour/minute format for more flexibility
    int getWateringStartHour() { return wateringStartHour; }
    void setWateringStartHour(int value) { wateringStartHour = value; }
    int getWateringStartMinute() { return wateringStartMinute; }
    void setWateringStartMinute(int value) { wateringStartMinute = value; }
    int getWateringEndHour() { return wateringEndHour; }
    void setWateringEndHour(int value) { wateringEndHour = value; }
    int getWateringEndMinute() { return wateringEndMinute; }
    void setWateringEndMinute(int value) { wateringEndMinute = value; }
    
    // Soil moisture threshold for watering
    float getSoilMoistureThreshold() { return soilMoistureThreshold; }
    void setSoilMoistureThreshold(float value) { soilMoistureThreshold = value; }
    
    // For backward compatibility
    bool isWateringEnabled() { return true; } // Always enabled, can be controlled by soil threshold
    
    // For legacy or simpler use cases
    int getWateringTimeStart() { return wateringStartHour; }
    int getWateringTimeEnd() { return wateringEndHour; }
};

#endif // CONFIG_H
