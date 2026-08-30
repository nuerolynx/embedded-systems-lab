# ESP32 Bluetooth speaker

An ESP32 Bluetooth A2DP sink that sends stereo audio to an external I2S amplifier or DAC and exposes simple serial volume controls.

## Hardware map

| I2S signal | ESP32 pin |
| --- | --- |
| BCLK | GPIO 26 |
| LRCLK / WS | GPIO 25 |
| DATA | GPIO 22 |

Power the amplifier from a supply suitable for its peak load and connect signal ground according to the amplifier and ESP32 specifications.

## Dependencies

- Espressif ESP32 Arduino core **3.x** (validated with 3.3.11)
- ESP32-A2DP **1.8.11** (`BluetoothA2DPSink.h`)

## Build and use

1. Install an ESP32 3.x core and ESP32-A2DP 1.8.11 or a compatible release.
2. Confirm the I2S pins and amplifier ratings.
3. Compile and upload to a compatible classic ESP32 board.
4. Pair a phone with `ESP32 Speaker`.
5. Use Serial Monitor commands `+`, `-`, or `v0` through `v127` to tune the clean output level.

Brownout detection remains enabled. If playback causes resets, improve the supply, current capacity, decoupling, grounding, or wiring before continuing. High playback levels can clip the signal or damage an amplifier, speaker, or hearing.
