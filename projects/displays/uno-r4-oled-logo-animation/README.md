# UNO R4 OLED logo animation

A self-contained 128 × 64 monochrome logo-rendering demonstration using a compiled bitmap and software SPI.

## Hardware map

| OLED signal | UNO R4 pin |
| --- | --- |
| CLK | A1 |
| MOSI | A2 |
| CS | 8 |
| DC | 4 |
| RESET | 5 |

## Dependencies

- Arduino UNO R4 Boards package
- U8g2

## Build

1. Wire a compatible 128 × 64 SSD1306 display to the documented pins.
2. Compile for the intended **Arduino UNO R4** board and upload.

The bitmap is stored directly in flash as `IMG_2654`. To replace it, export a 128 × 64 one-bit vertical-byte array and update that array while preserving the display format.
