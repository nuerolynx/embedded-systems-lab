# A1 two-variant requirements — not implemented

This records the owner's next revision request while preserving the completed A0 preliminary design. **No A1 PCB, production firmware, PoE qualification or 24 V approval is claimed. Do not apply 24 V or PoE to A0.**

| Requirement | Pigtail variant | Ethernet/PoE variant |
|---|---|---|
| Network/output | Selectable RS-485 OSDP PD or protected Wiegand | Ethernet TCP/IP; no requirement for legacy panel wiring |
| Input | Nominal 12/24 V DC; operating/transient envelope to be engineered | IEEE 802.3af PD power path; any optional DC feed needs backfeed prevention |
| Cable | Eight-conductor harness | Low-profile Ethernet connection |
| Exit | Single weather-sealed center-rear structural channel; no side/bottom exit | Same center-rear requirement |
| Enclosure target | 116 H × 43 W × 22 D mm | Same target |

The smaller enclosure is a new mechanical design. A0's 124 mm-high PCB cannot fit within a 116 mm-high external enclosure. Its bottom cable notch and current enclosure must be replaced, not relabeled. Connector bend radius, rear gland/gasket, retention, drainage and actual module heights must be modelled and tested; weather sealing is not established by CAD alone.

## Intended electronics and firmware

- Retain ESP32-WROOM-32-family processing. W5500 is a candidate Ethernet controller; LAN8720A is an alternative requiring a different RMII pin/clock design. Neither controller is itself a compliant PoE powered-device front end. Add a qualified 802.3af detection/classification, isolation, conversion and magnetics design.
- Re-engineer the DC protection chain for 24 V plus the chosen supply tolerance and transients. A0's SMBJ18A protection and present input contract do not permit a label-only upgrade. Recalculate regulator, diode, fuse, capacitors, thermal budget and LED/Ethernet peak loads.
- Keep documented HID-capable LF support. RDM6300's EM4100 capability does not establish HID Prox or PDK wristband compatibility; an EM-only economy option must be labeled accordingly.
- Retain an HF reader with complete UID length. Add a VL53L0X/VL53L1X-family ToF sensor over I2C, with optical-window/crosstalk calibration and measured behavior at the requested 20–100 mm gesture range.
- Add a ten-segment WS2812B light bar with an appropriate 3.3-to-5 V data buffer, supply decoupling and a verified current budget. The ESP32 must not receive 5 V. An external relay interface needs a suitable protected level-shifting/driver stage; a GPIO is not a strike power supply.
- Define separate authoritative GPIO maps for both variants, preserving boot states, independent UARTs and bounded shared-bus access.

## Requested application behavior and security requirements

Application modules should isolate `readRFID()`, `readNFC()`, `checkDistance()`, `sendTelemetry()` and `handleNetwork()`. Use bounded queues, deadlines and state machines; avoid blocking acquisition loops and application `delay()` calls. TLS connection work must not stall reader/lockdown servicing.

An in-range hand wave emits an `intent_wave` event with distance and a unique event identifier. It is a request for backend credential evaluation, not an authorization to unlock or proof that a smartphone is present. Backend/mobile credential discovery and cryptographic authentication are separate systems.

LED states: breathing single blue pixel at idle, green sweep on grant, rapid red double-flash on denial, and a latched flashing crimson lockdown override. Only authenticated, authorized commands may change access/lockdown state. Bind grant responses to the originating reader/event, enforce short expiration and replay rejection, and never replay stale grants after reconnect. Lockdown must override offline fallback as well as normal grants.

Stream health every 30 seconds: link uptime, gateway ICMP RTT/timeout, free/minimum heap, calibrated input voltage and sensor status. Internal temperature must be reported only where the selected ESP32/core provides a supported meaningful measurement; otherwise report unavailable or fit an external temperature sensor. Never invent temperature or ping readings.

Use mutually authenticated TLS MQTT or framed TLS TCP with CA/hostname verification, unique device credentials, bounded reconnect backoff and queue policy. Never use an insecure certificate bypass or a fleet-wide embedded secret. Define provisioning, signed updates, rollback, secure boot/flash protection and key rotation before production release.

Up to ten explicitly provisioned offline master identifiers may be held locally, with exact type/length matching, revocation and audit policy. UID-based fallback remains clonable identification, not cryptographic authentication; disable it by default and during lockdown. Bound relay activation independently and assess the real access/egress system separately.

## Completion gates

New native schematics/PCBs and both enclosure variants; component/pin-map review; clean ERC/DRC and power/fit checks; reproducible compiled firmware per profile; PoE/DC, optical, RF, network recovery, authenticated-command, offline-cache and panel tests. A0 check results do not satisfy these A1 gates.
