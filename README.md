# Embedded Systems Lab

A set of Arduino UNO R4 WiFi and ESP32 prototypes for access control, automation, displays, environmental monitoring, and Bluetooth audio.

Each project stands on its own and includes source code, hardware notes, dependencies, configuration steps, and known limitations. Network credentials and other deployment values stay outside source control.

## Projects

| Project | Platform | Highlights | Status |
| --- | --- | --- | --- |
| [RFID access controller](projects/access-control/uno-r4-rfid-access-controller) | Arduino UNO R4 WiFi | RFID, web UI, OLED, door monitoring, PTE and PIR inputs, event log | Prototype |
| [RFID + BLE door controller](projects/access-control/uno-r4-rfid-ble-door-controller) | Arduino UNO R4 WiFi | RFID authorization, BLE commands and status notifications | Prototype |
| [Eight-channel relay controller](projects/automation/uno-r4-eight-channel-relay-controller) | Arduino UNO R4 WiFi | Web control, pulse actions, auto-off, daily and one-shot schedules | Prototype |
| [LED matrix NTP clock](projects/displays/uno-r4-led-matrix-ntp-clock) | Arduino UNO R4 WiFi | NTP synchronization, RTC fallback, U.S. Central Time DST handling | Demonstration |
| [OLED logo animation](projects/displays/uno-r4-oled-logo-animation) | Arduino UNO R4 | Software-SPI OLED rendering and bitmap animation | Demonstration |
| [Camera environment dashboard](projects/monitoring/esp32-camera-environment-dashboard) | ESP32 camera board | MJPEG stream, DHT11 readings and responsive local dashboard | Prototype |
| [Bluetooth speaker](projects/audio/esp32-bluetooth-speaker) | ESP32 | A2DP sink, I2S audio output and serial volume control | Prototype |

See [Project status](docs/project-status.md) for scope and limitations.

## Getting started

1. Open the project folder you want to build.
2. Install the board package and libraries listed in that project's README.
3. When a project contains `*.example` configuration files, copy each one to the same name without `.example`.
4. Replace the placeholder values only in those local configuration files.
5. Select the documented board and port, then compile and upload from Arduino IDE.

Local `arduino_secrets.h` and `access_config.h` files are ignored by Git. Never commit network credentials, card UIDs, tokens, or production endpoint details.

## Engineering notes

- These projects are educational prototypes, not certified life-safety, security, or industrial-control products.
- Access and relay examples require external protection, correct fail-safe behavior, and validation against the applicable codes before real-world deployment.
- Brownout detection remains enabled. Fix resets with a regulated supply, adequate current capacity, short power wiring, decoupling, and correct grounding.
- Several web interfaces are intended only for a trusted local network and do not implement production authentication or transport encryption.

Read [Hardware safety](docs/hardware-safety.md), [Security policy](SECURITY.md), and [Contributing](CONTRIBUTING.md) before deploying or proposing changes.

## Repository history

This repository contains the projects selected for public reference. Older experiments, duplicate drafts, generated build artifacts, and deployment-specific values are kept out of it.

## License

No open-source license has been selected yet. All rights are reserved unless a license is added later.

