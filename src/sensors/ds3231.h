#ifndef DS3231_H
#define DS3231_H

#include <Wire.h>
#include <RTClib.h>

// Global RTC instance
extern RTC_DS3231 rtc;

// Function declarations only, no implementation
bool ds3231_init();

#endif