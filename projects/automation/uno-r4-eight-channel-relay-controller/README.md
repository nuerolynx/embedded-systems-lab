# UNO R4 WiFi eight-channel relay controller

A local web controller for eight relay channels with momentary actions and in-memory schedules.

## Features

- Individual and all-channel control
- Configurable channel names
- Momentary pulse actions and per-channel automatic shutoff
- One-shot and daily schedules synchronized by NTP
- JSON status/configuration endpoints and a browser-based control UI

## Hardware map

Channels 1 through 8 use digital pins 2 through 9 respectively. The current source assumes active-high relay inputs; verify this before applying power.

## Dependencies

- Arduino UNO R4 Boards package
- WiFiS3 and WiFiUdp from the board package

## Configure and build

1. Copy `arduino_secrets.h.example` to `arduino_secrets.h` and add lab-network credentials.
2. Confirm relay polarity and pin assignments in the sketch.
3. Compile for **Arduino UNO R4 WiFi** and upload.
4. Read the serial output for the assigned local IP address.

## Important limitations

Schedules and channel configuration are stored only in RAM and reset after a power cycle. Time uses a fixed UTC−6 offset and does not implement daylight-saving transitions. The HTTP interface has no authentication or TLS; use only on an isolated, trusted network. Do not switch mains voltage with exposed development hardware.
