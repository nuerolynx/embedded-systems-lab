# Optional relay demonstration board

The separate 20 × 30 mm, two-layer board in `hardware/kicad/relay-demo` demonstrates the reader's active-high relay output. Its native KiCad project, schematic, PCB, project-local libraries, BOM and validation reports are included. The board is fully routed. It is an unbuilt bench prototype, with no measured relay, EMC or lifetime results.

The demonstration limit is **24 VDC, 0.5 A maximum, resistive load only**. No mains or door-lock load is part of this demonstration. Keep this expansion outside the compact reader enclosure. Neither the coil current nor the switched load current flows through the reader's power rails.

## Wiring

| Expansion connection | Wire to |
|---|---|
| J1 pin1 GND | Main reader J5 pin1 GND |
| J1 pin2 RELAY_EN | Main reader J5 pin2 RELAY_EN / GPIO32 |
| Main reader J5 pin3 | Leave unconnected |
| J2 pin1 +12V_EXT | Separate regulated 12V ±5% bench supply, current limit100mA |
| J2 pin2 GND | Bench supply return; shared with reader ground through J1 |
| J3 pin1 NC | Load contact closed to common when relay is de-energized |
| J3 pin2 COM | Common of the isolated mechanical contacts |
| J3 pin3 NO | Load contact closed to common when relay is energized |

All three connectors are solder holes, not purchased headers. J1 and J2 use 3mm pitch; J3 uses 6mm pitch. Finished drill0.8mm, pad1.8mm. Provide strain relief and inspect solder joints. There is no reverse-polarity circuit or onboard fuse on this small expansion; use the stated current-limited bench supply and verify polarity first. Shared logic ground means the input is not galvanically isolated from the reader. Contacts have no intentional conductive connection to logic, but this board has no certified isolation rating.

## Circuit and selected components

K1 is Omron G5V-1 DC12, a non-latching SPDT relay. The manufacturer specifies a12V coil, nominal12.5mA and960Ω, with ±10% coil-resistance/current tolerance at23°C. The relay's published resistive contact rating is1A at24VDC; this demonstration uses the lower0.5A limit. Manufacturer bottom-view terminal drawing was visually checked: coil2/9, NC1, NO10 and common5/6. Both common pads are connected on the PCB. The standard KiCad `Relay_SPDT_Omron_G5V-1` footprint was checked against the drawing. [Omron datasheet K048-E1](https://components.omron.com/us-en/system/files/2023-01/datasheet_pdf/K048-E1.pdf), [alternate manufacturer PDF](https://omronfs.omron.com/en_US/ecb/products/pdf/en-g5v_1.pdf).

Q1 is onsemi MMBT3904LT1G with SOT23 pins1 base,2 emitter,3 collector. R1 is1kΩ, and R2 is100kΩ from base to ground. At VOH2.64V and VBE0.95V, approximate available base current is(2.64−0.95)/1000−0.95/100000=1.68mA. At12.6V and864Ω minimum nominal-room-temperature coil resistance, the simple coil-current bound is14.6mA before transistor drop. This gives forced beta below9, a conservative preliminary drive choice. It is not a substitute for hot/cold pickup and saturation measurements. [onsemi datasheet](https://www.onsemi.com/pdf/datasheet/mmbt3904lt1-d.pdf).

D1 is Diodes Incorporated1N4148W-7-F, SOD123, with cathode at+12V_EXT and anode atCOIL_LOW. Its100V reverse rating and300mA continuous-current limit exceed the intended coil voltage/current. Actual turn-off waveform and diode heating remain bench checks. A simple flyback diode slows mechanical release; do not claim the relay's unsuppressed release-time figure for this circuit. [Diodes Incorporated datasheet, September2024](https://www.diodes.com/datasheet/download/1N4148W.pdf).

R1 uses Yageo [RC0603FR-071KL](https://yageogroup.com/component-documentation/download/specsheet/RC0603FR-071KL); R2 uses [RC0603FR-07100KL](https://yageogroup.com/component-documentation/download/specsheet/RC0603FR-07100KL), both1%0603. The exact component list is in the expansion's `BOM.csv`.

## Layout and verification

The board uses1.6mm FR4, two copper layers, proposed1oz copper, 0.2mm minimum clearance, 0.25mm base-signal routes, 0.35mm coil/ground routes and0.6mm contact routes. Vias are0.65mm pads with0.3mm drills. Trace widths are prototype selections for these restricted currents; no high-voltage spacing, controlled impedance or high-energy protection claim is made. The relay body is10mm maximum above the board, plus leads below. No mounting holes are provided; retain the assembly mechanically and prevent contact with conductive surfaces.

KiCad 10 checks completed on 2026-09-26: ERC 0 violations; PCB DRC 0 violations, 0 unconnected items and 0 schematic-parity issues. The final check is `validation/DRC-parity.json`. Native schematic, combined copper/silkscreen plot and a KiCad 3D rendering were visually inspected. A schematic PDF and PNG 3D preview are included. Four used STEP models are copied into `RelayDemo.3dshapes`; both native PCB and footprint model references use project-relative paths. These checks establish CAD consistency, not real-world operation.

Before energizing: inspect the diode band, transistor orientation and connector pin1 marks; check for shorts; measure the unpowered coil resistance and NC/NO/contact-common mapping. Apply current-limited12V with RELAY_EN low, then assert3.3V logic. Measure pickup, dropout, coil current, Q1 collector voltage, turn-off overshoot and release delay. Finally switch a small resistive test load and confirm contact behavior. Observe reader reset and power sequencing; the relay must remain inactive until explicitly requested by the demo firmware. Firmware relay demonstration must remain disabled in normal access-reader operation.
