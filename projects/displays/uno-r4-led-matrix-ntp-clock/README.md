# UNO R4 WiFi LED matrix NTP clock

A compact network-synchronized clock for the UNO R4 WiFi's built-in 12 × 8 LED matrix.

## Features

- Non-blocking Wi-Fi retry behavior
- NTP synchronization with six-hour resyncs
- RTC timekeeping between network updates
- Alternating hour/minute display using a compact 5 × 7 font
- U.S. Central Time daylight-saving calculation
- Status pixels for display mode and synchronization state

## Dependencies

- Arduino UNO R4 Boards package
- WiFiS3, WiFiUdp, RTC, and Arduino_LED_Matrix from the board package
- NTPClient

## Configure and build

1. Copy `arduino_secrets.h.example` to `arduino_secrets.h` and add lab-network credentials.
2. Compile for **Arduino UNO R4 WiFi** and upload.
3. Open Serial Monitor to observe network and synchronization status.

The daylight-saving implementation is intentionally specific to current America/Chicago rules. Adapt and test it before using another locale or relying on it for time-critical control.
