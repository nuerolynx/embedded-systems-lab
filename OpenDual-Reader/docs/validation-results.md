# Validation results — OpenDual Reader A0

Engineering prototype, checked with KiCad 10.0.6 on 26 September 2026. **Not released for fabrication or field installation.** Automated checks do not establish RF performance, enclosure fit, surge protection or access-control interoperability.

| Validation category | Actual result | Evidence |
|---|---|---|
| Documentation verified | Key IC/module pins, rails, interfaces and package choices reviewed against cited manufacturer material; incomplete qualifications are listed separately | [Component review](component-review.md), [sources](datasheet-references.md) |
| Main schematic ERC | 0 errors, 0 warnings | [ERC report](../hardware/exports/erc.rpt) |
| Main PCB DRC | 0 violations; 0 unconnected items | [DRC report](../hardware/exports/drc.rpt), [machine-readable report](../hardware/exports/drc.json) |
| Main schematic/PCB parity | 0 issues | Included in the final DRC run |
| GPIO and pad mapping | 19 GPIO assignments match; 233 pad comparisons; 0 mismatches | [Contract check](../hardware/exports/contract-validation.json) |
| Local 3D assets | 61 model references; 0 missing files | [Model check](../hardware/exports/model-validation.json) |
| Optional relay board | ERC 0; DRC 0; unconnected 0; parity 0 | [Relay reports](../hardware/kicad/relay-demo/validation/DRC-parity.json) |
| Firmware compiled | All four environments compile and link: OSDP_PD, WIEGAND_READER, STANDALONE_DEMO and wired provisioning | [Firmware results](firmware-validation.md), [build log](../firmware/build-artifacts/build.log) |
| Mechanical validation | OpenSCAD 2021.01 compiled the cover and backplate as simple solids; both exported meshes are closed, with one connected component and no boundary/nonmanifold edges. Assembly fit remains an analytic study | [Solid check](../mechanical/solid-validation.json), [fit study](../mechanical/mechanical-fit.md) |
| Bench tested | Not performed | No device was assembled, powered or flashed |
| RF measured | Not performed | No tuning values or read range measured |
| Panel interoperability tested | Not performed | No OSDP control panel or Wiegand panel connected |

## What was checked

The native project contains a six-page schematic and a six-layer, 1.6 mm PCB, 36 × 124 mm with its 12 × 10 mm cable notch. The final board contains 74 footprints, 1712 track segments and 134 vias. The footprint count includes 68 electrical parts, four mounting holes and two mechanical RF reservations. Harness solder lands and jumpers are copper features rather than separate missing component models. The purchased HF module, remote tamper switch and its series resistor are separate BOM entries.

The final DRC uses the project rules and includes schematic parity. Copper clearance is 0.20 mm; copper-to-edge is 0.30 mm; routed signal width is 0.20 mm; routing vias are 0.60/0.30 mm. Power routing targets 0.50 mm trunks and permits short 0.30 mm pad escapes; a separate per-net width audit confirms no power segments narrower than 0.30 mm in the final board. Actual widths and routed lengths are in [the validation summary](../hardware/exports/validation-summary.json). The project's 0.20 mm minimum through-hole rule accommodates the ESP32 footprint's small thermal holes and requires confirmation with the selected fabricator. GND pours appear on all six layers. In3.Cu is reserved for GND; the other layers contain routing, and the RF keepouts apply to every layer. Continuity checks do not prove acceptable RF return paths or power impedance.

JP2.2 and U1.2 use solid ground connections where surrounding routing constrained thermal spokes. This changes the copper connection rather than suppressing a thermal check; assembly solderability still requires review.

No individual DRC violations were excluded. The inherited project settings leave these check categories disabled: `footprint_filters_mismatch`, `footprint_type_mismatch`, `missing_courtyard`, `track_not_centered_on_via`, `tuning_profile_track_geometries`. They were not changed to clear a discovered violation. Board-only attributes identify the four mounting holes and two mechanical reservations; these are not missing electrical schematic components. The 19 intentionally unused MCU/LF pads retain schematic NC markers and unique isolated PCB nets. MPN fields are synchronized between schematic and PCB. KiCad's normal nonessential ERC check settings are listed at the end of the ERC report; they were not used to waive a discovered electrical fault.

The native files loaded in the installed KiCad application. The populated 3D viewer was opened live. KiCad CLI generated front/back 3D renders, copper-layer PDF, assembly drawings, schematic PDF and placement data. Local model paths are portable. RF assembly and fuse models include explicitly documented simplified envelopes; vendor module internals, the harness and remote tamper mechanism are not reconstructed. See [library provenance](../hardware/libraries/README.md).

OpenSCAD also generated [front-cover STL](../mechanical/front-cover.stl), [backplate STL](../mechanical/backplate.stl) and an [exploded preview](../mechanical/enclosure-exploded.png). The exported mesh bounds retain the 40.7 × 131.2 × 17.6 mm assembled exterior. The enclosure preview uses coarse component envelopes and omits small rear parts; successful solid compilation is not complete interference analysis or production fit approval.

Initial/placement DRC reports retained in the folder are historical; `drc.json` and `drc.rpt` are the applicable final reports. A separate local `GUI-before-routing.kicad_pcb` backup preserves the earlier live editor state and is excluded from the delivery ZIP.

## Release holds and remaining measurements

1. Identify the reference reader and exact PDK credential model. Supply PCB-side photographs, readable IC markings, continuity measurements and known credential values; the replacement circuit is independent, not a reconstructed reference schematic.
2. Confirm LF footprint and module orientation against a physical sample. Capture EM/HID UART frames and establish HID bit order/parity before enabling any 26-bit Wiegand mapping. Preserve full 4/7/10-byte HF UIDs.
3. Measure LF/HF coexistence, latency and field strength in the final plastic housing on the actual metal mullion, with production cable/ferrite/fasteners. Interleaved fields would require an MCU-controlled LF-reset ECO; A0 cannot provide that without modification.
4. Resolve the enclosure tolerance stack: regulator roof gap, LF lead trimming, rear component/fastener clearance, cable bends and strain relief, module retention and tamper actuation. The requested exterior dimensions have not been enlarged. See the fit study for the proposed local pockets and other necessary changes.
5. Redesign and review the cable-entry protection placement/return path: the current D2/D4/D5 bank is approximately 64.5 mm longitudinally from J1, which is an explicit layout limitation despite clean geometric DRC. Qualify an entry-protection assembly or improved placement before manufacture. Measure startup/reset states, rail transients, worst-case current, PTC derating and sealed-case temperature at 9/12/16 V. Test actual harness ESD/surge response; a transceiver fault rating is not a system miswiring rating.
6. Test firmware on hardware: parser faults, queue pressure, loop timing, full UID delivery, relay cutoff, buzzer timing, key persistence, correct/wrong Secure Channel keys and reconnect behavior. Wiegand output mappings and the standalone whitelist default to disabled/empty.
7. Confirm purchased capacitor specifications, contact microload suitability, production stackup, assembly process and fabricator capabilities. Complete an independent electrical/RF/layout review before manufacture.

The selected remote tamper assembly uses the gold SPST-NO KSC223G LFG (exact order code Y31B13136FPLFG), pressed closed by the backplate, with a 1k series resistor to limit C13 discharge. Released/cut-wire HIGH asserts tamper; pressed input is approximately 0.30 V. This replaces the originally considered D2F-01L, whose published microload graph did not qualify the 3.3 V operating point. The switch carrier, controlled preload and physical actuation remain to be validated. See [tamper assembly](tamper-assembly.md).

See [open issues](open-issues.md) and [bring-up procedure](bringup.md). No board was ordered, component purchased or device flashed. GitHub publication was explicitly requested by the project owner.
