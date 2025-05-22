#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include <Arduino.h>
#include <Wire.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>

// Include our components
#include "config.h"
#include "utils/wifi_manager.h"
#include "sensors/bme280.h"
#include "sensors/soil_moisture.h"
#include "sensors/ds3231.h"
#include "controls/relay.h"
#include "controls/touch.h"

// Add after the includes but before any function declarations

// Define fixed relay pins for the quad relay board
const int RELAY1_PIN = 25;  // Fixed hardware pin for relay 1
const int RELAY2_PIN = 26;  // Fixed hardware pin for relay 2
const int RELAY3_PIN = 32;  // Fixed hardware pin for relay 3
const int RELAY4_PIN = 33;  // Fixed hardware pin for relay 4

// Global objects
Config config;
WiFiManager wifiManager;
SoilMoistureSensor soilSensor;
TouchControl touchSensor;
Relay relay1, relay2, relay3, relay4;
AsyncWebServer server(80);

// RTC and BME globals (declared in their header files)
extern RTC_DS3231 rtc;
extern Adafruit_BME280 bme;

// State variables
bool isWatering = false;
unsigned long wateringStartTime = 0;
unsigned long wateringDuration = 0;
unsigned long lastSensorUpdate = 0;
const unsigned long sensorUpdateInterval = 60000; // 1 minute
int lastWateringDay = -1;  // Day of month when watering last occurred

// Sensor readings
float temperature = 0;
float humidity = 0;
float pressure = 0;
float heatIndex = 0;
int soilMoistureRaw = 0;
float soilMoisturePercent = 0;

// Function prototypes
void setupWebServer();
void updateSensorReadings();
bool shouldWater();
void startWatering();
void stopWatering();
void checkWateringStatus();
void checkTouchSensor();
void readAllBME280Sensors();
float calculateHeatIndex(float temperature, float humidity);
float readAveragedTemperature();  
float readAveragedHumidity();     
float readAveragedPressure(); 

void setup() {
  // Configure relays with internal pull-ups first
  //pinMode(RELAY1_PIN, INPUT_PULLUP);
  //pinMode(RELAY2_PIN, INPUT_PULLUP);
  //pinMode(RELAY3_PIN, INPUT_PULLUP);
  //pinMode(RELAY4_PIN, INPUT_PULLUP);
  
  // Configure relay pins immediately to prevent clicking
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);
  pinMode(RELAY4_PIN, OUTPUT);
  
  // Drive all relays LOW (off) for active HIGH relays
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);
  digitalWrite(RELAY3_PIN, LOW);
  digitalWrite(RELAY4_PIN, LOW);
  delay(20);
  Serial.begin(115200);
  
  // Initialize LittleFS first (needed for config)
  if (!LittleFS.begin()) {
    Serial.println("Failed to mount file system");
  }
  
  // Initialize configuration (needed for relay pins)
  if (!config.begin()) {
    Serial.println("Failed to initialize configuration");
  }
  
  // IMPORTANT: Initialize relays first to prevent clicking
  relay1.setRelayPin(config.getRelay1Pin());
  relay2.setRelayPin(config.getRelay2Pin());
  relay3.setRelayPin(config.getRelay3Pin());
  relay4.setRelayPin(config.getRelay4Pin());
  
  // Configure relays as active HIGH (HIGH signal activates relay)
  relay1.setActiveHigh(true);
  relay2.setActiveHigh(true); 
  relay3.setActiveHigh(true);
  relay4.setActiveHigh(true);
  
  relay1.init();
  relay2.init();
  relay3.init();
  relay4.init();
  
  Serial.println("\n\nGarden Monitor System Starting...");
  
  // Configure I2C pins from config
  Wire.begin(config.getI2cSdaPin(), config.getI2cSclPin());
  
  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
  }
  
  // Initialize BME280 with multiple address attempts
  bool bmeFound = false;
  
  // Try default address 0x76 first
  Serial.println("Trying BME280 at address 0x76...");
  if (bme.begin(0x76, &Wire)) {
    bmeFound = true;
    Serial.println("BME280 found at address 0x76");
  } else {
    // Try alternative address 0x77
    delay(100);
    Serial.println("Trying BME280 at address 0x77...");
    if (bme.begin(0x77, &Wire)) {
      bmeFound = true;
      Serial.println("BME280 found at address 0x77");
    }
  }
  
  if (!bmeFound) {
    Serial.println("Could not find a valid BME280 sensor");
  } else {
    // Set BME280 to sleep mode to save power
    bme.setSampling(
      Adafruit_BME280::MODE_FORCED,
      Adafruit_BME280::SAMPLING_X1, // temperature
      Adafruit_BME280::SAMPLING_X1, // pressure
      Adafruit_BME280::SAMPLING_X1, // humidity
      Adafruit_BME280::FILTER_OFF
    );
  }
  
  // Initialize soil moisture sensor
  soilSensor.setSensorPin(config.getSoilMoistureSensorPin());
  soilSensor.setPowerPin(config.getSoilMoisturePowerPin());
  soilSensor.calibrateDry(config.getSoilMoistureDryValue());
  soilSensor.calibrateWet(config.getSoilMoistureWetValue());
  soilSensor.init();
  
  // Initialize touch sensor
  touchSensor.setTouchPin(config.getTouchSensorPin());
  touchSensor.setThreshold(config.getTouchSensorThreshold());
  touchSensor.init();
  
  // Initialize WiFi manager
  wifiManager.setAPCredentials(config.getDeviceName(), "gardening123");
  wifiManager.begin();
  
  // Check if this is first time setup or a reset was requested
  if (config.isFirstTimeSetup()) {
    Serial.println("First time setup or reset requested");
    wifiManager.setSetupMode(true);
    wifiManager.startHotspot();
  } else {
    // Always start hotspot after setup completion reboot
    // This gives user 15 minutes to reconnect after setup
    Serial.println("Starting hotspot after setup (15 min timeout)");
    wifiManager.startHotspot();
    wifiManager.resetClientActivityTimer();
  }
  
  // Set up web server routes and start server
  setupWebServer();
  
  // Take initial sensor readings
  updateSensorReadings();
  
  Serial.println("Setup complete");
}

void loop() {
  // Process DNS requests for captive portal
  wifiManager.processDNS();
  
  // Check if we need to update sensor readings
  if (millis() - lastSensorUpdate > sensorUpdateInterval) {
    updateSensorReadings();
  }
  
  // Check if watering is in progress
  if (isWatering) {
    checkWateringStatus();
  }
  
  // Check if watering should start (autonomous watering)
  if (!isWatering && shouldWater()) {
    startWatering();
  }
  
  // Check touch sensor to activate hotspot
  checkTouchSensor();
  
  // Check for hotspot inactivity timeout
  wifiManager.checkHotspotTimeout();
  
  // Small delay to prevent CPU hogging
  delay(10);
}

void updateSensorReadings() {
  Serial.println("\n--- UPDATING SENSOR READINGS ---");
  
  // Use the new combined BME280 reading function
  readAllBME280Sensors();
  yield();
  
  // Calculate heat index based on temperature and humidity
  heatIndex = calculateHeatIndex(temperature, humidity);
  Serial.printf("Heat Index: %.2f°C\n", heatIndex);
  
  // Read soil moisture
  Serial.println("Reading soil moisture sensor...");
  soilMoistureRaw = soilSensor.readAveragedRaw();
  yield();
  
  // Calculate soil moisture percentage  
  soilMoisturePercent = soilSensor.readPercentage();
  Serial.printf("Soil Moisture: %.2f%% (Raw: %d)\n", soilMoisturePercent, soilMoistureRaw);
  
  // Update timestamp for last sensor reading
  lastSensorUpdate = millis();
}

float calculateHeatIndex(float temperature, float humidity) {
  // Only calculate heat index if temperature is high enough
  // Below about 26.7°C (80°F), the heat index equals the temperature
  if (temperature < 26.7) {
    return temperature;
  }
  
  // Heat index calculation (simplified equation)
  // Source: https://www.wpc.ncep.noaa.gov/html/heatindex_equation.shtml
  float hIndex = 0.5 * (temperature + 61.0 + ((temperature - 68.0) * 1.2) + (humidity * 0.094));
  
  // If the humidity is high enough and temperature high enough, use full equation
  if (humidity > 40 && temperature >= 27) {
    hIndex = -42.379 +
      2.04901523 * temperature +
      10.14333127 * humidity -
      0.22475541 * temperature * humidity -
      0.00683783 * temperature * temperature -
      0.05481717 * humidity * humidity +
      0.00122874 * temperature * temperature * humidity +
      0.00085282 * temperature * humidity * humidity -
      0.00000199 * temperature * temperature * humidity * humidity;
  }
  
  return hIndex;
  }

  void checkTouchSensor() {
  if (touchSensor.isTouched()) {
    Serial.println("Touch detected! Starting hotspot...");
    wifiManager.startHotspot();
    wifiManager.resetClientActivityTimer();
  }
  }

  bool shouldWater() {
  // Only check if automatic watering is enabled
  if (!config.isWateringEnabled()) {
    return false;
  }
  
  // Get current time
  DateTime now = rtc.now();
  uint8_t currentHour = now.hour();
  uint8_t currentMinute = now.minute();
  uint8_t currentDay = now.dayOfTheWeek();
  int currentDayOfMonth = now.day(); 
  
    // Check if we already watered today
  if (lastWateringDay == currentDayOfMonth) {  // Add this check
    return false;  // Already watered today
  }

  // Don't water on Sundays (day 0)
  if (currentDay == 0) {
    return false;
  }
  
  // Don't water at night (22:00 - 07:00)
  if (currentHour >= 22 || currentHour < 7) {
    return false;
  }
  
  // Check if it's watering time
  int startHour = config.getWateringStartHour();
  int startMinute = config.getWateringStartMinute();
  int endHour = config.getWateringEndHour();
  int endMinute = config.getWateringEndMinute();
  
  // Convert time to minutes since midnight for easier comparison
  int currentTimeMinutes = currentHour * 60 + currentMinute;
  int startTimeMinutes = startHour * 60 + startMinute;
  int endTimeMinutes = endHour * 60 + endMinute;
  
  // Check if current time is within watering window
  bool isWateringTime = (currentTimeMinutes >= startTimeMinutes && currentTimeMinutes <= endTimeMinutes);
  
  if (!isWateringTime) {
    return false;
  }
  
  // Check soil moisture 
  if (soilMoisturePercent > config.getSoilMoistureThreshold()) {
    return false;
  }
  
  // All conditions met, should water now
  return true;
  }

  void startWatering() {
  if (isWatering) {
    return; // Already watering
  }
  
  Serial.println("Starting watering cycle");
  
    // Record today as the watering day
  DateTime now = rtc.now();
  lastWateringDay = now.day();

  // Turn on pump (relay2)
  relay2.turnOn();
  
  // Set watering state
  isWatering = true;
  wateringStartTime = millis();
  wateringDuration = config.getWateringDuration() * 1000; // Convert to milliseconds
  
  Serial.printf("Watering will run for %d seconds\n", config.getWateringDuration());
  }

  void stopWatering() {
  if (!isWatering) {
    return; // Not watering
  }
  
  Serial.println("Stopping watering cycle");
  
  // Turn off pump (relay2)
  relay2.turnOff();
  
  // Reset watering state
  isWatering = false;
  
  Serial.println("Watering cycle complete");
  }

  void checkWateringStatus() {
  if (!isWatering) {
    return;
  }
  
  // Check if watering duration has elapsed
  unsigned long elapsed = millis() - wateringStartTime;
  if (elapsed >= wateringDuration) {
    stopWatering();
  }
}

// New function to read all BME280 sensors with a single power-up cycle
void readAllBME280Sensors() {
    Serial.println("Reading BME280 sensor with single power cycle...");
    
    // Put BME280 in forced mode once for all readings
    bme.setSampling(
        Adafruit_BME280::MODE_FORCED,
        Adafruit_BME280::SAMPLING_X1, // temperature
        Adafruit_BME280::SAMPLING_X1, // pressure
        Adafruit_BME280::SAMPLING_X1, // humidity
        Adafruit_BME280::FILTER_OFF
    );
    
    // Wake up sensor and take initial measurement
    if (bme.takeForcedMeasurement()) {
        // Break up stabilization delay
        const int smallDelay = 20; // 20ms chunks
        for (int i = 0; i < 5; i++) {
            delay(smallDelay);
            yield();
        }
        
        // Take all readings in single wake/sleep cycle
        const int numReadings = 5;
        float tempSum = 0, humiditySum = 0, pressureSum = 0;
        int validTempReadings = 0, validHumidityReadings = 0, validPressureReadings = 0;
        
        Serial.println("Taking averaged BME280 readings...");
        for (int i = 0; i < numReadings; i++) {
            yield();
            
            // Read temperature
            float tempReading = bme.readTemperature();
            if (!isnan(tempReading) && tempReading >= -40 && tempReading <= 85) {
                tempSum += tempReading;
                validTempReadings++;
                Serial.printf("Reading %d: %.2f°C, ", i + 1, tempReading);
            }
            
            // Read humidity
            float humidityReading = bme.readHumidity();
            if (!isnan(humidityReading) && humidityReading >= 0 && humidityReading <= 100) {
                humiditySum += humidityReading;
                validHumidityReadings++;
                Serial.printf("%.2f%%, ", humidityReading);
            }
            
            // Read pressure
            float pressureReading = bme.readPressure() / 100.0F;
            if (!isnan(pressureReading) && pressureReading >= 300 && pressureReading <= 1100) {
                pressureSum += pressureReading;
                validPressureReadings++;
                Serial.printf("%.2f hPa\n", pressureReading);
            } else {
                Serial.println();
            }
            
            // Small delay between readings
            if (i < numReadings - 1) {
                delay(50);
                yield();
            }
        }
        
        // Calculate averages
        temperature = (validTempReadings > 0) ? (tempSum / validTempReadings) : 0;
        humidity = (validHumidityReadings > 0) ? (humiditySum / validHumidityReadings) : 0;
        pressure = (validPressureReadings > 0) ? (pressureSum / validPressureReadings) : 0;
        
        Serial.printf("Average temperature: %.2f°C (from %d readings)\n", temperature, validTempReadings);
        Serial.printf("Average humidity: %.2f%% (from %d readings)\n", humidity, validHumidityReadings);
        Serial.printf("Average pressure: %.2f hPa (from %d readings)\n", pressure, validPressureReadings);
    } else {
        Serial.println("Failed to perform forced measurement");
    }
    
    // Put BME280 back to sleep mode
    bme.setSampling(
        Adafruit_BME280::MODE_SLEEP,
        Adafruit_BME280::SAMPLING_X1,
        Adafruit_BME280::SAMPLING_X1,
        Adafruit_BME280::SAMPLING_X1,
        Adafruit_BME280::FILTER_OFF
    );
}
// BME280 averaged reading functions
float readAveragedTemperature() {
    const int numReadings = 5;
    const int readingInterval = 100; // ms
    
    float sum = 0;
    int validReadings = 0;
    
    Serial.println("Taking averaged temperature readings...");
    for (int i = 0; i < numReadings; i++) {
        if (bme.takeForcedMeasurement()) {
            delay(100); // Wait for measurement to complete
            float reading = bme.readTemperature();
            
            if (!isnan(reading) && reading >= -40 && reading <= 85) {
                sum += reading;
                validReadings++;
                Serial.printf("Reading %d: %.2f°C\n", i + 1, reading);
            } else {
                Serial.printf("Reading %d: Invalid value, ignored\n", i + 1);
            }
        }
        
        // Wait between readings if not the last one
        if (i < numReadings - 1) {
            delay(readingInterval);
        }
    }
    
    // Calculate average (or return 0 if no valid readings)
    float average = (validReadings > 0) ? (sum / validReadings) : 0;
    Serial.printf("Average temperature: %.2f°C (from %d valid readings)\n", 
                  average, validReadings);
    
    return average;
}

float readAveragedHumidity() {
    const int numReadings = 5;
    const int readingInterval = 100; // ms
    
    float sum = 0;
    int validReadings = 0;
    
    Serial.println("Taking averaged humidity readings...");
    for (int i = 0; i < numReadings; i++) {
        if (bme.takeForcedMeasurement()) {
            delay(100); // Wait for measurement to complete
            float reading = bme.readHumidity();
            
            if (!isnan(reading) && reading >= 0 && reading <= 100) {
                sum += reading;
                validReadings++;
                Serial.printf("Reading %d: %.2f%%\n", i + 1, reading);
            } else {
                Serial.printf("Reading %d: Invalid value, ignored\n", i + 1);
            }
        }
        
        // Wait between readings if not the last one
        if (i < numReadings - 1) {
            delay(readingInterval);
        }
    }
    
    // Calculate average (or return 0 if no valid readings)
    float average = (validReadings > 0) ? (sum / validReadings) : 0;
    Serial.printf("Average humidity: %.2f%% (from %d valid readings)\n", 
                 average, validReadings);
    
    return average;
}

float readAveragedPressure() {
    const int numReadings = 5;
    const int readingInterval = 100; // ms
    
    float sum = 0;
    int validReadings = 0;
    
    Serial.println("Taking averaged pressure readings...");
    for (int i = 0; i < numReadings; i++) {
        if (bme.takeForcedMeasurement()) {
            delay(100); // Wait for measurement to complete
            float reading = bme.readPressure() / 100.0F; // Convert Pa to hPa
            
            if (!isnan(reading) && reading >= 300 && reading <= 1100) {
                sum += reading;
                validReadings++;
                Serial.printf("Reading %d: %.2f hPa\n", i + 1, reading);
            } else {
                Serial.printf("Reading %d: Invalid value, ignored\n", i + 1);
            }
        }
        
        // Wait between readings if not the last one
        if (i < numReadings - 1) {
            delay(readingInterval);
        }
    }
    
    // Calculate average (or return 0 if no valid readings)
    float average = (validReadings > 0) ? (sum / validReadings) : 0;
    Serial.printf("Average pressure: %.2f hPa (from %d valid readings)\n", 
                 average, validReadings);
    
    return average;
}

void setupWebServer() {
  // IMPORTANT: Define API endpoints BEFORE the static file handler
  
  // API endpoint: Get sensor data
  server.on("/api/sensor-data", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Create JSON response
    DynamicJsonDocument doc(1024);
    
    // Add device information
    doc["deviceName"] = config.getDeviceName();
    
    // BME280 sensor data
    doc["temperature"] = temperature;
    doc["humidity"] = humidity;
    doc["pressure"] = pressure;
    doc["heat_index"] = heatIndex;
    
    // Soil data
    doc["soil_raw"] = soilMoistureRaw;
    doc["soil_moisture"] = soilMoisturePercent;
    
    // Get current time from RTC
    DateTime now = rtc.now();
    char timeStr[20];
    sprintf(timeStr, "%02d/%02d/%04d %02d:%02d:%02d", 
            now.day(), now.month(), now.year(),
            now.hour(), now.minute(), now.second());
    doc["time_str"] = timeStr;
    doc["timestamp"] = now.unixtime();
    
    // Relay status
    JsonArray relays = doc.createNestedArray("relays");
    relays.add(relay1.getState());
    relays.add(relay2.getState());
    relays.add(relay3.getState());
    relays.add(relay4.getState());
    
    // Relay names
    JsonArray relayNames = doc.createNestedArray("relay_names");
    relayNames.add(config.getRelay1Name());
    relayNames.add(config.getRelay2Name());
    relayNames.add(config.getRelay3Name());
    relayNames.add(config.getRelay4Name());
    
    // Watering status
    doc["watering_active"] = isWatering;
    if (isWatering) {
      unsigned long elapsed = millis() - wateringStartTime;
      unsigned long remaining = (elapsed < wateringDuration) ? 
                              (wateringDuration - elapsed) / 1000 : 0;
      doc["watering_remaining"] = remaining;
    }
    
    // Serialize JSON to string
    String jsonResponse;
    serializeJson(doc, jsonResponse);
    
    // Send response
    request->send(200, "application/json", jsonResponse);
    
    // Reset client activity timer
    wifiManager.resetClientActivityTimer();
  });
  
  // API endpoint: Get configuration data
  server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(1024);
    
    // Device and authentication settings
    doc["deviceName"] = config.getDeviceName();
    doc["username"] = config.getUsername();
    doc["password"] = config.getPassword();
    
    // Pin assignments
    doc["soilMoisturePin"] = config.getSoilMoistureSensorPin();
    doc["soilMoisturePowerPin"] = config.getSoilMoisturePowerPin();
    doc["touchPin"] = config.getTouchSensorPin();
    doc["touchSensorThreshold"] = config.getTouchSensorThreshold();
    doc["i2cSdaPin"] = config.getI2cSdaPin();
    doc["i2cSclPin"] = config.getI2cSclPin();
    doc["relay1Pin"] = config.getRelay1Pin();
    doc["relay2Pin"] = config.getRelay2Pin();
    doc["relay3Pin"] = config.getRelay3Pin();
    doc["relay4Pin"] = config.getRelay4Pin();
    
    // Relay names
    doc["relay1Name"] = config.getRelay1Name();
    doc["relay2Name"] = config.getRelay2Name();
    doc["relay3Name"] = config.getRelay3Name();
    doc["relay4Name"] = config.getRelay4Name();
    
    // Soil moisture calibration
    doc["soilMoistureDry"] = config.getSoilMoistureDryValue();
    doc["soilMoistureWet"] = config.getSoilMoistureWetValue();
    
    // Watering settings
    doc["wateringDuration"] = config.getWateringDuration();
    
    // Add compatibility fields for the legacy form fields
    doc["wateringTimeStart"] = config.getWateringStartHour();
    doc["wateringTimeEnd"] = config.getWateringEndHour();
    doc["waterAtPercent"] = config.getSoilMoistureThreshold();
    
    // Add new format fields too
    doc["wateringStartHour"] = config.getWateringStartHour();
    doc["wateringStartMinute"] = config.getWateringStartMinute();
    doc["wateringEndHour"] = config.getWateringEndHour();
    doc["wateringEndMinute"] = config.getWateringEndMinute();
    doc["soilMoistureThreshold"] = config.getSoilMoistureThreshold();
    
    String jsonResponse;
    serializeJson(doc, jsonResponse);
    
    request->send(200, "application/json", jsonResponse);
    
    // Reset client activity timer
    wifiManager.resetClientActivityTimer();
  });
  
  // API endpoint: Control relay
  server.on("/api/relay", HTTP_POST, [](AsyncWebServerRequest *request) {
    // Require authentication if not in setup mode
    if (!config.isFirstTimeSetup() && 
        !request->authenticate(config.getUsername().c_str(), config.getPassword().c_str())) {
      return request->requestAuthentication();
    }
    
    // Check for required parameters
    if (!request->hasParam("id", true) || !request->hasParam("state", true)) {
      return request->send(400, "application/json", "{\"success\":false,\"message\":\"Missing parameters\"}");
    }
    
    // Get parameters
    int relayId = request->getParam("id", true)->value().toInt();
    int state = request->getParam("state", true)->value().toInt();
    
    // Validate relay ID (0-3)
    if (relayId < 0 || relayId > 3) {
      return request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid relay ID\"}");
    }
    
    // Control appropriate relay
    bool success = true;
    if (relayId == 0) {
      if (state) relay1.turnOn(); else relay1.turnOff();
    } else if (relayId == 1) {
      if (state) relay2.turnOn(); else relay2.turnOff();
    } else if (relayId == 2) {
      if (state) relay3.turnOn(); else relay3.turnOff();
    } else if (relayId == 3) {
      if (state) relay4.turnOn(); else relay4.turnOff();
    } else {
      success = false;
    }
    
    // Create JSON response
    DynamicJsonDocument doc(128);
    doc["success"] = success;
    doc["id"] = relayId;
    doc["state"] = state;
    
    String jsonResponse;
    serializeJson(doc, jsonResponse);
    
    // Send response
    request->send(200, "application/json", jsonResponse);
    
    // Reset client activity timer
    wifiManager.resetClientActivityTimer();
  });
  
  // API endpoint: Start watering
  server.on("/api/water-now", HTTP_POST, [](AsyncWebServerRequest *request) {
    // Require authentication if not in setup mode
    if (!config.isFirstTimeSetup() && 
        !request->authenticate(config.getUsername().c_str(), config.getPassword().c_str())) {
      return request->requestAuthentication();
    }
    
    // Start watering with configured duration
    Serial.println("Watering requested via API");
    startWatering();
    
    request->send(200, "text/plain", "Watering started");
    
    // Reset client activity timer
    wifiManager.resetClientActivityTimer();
  });

  // API endpoint: Force sensor reading now
  server.on("/api/read-now", HTTP_POST, [](AsyncWebServerRequest *request) {
    // Require authentication if not in setup mode
    if (!config.isFirstTimeSetup() && 
        !request->authenticate(config.getUsername().c_str(), config.getPassword().c_str())) {
      return request->requestAuthentication();
    }
    
    Serial.println("\n!!! SENSOR READING REQUESTED VIA READ NOW BUTTON !!!");
    
    // Trigger a sensor update immediately
    updateSensorReadings();
    
    // Send success response
    request->send(200, "application/json", "{\"success\":true}");
    
    // Reset client activity timer
    wifiManager.resetClientActivityTimer();
  });
  
  // API endpoint: Reset device to setup mode
  server.on("/api/reset", HTTP_POST, [](AsyncWebServerRequest *request) {
    // Require authentication if not in setup mode
    if (!config.isFirstTimeSetup() && 
        !request->authenticate(config.getUsername().c_str(), config.getPassword().c_str())) {
      return request->requestAuthentication();
    }
    
    // Reset configuration and set first-time setup flag
    config.setFirstTimeSetup(true);
    
    // Save configuration BEFORE restarting
    if (!config.saveConfig()) {
      Serial.println("Failed to save configuration before reset");
    }
    
    // Send response before restarting
    request->send(200, "text/plain", "Device will restart in setup mode");
    
    // Short delay to ensure response is sent
    delay(500);
    
    // Restart the device
    ESP.restart();
  });
  
  // API endpoint: Save configuration (for setup page)
  // This handles both form-encoded data AND JSON data
  server.on("/api/config", HTTP_POST, 
    // Handler for form-encoded data
    [](AsyncWebServerRequest *request) {
      // This handler is only for form-encoded data, not JSON
      // Check content type to avoid double-processing
      if (request->contentType() == "application/json") {
        // Skip - this will be handled by the onBody handler
        return;
      }
      
      Serial.println("Received setup configuration (form-encoded)");
      
      // Process all form fields
      int params = request->params();
      
      for (int i = 0; i < params; i++) {
        AsyncWebParameter* p = request->getParam(i);
        if (p->isPost()) {
          Serial.printf("Setup parameter: %s = %s\n", p->name().c_str(), p->value().c_str());
          
          // Device name
          if (p->name() == "deviceName") {
            config.setDeviceName(p->value());
          }
          // Date and Time settings
          else if (p->name() == "date") {
            // Store the date param for later processing with time
          }
          else if (p->name() == "time") {
            // If we have both date and time, set the RTC
            if (request->hasParam("date", true)) {
              String dateStr = request->getParam("date", true)->value();
              String timeStr = p->value();
              
              Serial.print("Setting date: ");
              Serial.print(dateStr);
              Serial.print(" and time: ");
              Serial.println(timeStr);
              
              // Parse date - format YYYY-MM-DD
              int year = dateStr.substring(0, 4).toInt();
              int month = dateStr.substring(5, 7).toInt();
              int day = dateStr.substring(8, 10).toInt();
              
              // Parse time - format HH:MM or HH:MM:SS
              int hour = timeStr.substring(0, 2).toInt();
              int minute = timeStr.substring(3, 5).toInt();
              int second = timeStr.length() > 5 ? timeStr.substring(6, 8).toInt() : 0;
              
              Serial.printf("Parsed: %04d-%02d-%02d %02d:%02d:%02d\n", 
                          year, month, day, hour, minute, second);
              
              // Check if parsed values are valid
              if (year >= 2023 && month >= 1 && month <= 12 && day >= 1 && day <= 31 &&
                  hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59 && second >= 0 && second <= 59) {
                
                // Set the RTC
                rtc.adjust(DateTime(year, month, day, hour, minute, second));
                Serial.println("RTC time set successfully");
              } else {
                Serial.println("Invalid date/time format");
              }
            }
          }
          // GPIO pin settings
          else if (p->name() == "soilMoisturePin") {
            config.setSoilMoistureSensorPin(p->value().toInt());
          }
          else if (p->name() == "soilMoisturePowerPin") {
            config.setSoilMoisturePowerPin(p->value().toInt());
          }
          else if (p->name() == "touchPin") {
            config.setTouchSensorPin(p->value().toInt());
          }
          else if (p->name() == "touchSensorThreshold") {
            config.setTouchSensorThreshold(p->value().toInt());
          }
          else if (p->name() == "i2cSdaPin") {
            config.setI2cSdaPin(p->value().toInt());
          }
          else if (p->name() == "i2cSclPin") {
            config.setI2cSclPin(p->value().toInt());
          }
          else if (p->name() == "relay1Pin") {
            config.setRelay1Pin(p->value().toInt());
          }
          else if (p->name() == "relay2Pin") {
            config.setRelay2Pin(p->value().toInt());
          }
          else if (p->name() == "relay3Pin") {
            config.setRelay3Pin(p->value().toInt());
          }
          else if (p->name() == "relay4Pin") {
            config.setRelay4Pin(p->value().toInt());
          }
          // Relay names
          else if (p->name() == "relay1Name") {
            config.setRelay1Name(p->value());
          }
          else if (p->name() == "relay2Name") {
            config.setRelay2Name(p->value());
          }
          else if (p->name() == "relay3Name") {
            config.setRelay3Name(p->value());
          }
          else if (p->name() == "relay4Name") {
            config.setRelay4Name(p->value());
          }
          // Soil moisture calibration
          else if (p->name() == "soilMoistureDry") {
            config.setSoilMoistureDryValue(p->value().toInt());
          }
          else if (p->name() == "soilMoistureWet") {
            config.setSoilMoistureWetValue(p->value().toInt());
          }
          // Watering configuration - support both legacy and new formats
          else if (p->name() == "wateringDuration") {
            config.setWateringDuration(p->value().toInt());
          }
          else if (p->name() == "wateringTimeStart") {
            // Legacy format - just hour
            config.setWateringStartHour(p->value().toInt());
            config.setWateringStartMinute(0); // Default to 0 minutes
          }
          else if (p->name() == "wateringTimeEnd") {
            // Legacy format - just hour
            config.setWateringEndHour(p->value().toInt());
            config.setWateringEndMinute(0); // Default to 0 minutes
          }
          else if (p->name() == "waterAtPercent") {
            // Legacy format for threshold
            config.setSoilMoistureThreshold(p->value().toFloat());
          }
          // New format fields
          else if (p->name() == "wateringStartHour") {
            config.setWateringStartHour(p->value().toInt());
          }
          else if (p->name() == "wateringStartMinute") {
            config.setWateringStartMinute(p->value().toInt());
          }
          else if (p->name() == "wateringEndHour") {
            config.setWateringEndHour(p->value().toInt());
          }
          else if (p->name() == "wateringEndMinute") {
            config.setWateringEndMinute(p->value().toInt());
          }
          else if (p->name() == "soilMoistureThreshold") {
            config.setSoilMoistureThreshold(p->value().toFloat());
          }
          // Authorization
          else if (p->name() == "username") {
            config.setUsername(p->value());
          }
          else if (p->name() == "password") {
            config.setPassword(p->value());
          }
        }
      }
      
      // Complete setup
      config.setFirstTimeSetup(false);
      
      // Save configuration
      if (config.saveConfig()) {
        Serial.println("Configuration saved successfully");
      } else {
        Serial.println("Failed to save configuration");
      }
      
      // Send response
      request->send(200, "text/plain", "Configuration saved. The device will restart.");
      
      // Short delay to ensure response is sent
      delay(500);
      
      // Restart the device
      ESP.restart();
    },
    // Handler for file uploads (none for this endpoint)
    NULL,
    // Handler for body data (JSON)
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      // This handler is only for JSON data
      if (request->contentType() != "application/json") {
        return; // Skip - not JSON
      }
      
      Serial.println("Received setup configuration (JSON)");
      
      // Parse JSON data
      DynamicJsonDocument doc(2048);
      DeserializationError error = deserializeJson(doc, data, len);
      
      if (error) {
        Serial.print("JSON parsing failed: ");
        Serial.println(error.c_str());
        request->send(400, "text/plain", "Invalid JSON data");
        return;
      }
      
      // Process device name
      if (doc.containsKey("deviceName")) {
        config.setDeviceName(doc["deviceName"].as<String>());
      }
      
      // Process date and time
      if (doc.containsKey("date") && doc.containsKey("time")) {
        String dateStr = doc["date"].as<String>();
        String timeStr = doc["time"].as<String>();
        
        Serial.print("Setting date: ");
        Serial.print(dateStr);
        Serial.print(" and time: ");
        Serial.println(timeStr);
        
        // Parse date - format YYYY-MM-DD
        int year = dateStr.substring(0, 4).toInt();
        int month = dateStr.substring(5, 7).toInt();
        int day = dateStr.substring(8, 10).toInt();
        
        // Parse time - format HH:MM or HH:MM:SS
        int hour = timeStr.substring(0, 2).toInt();
        int minute = timeStr.substring(3, 5).toInt();
        int second = timeStr.length() > 5 ? timeStr.substring(6, 8).toInt() : 0;
        
        Serial.printf("Parsed: %04d-%02d-%02d %02d:%02d:%02d\n", 
                    year, month, day, hour, minute, second);
        
        // Check if parsed values are valid
        if (year >= 2023 && month >= 1 && month <= 12 && day >= 1 && day <= 31 &&
            hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59 && second >= 0 && second <= 59) {
          
          // Set the RTC
          rtc.adjust(DateTime(year, month, day, hour, minute, second));
          Serial.println("RTC time set successfully");
        } else {
          Serial.println("Invalid date/time format");
        }
      }
      
      // Process all other config values from JSON
      if (doc.containsKey("soilMoisturePin")) {
        config.setSoilMoistureSensorPin(doc["soilMoisturePin"].as<int>());
      }
      if (doc.containsKey("soilMoisturePowerPin")) {
        config.setSoilMoisturePowerPin(doc["soilMoisturePowerPin"].as<int>());
      }
      if (doc.containsKey("touchPin")) {
        config.setTouchSensorPin(doc["touchPin"].as<int>());
      }
      if (doc.containsKey("touchSensorThreshold")) {
        config.setTouchSensorThreshold(doc["touchSensorThreshold"].as<int>());
      }
      if (doc.containsKey("i2cSdaPin")) {
        config.setI2cSdaPin(doc["i2cSdaPin"].as<int>());
      }
      if (doc.containsKey("i2cSclPin")) {
        config.setI2cSclPin(doc["i2cSclPin"].as<int>());
      }
      if (doc.containsKey("relay1Pin")) {
        config.setRelay1Pin(doc["relay1Pin"].as<int>());
      }
      if (doc.containsKey("relay2Pin")) {
        config.setRelay2Pin(doc["relay2Pin"].as<int>());
      }
      if (doc.containsKey("relay3Pin")) {
        config.setRelay3Pin(doc["relay3Pin"].as<int>());
      }
      if (doc.containsKey("relay4Pin")) {
        config.setRelay4Pin(doc["relay4Pin"].as<int>());
      }
      
      // Relay names
      if (doc.containsKey("relay1Name")) {
        config.setRelay1Name(doc["relay1Name"].as<String>());
      }
      if (doc.containsKey("relay2Name")) {
        config.setRelay2Name(doc["relay2Name"].as<String>());
      }
      if (doc.containsKey("relay3Name")) {
        config.setRelay3Name(doc["relay3Name"].as<String>());
      }
      if (doc.containsKey("relay4Name")) {
        config.setRelay4Name(doc["relay4Name"].as<String>());
      }
      
      // Soil moisture calibration
      if (doc.containsKey("soilMoistureDry")) {
        config.setSoilMoistureDryValue(doc["soilMoistureDry"].as<int>());
      }
      if (doc.containsKey("soilMoistureWet")) {
        config.setSoilMoistureWetValue(doc["soilMoistureWet"].as<int>());
      }
      
      // Watering settings
      if (doc.containsKey("wateringDuration")) {
        config.setWateringDuration(doc["wateringDuration"].as<int>());
      }
      
      // Legacy format support
      if (doc.containsKey("wateringTimeStart")) {
        config.setWateringStartHour(doc["wateringTimeStart"].as<int>());
        config.setWateringStartMinute(0);
      }
      if (doc.containsKey("wateringTimeEnd")) {
        config.setWateringEndHour(doc["wateringTimeEnd"].as<int>());
        config.setWateringEndMinute(0);
      }
      if (doc.containsKey("waterAtPercent")) {
        config.setSoilMoistureThreshold(doc["waterAtPercent"].as<float>());
      }
      
      // New format fields
      if (doc.containsKey("wateringStartHour")) {
        config.setWateringStartHour(doc["wateringStartHour"].as<int>());
      }
      if (doc.containsKey("wateringStartMinute")) {
        config.setWateringStartMinute(doc["wateringStartMinute"].as<int>());
      }
      if (doc.containsKey("wateringEndHour")) {
        config.setWateringEndHour(doc["wateringEndHour"].as<int>());
      }
      if (doc.containsKey("wateringEndMinute")) {
        config.setWateringEndMinute(doc["wateringEndMinute"].as<int>());
      }
      if (doc.containsKey("soilMoistureThreshold")) {
        config.setSoilMoistureThreshold(doc["soilMoistureThreshold"].as<float>());
      }
      
      // Authentication
      if (doc.containsKey("username")) {
        config.setUsername(doc["username"].as<String>());
      }
      if (doc.containsKey("password")) {
        config.setPassword(doc["password"].as<String>());
      }
      
      // Complete setup
      config.setFirstTimeSetup(false);
      
      // Save configuration
      if (config.saveConfig()) {
        Serial.println("Configuration saved successfully");
      } else {
        Serial.println("Failed to save configuration");
      }
      
      // Send response
      request->send(200, "text/plain", "Configuration saved. The device will restart.");
      
      // Short delay to ensure response is sent
      delay(500);
      
      // Restart the device
      ESP.restart();
    }
  );
  
  // IMPORTANT: Setup authentication handler for index.html
  server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    // If we're in setup mode, redirect to setup page
    if (config.isFirstTimeSetup()) {
      request->redirect("/setup.html");
      return;
    }
    
    // Require authentication
    if (!request->authenticate(config.getUsername().c_str(), config.getPassword().c_str())) {
      return request->requestAuthentication();
    }
    
    // If authenticated, serve the file
    request->send(LittleFS, "/index.html", "text/html");
    
    // Reset client activity timer
    wifiManager.resetClientActivityTimer();
  });
  
  // Redirect root to either setup.html or index.html
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (config.isFirstTimeSetup()) {
      request->redirect("/setup.html");
    } else {
      request->redirect("/index.html");
    }
    
    // Reset client activity timer
    wifiManager.resetClientActivityTimer();
  });
  
  // Handle captive portal detection files
  server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/");
    // Reset client activity timer
    wifiManager.resetClientActivityTimer();
  });
  
  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/");
    wifiManager.resetClientActivityTimer();
  }); 
  
  server.on("/connectivity-check.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/");
    wifiManager.resetClientActivityTimer();
  });
  
  // Additional captive portal detection handlers
  server.on("/success.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/");
    wifiManager.resetClientActivityTimer();
  });
  
  server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/");
    wifiManager.resetClientActivityTimer();
  });

  // NOTE: Static file handler MUST be last!
  // This allows all API endpoints defined above to be handled properly first
  server.serveStatic("/", LittleFS, "/");
  
  // Start the server
  server.begin();
  Serial.println("Web server started");
}