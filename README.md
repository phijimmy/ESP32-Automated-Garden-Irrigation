# ESP32 Garden Monitor

An automated garden monitoring and irrigation system built with ESP32. This project provides real-time environmental monitoring (temperature, humidity, pressure), soil moisture tracking, and automated watering capabilities with a user-friendly web interface.


## Features

- **Environmental Monitoring**: Temperature, humidity, barometric pressure, and heat index
- **Soil Moisture Tracking**: Real-time soil moisture percentage with calibration support
- **Automated Irrigation**: Schedule-based or moisture-triggered watering
- **Manual Controls**: Direct control of four relays for water pumps or valves
- **Web Interface**: Mobile-friendly dashboard for monitoring and control
- **WiFi Connectivity**: Easy setup via access point and configuration portal
- **Real-Time Clock**: Battery-backed DS3231 for accurate timekeeping

## Hardware Requirements

- ESP32 Development Board
- BME280 Temperature/Humidity/Pressure Sensor
- Capacitive Soil Moisture Sensor
- DS3231 RTC Module
- 4-Channel Relay Module
- Power Supply (5V)
- Water Pump or Solenoid Valves
- Touch Sensor (optional)

## Software Dependencies

- Arduino framework for ESP32
- Adafruit BME280 Library
- Adafruit Unified Sensor
- RTClib
- ESPAsyncWebServer
- AsyncTCP
- ArduinoJson

## Installation

1. Clone this repository
2. Open in PlatformIO (or Arduino IDE with manual library installation)
3. Upload the code to your ESP32
4. Upload the data files to ESP32 flash using:
   - In PlatformIO: `pio run --target uploadfs`
   - In Arduino IDE: Use ESP32 Sketch Data Upload tool

## Initial Setup

1. Power on the ESP32
2. Connect to the "Garden Monitor" WiFi network (password: gardening123)
3. Open a browser and navigate to `http://192.168.4.1`
4. Complete the setup form with your desired configuration
5. The device will restart and begin monitoring

## Usage

- View real-time sensor readings on the main dashboard
- Use "Read Now" to take immediate sensor readings
- Control relays manually with the relay buttons
- Use "Water Now" for immediate irrigation
- Configure settings in the setup interface

## Documentation

- [Wiring Diagram](docs/wiring.md) (to-do)
- [Configuration Options](docs/config.md) (to-do)
- [API Documentation](docs/api.md) (to-do)

## License

MIT License - See [LICENSE](LICENSE) for details

## Acknowledgments

Created, designed and programmed by Jim - [M1Musik.com](https://M1Musik.com)
