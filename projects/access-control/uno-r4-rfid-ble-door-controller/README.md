# UNO R4 WiFi RFID + BLE door controller

A focused door-control prototype with RFID authorization and a Bluetooth Low Energy command/status interface.

## Features

- MFRC522 card reader
- Configurable authorized UID outside source control
- Active-high relay output with red/green indicators and audible feedback
- BLE command characteristic and status notifications
- Timed five-second grant or denial actions

## Hardware map

| Function | UNO R4 pin |
| --- | --- |
| RFID SS / RST | 10 / 9 |
| Relay | 7 |
| Red / green LED | 2 / 3 |
| Buzzer | 6 |

## Dependencies

- Arduino UNO R4 Boards package
- MFRC522
- ArduinoBLE

## Configure and build

1. Copy `access_config.h.example` to `access_config.h`.
2. Replace the placeholder with the colon-separated UID printed by the sketch.
3. Confirm the relay module is active-high and safe for the intended load.
4. Compile for **Arduino UNO R4 WiFi** and upload.

## Important limitations

The BLE service is a test interface, not a production authentication protocol. Any nearby compatible client may be able to interact with it. Do not rely on it to protect a real opening without a separate threat model, secure identity design, and code-compliant hardware.
