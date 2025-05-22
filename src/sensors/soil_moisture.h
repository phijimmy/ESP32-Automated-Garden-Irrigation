#ifndef SOIL_MOISTURE_H
#define SOIL_MOISTURE_H

#include <Arduino.h>

class SoilMoistureSensor {
private:
    int sensorPin = -1;
    int powerPin = -1;
    int wetValue = 815;   // Default calibration value for wet soil
    int dryValue = 2350;   // Default calibration value for dry soil
    const int numReadings = 5;        // Number of readings to average
    const int readingInterval = 100;  // ms between readings

public:
    SoilMoistureSensor() = default;
    
    // Function declarations only (no implementations)
    void init();
    int readRaw();
    int readRawWithoutPower();
    float readPercentage();
    float readAveragedPercentage();
    int readAveragedRaw();
    float temperatureCompensation(float moisture, float temperature);
    
    // Calibration support
    void calibrateDry(int value);
    void calibrateWet(int value);
    
    // Pin configuration
    void setSensorPin(int pin);
    void setPowerPin(int pin);
};

#endif