# OpenDual optional relay demo

Open `OpenDual-Relay-Demo.kicad_pro` in KiCad 10. This separate 20×30 mm two-layer board is fully routed, with project-local symbols and footprints. The schematic uses ordinary editable relay, transistor, diode and resistor symbols.

- Wiring and restrictions: [relay-expansion.md](../../../docs/relay-expansion.md).
- Exact parts: `BOM.csv`.
- ERC and DRC results plus plots: `validation/`.
- Main-reader connection: J1.1 to main J5.1 GND; J1.2 to main J5.2 RELAY_EN. Leave main J5.3 unused.
- Separate regulated 12V ±5% supply at J2, current limit 100mA. J3 pins 1/2/3 are NC/COM/NO.

24VDC / 0.5A resistive demonstration only. No mains. This unbuilt optional assembly is outside the reader enclosure and has no fabrication or field-installation approval.

The project copies standard KiCad library symbols, footprints and the four used STEP models locally for editing. Model paths use `${KIPRJMOD}/RelayDemo.3dshapes`, so this subproject's 3D assets travel with it. The main project's licensing and third-party-library notices apply.
