# ESP32 camera environment dashboard

A local browser dashboard that combines an ESP32 camera stream with DHT11 temperature and humidity readings.

## Features

- MJPEG camera stream on port 81
- Responsive status dashboard and sensor endpoint on port 80
- Celsius, Fahrenheit, and relative-humidity readings
- Camera pin map for the documented SunFounder-style extension hardware
- Brownout protection left enabled to expose power-integrity problems

## Hardware map

The camera mapping is defined at the top of the sketch. The DHT11 data pin defaults to GPIO 14. Confirm the map against your exact ESP32 camera board before powering it.

## Dependencies

- Espressif ESP32 Arduino board package
- DHT sensor library and its required sensor support dependency
- `esp_camera`, WiFi, WebServer, and HTTP server components supplied by the ESP32 package

## Configure and build

1. Copy `arduino_secrets.h.example` to `arduino_secrets.h` and add lab-network credentials.
2. Select an ESP32 board configuration with PSRAM appropriate for your camera hardware.
3. Verify every camera signal and the DHT pin.
4. Compile, upload, then read Serial Monitor for the dashboard address.

## Power and security

Camera initialization and Wi-Fi transmission create current spikes. Use a stable regulated supply and correct wiring; do not disable brownout detection to conceal resets. The dashboard has no authentication or TLS and should be reachable only on a trusted, isolated network.
