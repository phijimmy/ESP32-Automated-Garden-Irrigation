#ifndef TOUCH_H
#define TOUCH_H

#include <Arduino.h>

class TouchControl {
private:
    int touchPin;
    int threshold = 40;  // Default touch threshold
    unsigned long lastTouchTime = 0;
    const unsigned long debounceDelay = 500; // Debounce time in milliseconds
    
public:
    TouchControl(int pin) {
        touchPin = pin;
    }
    
    TouchControl() {
        touchPin = 4;  // Default pin
    }
    
    void init() {
        // No specific initialization needed for touch pins on ESP32
        Serial.printf("Touch sensor initialized on pin %d with threshold %d\n", 
                     touchPin, threshold);
    }
    
    bool isTouched() {
        // Read touch value (ESP32 returns lower values when touched)
        int touchValue = touchRead(touchPin);
        
        // Check if below threshold (touched) and debounce
        bool touched = (touchValue < threshold);
        
        if (touched) {
            // Debouncing logic
            unsigned long currentTime = millis();
            if (currentTime - lastTouchTime > debounceDelay) {
                lastTouchTime = currentTime;
                Serial.printf("Touch detected on pin %d (value: %d)\n", touchPin, touchValue);
                return true;
            }
        }
        
        return false;
    }
    
    // Check for touch and perform action when detected
    bool checkForTouch() {
        if (isTouched()) {
            Serial.println("Touch action triggered");
            return true;
        }
        return false;
    }
    
    void setThreshold(int value) {
        threshold = value;
        Serial.printf("Touch threshold set to %d\n", threshold);
    }
    
    int getThreshold() {
        return threshold;
    }
    
    int getRawValue() {
        return touchRead(touchPin);
    }
    // Add to TouchControl class
    void setTouchPin(int pin) { touchPin = pin; }
};

#endif