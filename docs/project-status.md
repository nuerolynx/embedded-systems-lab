# Project status

| Project | Maturity | Hardware validation | Known limitations |
| --- | --- | --- | --- |
| RFID access controller | Prototype | Based on a working UNO R4 WiFi hardware configuration | Local HTTP control is unauthenticated; authorization data is static; not certified for security or life safety |
| RFID + BLE door controller | Prototype | Based on a working fail-secure relay configuration | BLE command interface is a test interface; no production identity or authorization protocol |
| Eight-channel relay controller | Prototype | Pin mapping targets an eight-channel relay module | Schedules are RAM-only; fixed UTC-6 offset; local HTTP control is unauthenticated |
| LED matrix NTP clock | Demonstration | Targets the built-in UNO R4 WiFi LED matrix and RTC | Time-zone logic is specific to America/Chicago rules |
| OLED logo animation | Demonstration | Targets a 128 × 64 SSD1306 display over software SPI | Bitmap is compiled into the sketch; wiring is fixed in source |
| Camera environment dashboard | Prototype | Targets the documented SunFounder-style ESP32 camera pin map and DHT11 | Local-only HTTP/MJPEG; no authentication or TLS; sensor readings are not calibrated |
| Bluetooth speaker | Prototype | Targets an ESP32 with an external I2S amplifier or DAC | Library/core compatibility is version-sensitive; audio output requires a suitable power supply |

“Prototype” means the design demonstrates an end-to-end workflow but still needs threat modeling, fault testing, enclosure and power design, and deployment-specific validation. “Demonstration” means the example is intentionally narrow and educational.

## Compile validation

Validation performed with Arduino CLI 1.5.1 on 2026-08-29:

| Project | Result | Validation target |
| --- | --- | --- |
| RFID access controller | Pending | Local U8g2 dependency discovery stalled in the installed Renesas compiler; no sketch error was reported before the check was stopped |
| RFID + BLE door controller | Passed | UNO R4 WiFi core 1.6.0, MFRC522 1.4.12, ArduinoBLE 2.1.0 |
| Eight-channel relay controller | Passed | UNO R4 WiFi core 1.6.0 |
| LED matrix NTP clock | Passed | UNO R4 WiFi core 1.6.0, NTPClient 3.2.1 |
| OLED logo animation | Pending | Local U8g2 dependency discovery stalled in the installed Renesas compiler; no sketch error was reported before the check was stopped |
| Camera environment dashboard | Passed | ESP32 core 3.3.11, ESP32 Wrover target, DHT sensor library 1.4.7 |
| Bluetooth speaker | Passed | ESP32 core 3.3.11, ESP32-A2DP 1.8.11 |

Compile success does not establish correct wiring, safe power delivery, sensor accuracy, relay ratings, or real-world security behavior. Each project still requires physical-hardware validation.
