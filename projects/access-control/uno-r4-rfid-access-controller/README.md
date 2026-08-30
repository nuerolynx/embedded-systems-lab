# UNO R4 WiFi RFID access controller

An integrated access-control prototype combining card authorization, a local status interface, door-position monitoring, request-to-exit inputs, and an OLED display.

## Features

- MFRC522 RFID reader with locally configured authorized UIDs
- Relay, red/green indicators, and buzzer feedback
- SH1106 128 × 64 OLED status display over software SPI
- Normally closed door contact, push-to-exit button, and PIR request-to-exit input
- Local web interface and a 20-event in-memory log
- Forced-open and held-open alarm states

## Hardware map

| Function | UNO R4 pin |
| --- | --- |
| RFID SS / RST | 10 / 9 |
| Relay | 7 |
| Red / green LED | 2 / 3 |
| Buzzer | 6 |
| OLED CLK / MOSI / CS / DC / RST | A1 / A2 / 8 / 4 / 5 |
| Door contact / PTE / PIR | A3 / A4 / A5 |

## Dependencies

- Arduino UNO R4 Boards package
- MFRC522
- WiFiS3
- U8g2

## Configure and build

1. Copy `arduino_secrets.h.example` to `arduino_secrets.h` and add the lab-network credentials.
2. Copy `access_config.h.example` to `access_config.h` and replace the placeholder UID. Keep the colon-separated format emitted by the sketch.
3. Confirm relay polarity, contact type, input polarity, and the pin map against your hardware.
4. Compile for **Arduino UNO R4 WiFi** and upload.

Both local configuration files are ignored by Git.

## Important limitations

The web interface is an unauthenticated local-network test interface. This prototype is not a certified access-control, egress, fire, or life-safety product. Review [hardware safety](../../../docs/hardware-safety.md) before connecting a lock or relay.
