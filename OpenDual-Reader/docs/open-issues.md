# Open issues and release hold

This is an engineering prototype. A populated 3D view shows intended geometry; it does not prove electrical completion or physical fit.

| Priority | Issue | Evidence or work needed |
|---|---|---|
| Blocker | Reference reader unidentified | Manufacturer, model and revision; label photos, both PCB sides, readable IC markings and continuity measurements |
| Blocker | Credential mapping | Exact PDK model, known facility/card number, complete LF UART captures, and the panel's required bit format |
| Review | PCB layout | Final A0 native DRC: 0 violations, 0 unconnected items, 0 schematic/PCB parity issues. Independent layout, return-path and protection-placement review remains required. |
| Blocker | LF footprint fit | Compare a 1:1 land pattern print with the physical module; verify pin spacing, orientation and drill size |
| Blocker | RF coexistence | Measure both fields and read latency in the final enclosure on the intended metal mullion; A0 has no MCU-controlled LF reset |
| Blocker | Mechanical tolerances | Confirm 0.4 mm regulator roof clearance, LF lead trimming, cable bends, strain relief, HF retention and remote tamper actuation |
| Blocker | Rear wall screw clearance | The lower wall slot at enclosure (20.35,112), equivalent to PCB (118,158.4), lies beneath D3 and near Q2. Relocate or verify a flush mounting arrangement; the bare-PCB head-clearance calculation does not clear rear components. |
| Blocker | Remote tamper assembly fit | Y31B13136FPLFG / KSC223G LFG is 3.5 mm high before its carrier, solder and actuator travel, exceeding the 2.0 mm rear gap. The smallest proposed change is a supported 7 × 13 mm front carrier beside U3 at PCB x100.8…107.8, y60…73, retaining the external dimensions. The G-terminal land span rules out the earlier 7 × 9 mm suggestion. A nominal 4.5 mm carrier/solder/switch stack leaves 6.1 mm of front space before supports and tolerances. Complete the land pattern, 1 kΩ resistor, retention, dielectric separation and compliant actuator/hard stop; verify RF effects and the required removal event. This is a proposal, not modelled or physically demonstrated fit. See the mechanical fit study. |
| Blocker | Power and heat | Measure startup, peak and continuous current, PTC temperature derating, ripple and sealed-case temperature at 9/12/16 V |
| Blocker | Panel interoperability | Verify Secure Channel with correct/wrong keys, full UID lengths, HID interpretation and Wiegand timing/voltage |
| Blocker | Protection placement and validation | D2/D4/D5 are at PCB y97, 64.5 mm longitudinally from cable pads J1 at y161.5 because package fit forced the row above the LF module. Review suppressor placement/short return paths near J1, or a separate supported cable-entry protection assembly, before fabrication. DRC does not validate trace inductance or connector clamp performance. Test final harness surge/ESD response; a fault-tolerant transceiver does not make termination safe under sustained miswiring. |
| Open | HF lifecycle | PN532 is NRND; a PN7160 successor needs separate firmware and RF work |
| Open | Runtime firmware | Fault-inject parsers/queues, measure loop latency and heap, verify cutoff timing and key persistence |
| Open | 3D fidelity | RF models are dimension-based proxies; internal module circuitry, cables and the remote tamper carrier/plunger are not manufacturer CAD |
| Open | Optional panel inputs | Separate external LED/buzzer control inputs are omitted; OSDP commands control status outputs |
| Open | Manufacturing | Lock fabricator stackup and 0.20 mm drilled-hole capability, stencil, assembly sequence, fixture and test plan |

See `validation-results.md` for the latest actual check counts. No physical test results are implied by ERC, DRC, firmware compilation or rendering.

The proposed front tamper carrier and the cable-entry protection limitation are detailed in the [mechanical fit study](../mechanical/mechanical-fit.md).

