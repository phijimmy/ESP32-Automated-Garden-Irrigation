#include "ds3231.h"

// Define the global RTC object
RTC_DS3231 rtc;

bool ds3231_init() {
    if (!rtc.begin()) {
        Serial.println("Couldn't find RTC");
        return false;
    }
    
    if (rtc.lostPower()) {
        Serial.println("RTC lost power, let's set the time!");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    
    return true;
}