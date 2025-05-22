#ifndef BME280_H
#define BME280_H

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// Global BME280 sensor instance
extern Adafruit_BME280 bme;

// Function declarations only, no implementation
bool bme280_init();

#endif