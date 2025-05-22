#include "soil_moisture.h"

void SoilMoistureSensor::init() {
    if (sensorPin >= 0) {
        pinMode(sensorPin, INPUT);
    }
    
    if (powerPin >= 0) {
        pinMode(powerPin, OUTPUT);
        digitalWrite(powerPin, LOW); // Start with power off
    }
}

// New method to read raw value without power management
int SoilMoistureSensor::readRawWithoutPower() {
    // Just read the analog value without power management
    int value = analogRead(sensorPin);
    yield(); // Add yield after reading
    return value;
}

int SoilMoistureSensor::readRaw() {
    // Power on the sensor if a power pin is configured
    if (powerPin >= 0) {
        digitalWrite(powerPin, HIGH);
        delay(500); // Increase to 750-1000ms for more reliable readings
        yield(); // Add yield after delay to prevent watchdog timeout
    }
    
    // Read the analog value
    int value = analogRead(sensorPin);
    yield(); // Add yield after reading
    
    // Turn off power to save energy
    if (powerPin >= 0) {
        digitalWrite(powerPin, LOW);
    }
    
    return value;
}

float SoilMoistureSensor::readPercentage() {
    int rawValue = readRaw();
    yield(); // Add yield after raw reading
    
    // Map the raw value to 0-100% range
    // Note: higher raw value = drier soil, so we invert the logic
    float percentage = map(rawValue, wetValue, dryValue, 100, 0);
    
    // Constrain to valid range
    if (percentage > 100) percentage = 100;
    if (percentage < 0) percentage = 0;
    
    return percentage;
}

float SoilMoistureSensor::readAveragedPercentage() {
    Serial.println("Taking averaged soil moisture readings...");
    
    float sum = 0;
    int validReadings = 0;
    
    for (int i = 0; i < numReadings; i++) {
        float reading = readPercentage();
        yield(); // Add yield after each reading
        
        // Only include readings within valid range
        if (reading >= 0 && reading <= 100) {
            sum += reading;
            validReadings++;
            Serial.printf("Reading %d: %.1f%%\n", i + 1, reading);
        } else {
            Serial.printf("Reading %d: Invalid value (%.1f), ignored\n", i + 1, reading);
        }
        
        // Wait between readings if not the last reading
        if (i < numReadings - 1) {
            delay(readingInterval);
            yield(); // Add yield after delay
        }
    }
    
    // Calculate average (or return 0 if no valid readings)
    float average = (validReadings > 0) ? (sum / validReadings) : 0;
    Serial.printf("Average soil moisture: %.1f%% (from %d valid readings)\n", 
                  average, validReadings);
    
    return average;
}

int SoilMoistureSensor::readAveragedRaw() {
    Serial.println("Taking averaged raw soil moisture readings...");
    
    // Power on the sensor ONCE at the beginning
    if (powerPin >= 0) {
        digitalWrite(powerPin, HIGH);
        
        // Break up the 500ms delay into smaller chunks with yield calls
        Serial.println("Powering up soil moisture sensor...");
        const int smallDelay = 50; // 50ms chunks
        for (int i = 0; i < 10; i++) { // 10 * 50ms = 500ms
            delay(smallDelay);
            yield();
        }
        Serial.println("Sensor powered up, taking readings...");
    }
    
    long sum = 0;
    int validReadings = 0;
    
    for (int i = 0; i < numReadings; i++) {
        yield(); // Add yield before each reading
        
        // Use readRawWithoutPower() since the sensor is already on
        int reading = readRawWithoutPower();
        yield(); // Add yield after reading
        
        // Use a simple validity check (non-zero and within ADC range)
        if (reading > 0 && reading < 4096) {
            sum += reading;
            validReadings++;
            Serial.printf("Raw reading %d: %d\n", i + 1, reading);
        } else {
            Serial.printf("Raw reading %d: Invalid value (%d), ignored\n", i + 1, reading);
        }
        
        // Wait between readings if not the last reading
        if (i < numReadings - 1) {
            // Smaller delay between readings
            delay(readingInterval);
            yield();
        }
    }
    
    // Power off the sensor at the end
    if (powerPin >= 0) {
        digitalWrite(powerPin, LOW);
        Serial.println("Soil moisture sensor powered down");
    }
    
    // Calculate average (or return max value if no valid readings)
    int average = (validReadings > 0) ? (sum / validReadings) : dryValue;
    Serial.printf("Average raw soil moisture: %d (from %d valid readings)\n", 
                 average, validReadings);
    
    return average;
}

float SoilMoistureSensor::temperatureCompensation(float moisture, float temperature) {
    // Temperature compensation factor
    const float compensationFactor = 0.3; // % adjustment per °C
    
    // Reference temperature (calibration temperature)
    const float refTemp = 25.0; // °C
    
    // Calculate temperature difference from reference
    float tempDiff = temperature - refTemp;
    
    // Apply compensation
    float compensatedMoisture = moisture + (tempDiff * compensationFactor);
    
    // Constrain to valid range
    if (compensatedMoisture > 100) compensatedMoisture = 100;
    if (compensatedMoisture < 0) compensatedMoisture = 0;
    
    Serial.printf("Temperature compensation: Original: %.1f%%, After (at %.1f°C): %.1f%%\n", 
                  moisture, temperature, compensatedMoisture);
    
    return compensatedMoisture;
}

void SoilMoistureSensor::calibrateDry(int value) {
    dryValue = value;
}

void SoilMoistureSensor::calibrateWet(int value) {
    wetValue = value;
}

void SoilMoistureSensor::setSensorPin(int pin) { 
    sensorPin = pin; 
}

void SoilMoistureSensor::setPowerPin(int pin) { 
    powerPin = pin; 
}