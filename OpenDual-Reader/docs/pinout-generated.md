# Generated GPIO contract

Source: `firmware/hardware_contract.json`.

| GPIO | Net | Firmware name | Function | Startup |
|---|---|---|---|---|
| 1 | PROG_TX | PROGRAM_TX | 3.3 V UART programming TX | UART0 boot log |
| 3 | PROG_RX | PROGRAM_RX | 3.3 V UART programming RX | input |
| 26 | RS485_TX | RS485_TX | UART1 TX to transceiver DI | high after UART setup |
| 27 | RS485_RX | RS485_RX | UART1 RX from transceiver RO | input |
| 4 | RS485_DIR | RS485_DIR | UART1 RTS automatic half duplex; DE and /RE | external pulldown; receive |
| 34 | LF_RX_3V3 | LF_RX | UART2 RX 9600 8N1 from ID-12LA-HE pin9 TTL UART; verify idle-high on bench | input-only |
| 18 | HF_SCK | HF_SCK | PN532 SPI clock | low |
| 19 | HF_MISO | HF_MISO | PN532 SPI data to host | input |
| 23 | HF_MOSI | HF_MOSI | PN532 SPI data from host | low |
| 21 | HF_CS_N | HF_CS | PN532 SPI select | external pullup; inactive high |
| 22 | HF_RST_N | HF_RESET | PN532 reset active low | released; module pullup |
| 16 | WG_D0_GATE | WIEGAND_D0 | high enables D0 sink transistor | external pulldown; transistor off |
| 17 | WG_D1_GATE | WIEGAND_D1 | high enables D1 sink transistor | external pulldown; transistor off |
| 25 | LED_R_GATE | LED_R | high enables red LED driver | external pulldown; off |
| 33 | LED_G_GATE | LED_G | high enables green LED driver | external pulldown; off |
| 13 | LED_B_GATE | LED_B | high enables blue LED driver | external pulldown; off |
| 14 | BUZZER_GATE | BUZZER | 4kHz PWM enables passive piezo driver | external pulldown; off |
| 32 | RELAY_EN | DEMO_RELAY | logic only to separate demonstration relay board | external pulldown; off |
| 36 | TAMPER_N | TAMPER | assembled-closed to GND through remote 1k and KSC223G LFG SPST-NO; HIGH/open = tamper | external pullup required |
