# OpenDual Reader engineering firmware

Arduino-framework firmware for ESP32-WROOM-32E-N4. This is an unbench-tested engineering build, not a certified access-control product. `hardware_contract.json` is the authoritative GPIO source; `scripts/generate_pins.py` generates `include/pins.h` and `../docs/pinout-generated.md` on every build. Runtime UART0 is3.3V only; reader has no USB bridge.

## Build

Install Python3.12 and run `python -m pip install platformio==6.1.18`. From this firmware directory run `python -m platformio run -e opendual`. Package versions and LibOSDP sources are pinned; see THIRD_PARTY.md. Arduino uses the generic `esp32dev` board definition solely for a4MB WROOM-32E target and programming parameters.

The exclusive build environments are:

| Environment | Function |
|---|---|
| `opendual` | OSDP_PD; upstream CP authorizes access; relay never energized |
| `wiegand` | WIEGAND_READER; no local relay; requires explicit supported raw bit-length policy |
| `standalone` | STANDALONE_DEMO; exact whitelist controls separate relay board for3seconds |
| `provision` | Wired key/address provisioning only; no RF acquisition or relay grants |

Do not flash hardware until rail, reset, pin map and driver polarity are checked. Firmware artifacts alone do not establish hardware correctness.

## Secure Channel provisioning

No shared or default SCBK ships with source. Normal OSDP firmware refuses to initialize without a16-byte key and an address0..126. It uses `OSDP_FLAG_ENFORCE_SECURE`, never install mode, and never silently falls back to plaintext. Events are accepted only after Secure Channel is established; pending events flush on SC loss. Device identity/capability negotiation remains visible as required by the protocol.

On an isolated trusted UART0 fixture, flash `provision`, then run `python scripts/provision.py --port COMx --address 1 --panel-record DEVICE.private.json`. The script generates a unique host-CSPRNG key, saves an exclusive private record, sends it over wired UART, and requires an acknowledgement after NVS readback. Transfer that device's key to the CP securely. Flash `opendual` preserving NVS, and verify both successful SC handshake and refusal with a wrong CP key. Never deploy the provisioning image. Operational firmware accepts key rotation only via authenticated OSDP KEYSET and persists it before acknowledging. Runtime UART has no key-setting command.

NVS encryption, flash encryption and secure boot are not enabled by this prototype build: a physically accessible programming header/flash is outside its protection boundary. Product deployment requires a reviewed manufacturing/security lifecycle. Keys are never logged by normal firmware. No IEEE OUI is invented: vendor code0 is a prototype placeholder needing allocation/legitimate vendor registration for production.

## Credential representation and output

HF ISO14443A UID lengths4,7,10bytes are preserved. Card identification does not authenticate a card and does not imply Seos, proprietary HID applications, NFC wallet or MIFARE sector access. PN532 transport uses1MHz SPI mode0 LSB-first, async ACK/response state, SPI readiness polling, bounded frames and recovery timeouts. No IRQ is wired. Passive activation retry is bounded; successive scans are automatic.

LF parser accepts ID-12LA-HE STX/type/space/payload/CR/LF/ETX, 9600 8N1. EM type1 converts the exact10hex characters to all5credential bytes. HID type2 retains the complete type/space/12-or24hex payload as ASCII without guessing the contradictory vendor bit packing. Unknown lengths, overflow and malformed frames are rejected and counted; no checksum is invented. Confirm UART pin9 idle-high with actual module captures.

OSDP reports raw unspecified card data with reader0=HF and reader1=LF. The HF/EM bytes are the complete value; HID raw ASCII is intentionally an engineering payload, requiring explicit CP interpretation before real panel use. This preserves evidence but is **not yet a validated H10301 panel credential mapping**. No claim of tested PDK wristband compatibility is made.

Wiegand defaults to refusing credentials until `ALLOW_HF_RAW_WIEGAND`/`ALLOW_EM_RAW_WIEGAND` is explicitly configured against a panel supporting the entire raw length. Optional raw output emits MSB first,50us sink pulses,2ms bit slots,30ms trailing gap using RMT hardware. HID-to26bit mapping remains blocked pending capture evidence and parity/bit-position validation. Full UIDs are never silently truncated to26bits. The demonstration whitelist is empty by default; populate exact kind/length/bytes after controlled enrollment. UID whitelisting is not cryptographic authentication.

## Scheduling and outputs

Credential FIFO capacity8; event pool8; LF per-loop budget32bytes; LF timeout100ms; duplicate window1.2s; queued credential TTL1.5s. No card-acquisition wait loop or `readString` is used. PN532 transfers are bounded (at most64data bytes); main service targets about1ms and records worst execution duration internally. Measure actual worst-case latency on hardware: flash key persistence and RTOS scheduling can pause the loop. LibOSDP must be refreshed within its timing requirements.

RS485 uses the ESP-IDF UART1 driver half-duplex RTS to DE and /RE, releasing direction after the final stop bit. UART2 RX uses the IDF driver directly so an implicit Arduino default TX pin cannot take GPIO17 away from Wiegand. LF RF remains continuously enabled; LF reset is a manual jumper inRevA. RF coexistence and interleaving are unresolved hardware measurements.

RGB is binary on/off through active-high drivers. GPIO14 generates a4kHz50%duty carrier for the passive piezo; finite CP envelopes are capped at30seconds and unbounded commands are rejected. Relay uses GPIO32 to an external board only, a one-shot3second `esp_timer`, no retrigger extension, and asserted tamper cancels it. Software does not establish a safety-rated maximum if processor hardware fails: use an independently bounded external relay module where required. Outputs also require the hardware pulldowns documented in the pin contract.

Tamper uses an assembled-closed loop to GND and an external pull-up. Use the remote Y31B13136FPLFG / KSC223G LFG gold SPST-NO switch with RT1, a 1 kΩ series resistor from J4.1; J4.2 returns to GND. A supported actuator must hold the switch pressed when assembled and release it for the required removal event. The carrier and actuator are still proposed; see [tamper assembly](../docs/tamper-assembly.md) and [mechanical fit](../mechanical/mechanical-fit.md). Closed input is approximately 0.30 V; release opens the loop. HIGH/open means asserted; the net name TAMPER_N denotes closed-contact low, and GPIO36 has no internal pull-up. Debounce is 30 ms. OSDP reports local tamper and answers status requests. Verify that enclosure opening releases the contact and produces HIGH, and that a cut switch wire also asserts tamper.

