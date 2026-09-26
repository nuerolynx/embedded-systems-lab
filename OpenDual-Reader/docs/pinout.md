# Connectors and authoritative GPIO contract

[hardware_contract.json](../firmware/hardware_contract.json) is the single authoritative application GPIO/net definition. [pinout-generated.md](pinout-generated.md) and [pins.h](../firmware/include/pins.h) are generated from it. Hardware uses the same net names. The schematic and [design-data.json](../hardware/design-data.json) record module pad numbers and connectivity. The final PCB and assembly exports are authoritative for physical placement; the design-data file retains the initial placement proposals.

The reader's connector footprints are PCB solder lands. Pin 1 is square. They do not establish a verified mating commercial connector; service shorting pads are temporary fixture or jumper connections. Follow the numbered pads in the assembly export, especially the rotated J3 harness row.

| Connector | Pin order | Electrical meaning |
|---|---|---|
| J1 — panel | 1: VIN12; 2: GND; 3: RS485_A; 4: RS485_B; 5: WIEGAND_D0; 6: WIEGAND_D1 | 9–16 V DC input. RS-485 A is noninverting; B is inverting. Wiegand operating limit: 15 V pull-up and 5 mA sink current per line. |
| J2 — programming | 1: GND; 2: PROG_TX; 3: PROG_RX; 4: ESP_EN; 5: BOOT_N; 6: +3V3 | 3.3 V UART logic. Connect adapter RX to pad 2 and adapter TX to pad 3. Pad 6 is sense-only; do not power the reader from the adapter. |
| J3 — HF harness | 1: HF_CS_N; 2: HF_MOSI; 3: HF_MISO; 4: HF_SCK; 5: HF_RST_N; 6: +5V; 7: GND | Map pin-for-pin to the exact PN5321 MINI hardware V2 host connector. Supply is 5 V; host logic is 3.3 V. Firmware drives reset open drain. Verify harness continuity and connector orientation. |
| J4 — tamper | 1: TAMPER_N; 2: GND | Remote Y31B13136FPLFG / KSC223G LFG SPST-NO switch, held closed when assembled, with a 1 kΩ series resistor on its remote carrier. Released/open means HIGH and tamper. Confirm approximately 1 kΩ through the disconnected loop only while pressed. Use short internal wiring; no protected external tamper cable interface is provided. |
| J5 — relay logic | 1: GND; 2: RELAY_EN; 3: +3V3 | 3.3 V logic for a separate driver board. No relay-coil or lock power is provided. |
| JP1 — reset | 1: ESP_EN; 2: GND | Momentary short resets the ESP32. |
| JP2 — boot | 1: BOOT_N; 2: GND | Hold low during reset to enter the UART bootloader. |
| JP3 — LF reset | 1: LF_RESET_N; 2: GND | Manual LF reset and field-inhibit experiment. No MCU GPIO controls this net in Rev A. |
| JP4 — termination | 1: TERM_SW; 2: RS485_B | Normally open. Short to connect R7's 120 Ω termination across A/B when this reader is at a bus end. |

Some panels reverse the A/B lettering. Determine their noninverting conductor from documentation or a differential capture. RS-485 and Wiegand use the reader's signal GND reference. Wiring colors are unspecified. J1's bus and Wiegand protection do not establish tested surge or panel interoperability ratings.

The selected normally open tactile switch closes only while pressed. Its duplicated terminals form two electrically distinct groups; verify those groups against the purchased part before wiring. Connect across the two groups through the remote series resistor, rather than connecting two terminals from the same group. The supported carrier, bracket, plunger and enclosure actuation remain unvalidated. See [tamper assembly](tamper-assembly.md) and [mechanical fit](../mechanical/mechanical-fit.md).

Test access includes connector pads, U1/U2 output pads, ESP32 enable/boot pads, LF reset pads and component bypass pads. A dedicated fixture and bed-of-nails geometry are not finalized. Output timing, reset states and connector behavior remain bench-validation tasks.
