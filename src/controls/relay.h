#ifndef RELAY_H
#define RELAY_H

#include <Arduino.h>

class Relay {
private:
    int relayPin;
    bool state = false;
    bool activeHigh = true; // Set to true for relays that activate with HIGH
    
public:
    Relay(int pin, bool isActiveHigh = true) {
        relayPin = pin;
        activeHigh = isActiveHigh;
        
        // Immediately configure pin if set in constructor
        if (relayPin >= 0) {
            pinMode(relayPin, OUTPUT);
            // Ensure relay is OFF at instantiation - critical for power-up
            digitalWrite(relayPin, activeHigh ? LOW : HIGH);
        }
    }
    
    Relay() {
        relayPin = -1;
    }
    
    void init() {
        if (relayPin >= 0) {  // IMPORTANT: Keep this check
            // Set pin as OUTPUT first
            pinMode(relayPin, OUTPUT);
            
            // Immediately drive to inactive state
            digitalWrite(relayPin, activeHigh ? LOW : HIGH);
            
            // Initialize state
            state = false;
            
            Serial.printf("Relay pin %d initialized to OFF\n", relayPin);
        }
    }
    
    void turnOn() {
        if (relayPin >= 0) {
            // For active HIGH relays, we set HIGH to turn ON
            // For active LOW relays, we set LOW to turn ON
            digitalWrite(relayPin, activeHigh ? HIGH : LOW);
            state = true;
            Serial.printf("Relay pin %d turned ON\n", relayPin);
        }
    }
    
    void turnOff() {
        if (relayPin >= 0) {
            // For active HIGH relays, we set LOW to turn OFF
            // For active LOW relays, we set HIGH to turn OFF
            digitalWrite(relayPin, activeHigh ? LOW : HIGH);
            state = false;
            Serial.printf("Relay pin %d turned OFF\n", relayPin);
        }
    }
    
    void toggle() {
        if (state) {
            turnOff();
        } else {
            turnOn();
        }
    }
    
    bool getState() {
        return state;
    }
    
    void syncState() {
        if (relayPin >= 0) {
            // Read the current pin value
            int pinValue = digitalRead(relayPin);
            // Determine the logical state based on activeHigh setting
            state = (activeHigh && pinValue == HIGH) || (!activeHigh && pinValue == LOW);
        }
    }
    
    void setRelayPin(int pin) { 
        relayPin = pin; 
        // When pin is changed, immediately configure it
        if (relayPin >= 0) {
            pinMode(relayPin, OUTPUT);
            // Ensure relay is OFF when pin is set
            digitalWrite(relayPin, activeHigh ? LOW : HIGH);
            state = false;
        }
    }
    
    void setActiveHigh(bool high) {
        // Only change if it's different to avoid unnecessary toggling
        if (activeHigh != high) {
            activeHigh = high;
            // Reapply current state with new logic
            if (relayPin >= 0) {
                digitalWrite(relayPin, state ? 
                    (activeHigh ? HIGH : LOW) : 
                    (activeHigh ? LOW : HIGH));
                Serial.printf("Relay %d logic changed to active %s\n", 
                             relayPin, activeHigh ? "HIGH" : "LOW");
            }
        }
    }
    
    bool isActiveHigh() {
        return activeHigh;
    }
};

#endif