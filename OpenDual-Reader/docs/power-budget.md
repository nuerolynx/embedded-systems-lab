# Power and thermal allocation

No electrical power or temperature was measured. This conservative **design allocation** is not a verified worst-case product specification: typical manufacturer currents do not establish system maxima. The operating input target is **9–16 V DC, nominally 12 V**. The initial ambient target of 0–50 °C requires enclosure validation.

| 3.3 V load | Allocated current | Power |
|---|---:|---:|
| ESP32 module, including RF bursts | 500 mA | 1.650 W |
| LF module | 50 mA | 0.165 W |
| RS-485 driver/receiver and termination load | 80 mA | 0.264 W |
| GPIO drivers, pull-ups and margin | 30 mA | 0.099 W |
| **Total 3.3 V rail** | **660 mA** | **2.178 W** |

| 5 V load | Allocated current | Power |
|---|---:|---:|
| PN5321 MINI and RF antenna | 200 mA | 1.000 W |
| RGB LED, all colors on | 45 mA | 0.225 W |
| Buzzer pull-up branch | 5 mA | 0.025 W |
| **Total 5 V rail** | **250 mA** | **1.250 W** |

Both TSR 1 regulators are rated for 1 A under their specified thermal conditions. Assuming 80% conversion efficiency gives (2.178 W + 1.250 W) / 0.80 = **4.285 W** at the converter inputs. An additional approximately 0.30 W allowance for the series diode and fuse gives about **4.6 W at the cable connector**. Budget 0.52 A at 9 V and 0.39 A at 12 V. These rounded values are planning allowances; capture startup inrush and RF current peaks. If measured continuous draw exceeds the allocation, revise fuse, copper and thermal design before release.

At 80% efficiency, the allocated converter losses are approximately 0.545 W for the 3.3 V regulator and 0.313 W for the 5 V regulator. A 0.5 V series-diode drop at 0.52 A would add 0.26 W; this is an assumed budget point, not a measured diode drop or a complete temperature-dependent maximum. Treat 4.6 W as a conservative enclosure heat load before accounting for energy emitted or dissipated outside it. No enclosure thermal-resistance model or hot-spot measurement is available. Firmware normally leaves Wi-Fi and Bluetooth disabled, while this allocation retains margin for their supply bursts. TRACO's temperature derating must be applied to the local regulator ambient, which can exceed the room temperature substantially in a closed enclosure.

F1 is the 0.75 A **1812L075/33DR** resettable PTC. Its hold current is temperature dependent: the manufacturer table lists 0.60 A at 50 °C and 0.56 A at 60 °C under its stated conditions, compared with the 0.52 A allocation at 9 V. Local self-heating and enclosure temperature rise therefore need measurement. Its maximum body thickness is 1.55 mm, leaving only 0.45 mm nominal clearance in the 2.0 mm rear gap before solder and assembly tolerances. [Littelfuse 1812L datasheet, June 2024](https://www.littelfuse.com/~/media/electronics/datasheets/resettable_ptcs/littelfuse_ptc_1812l_datasheet.pdf.pdf).

Input bulk capacitor C1 is 10 µF, rated for 50 V. D2 is the SMBJ18A with 18 V reverse standoff and a specified 29.2 V clamp point at its rated pulse current. Both converters accept input up to 36 V; the series diode B340A is rated for 40 V reverse voltage. These static ratings do not establish surge immunity. Harness inductance, clamp overshoot, pulse energy and PTC response need testing. Measure MLCC effective capacitance under DC bias and startup behavior. Each rail includes 10 µF bulk capacitance with local 100 nF bypassing at the relevant loads. No speculative antenna-tuning capacitor values were added.

The enclosure leaves only 0.4 mm above the maximum regulator body after its local roof pockets; thermal and mechanical tolerances remain release blockers. See [mechanical fit](../mechanical/mechanical-fit.md).

There is no USB power input. J2 pad 6 is a 3.3 V sense connection, not an adapter power input. Programming fixtures require 3.3 V logic and protection against injecting current while the reader is unpowered. The separate relay coil and switched load are excluded from this reader budget.
