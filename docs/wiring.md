# Garden Monitor - Wiring Diagram

## Components

- ESP32 Development Board
- BME280 Temperature/Humidity/Pressure Sensor (I2C)
- Capacitive Soil Moisture Sensor
- DS3231 RTC Module (I2C)
- 4-Channel Relay Module
- Touch Sensor (optional)
- 5V Power Supply

## Wiring Connections

### Power

- ESP32 VIN → 5V Power Supply
- ESP32 GND → Common Ground
- All other components connect to 3.3V (or 5V where required) and GND from the ESP32

### I2C Bus

- ESP32 GPIO21 (SDA) → BME280 SDA, DS3231 SDA
- ESP32 GPIO22 (SCL) → BME280 SCL, DS3231 SCL
- 4.7K pull-up resistors recommended on SDA and SCL lines

### Soil Moisture Sensor

- ESP32 GPIO36 (or configured analog pin) → Soil Moisture Sensor Output
- ESP32 GPIO32 (or configured digital pin) → Soil Moisture Sensor Power Control (optional)

### Relay Module

- ESP32 GPIO25 → Relay 1 Input
- ESP32 GPIO26 → Relay 2 Input
- ESP32 GPIO32 → Relay 3 Input
- ESP32 GPIO33 → Relay 4 Input

### Touch Sensor (Optional)

- ESP32 GPIO4 (or configured touch pin) → Touch Sensor Pad

## Notes

- Use appropriate resistors and protection circuits for production use
- For outdoor installations, ensure proper waterproofing of all components
- Consider adding a capacitor across the power supply to filter noise
- The soil moisture sensor should be powered only when taking readings to prevent corrosion

## Schematic Diagram

[Include a simple schematic diagram image here]
