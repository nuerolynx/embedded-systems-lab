# Hardware safety

These examples control real outputs. A software demonstration is not a substitute for electrical design review, code compliance, or commissioning.

## Power and grounding

- Use a regulated supply sized for peak current, including Wi-Fi, camera, relay-coil, display, and amplifier transients.
- Keep high-current loads off the development board's regulator unless the board and load specifications explicitly allow it.
- Add appropriate local decoupling and keep power paths short.
- Establish the correct common reference for low-voltage signals without creating unsafe ground paths.
- Do not disable the ESP32 brownout detector. Brownout resets indicate a power-integrity problem that should be corrected in hardware.

## Relays and access control

- Development-board relay examples should be treated as low-voltage prototypes.
- Mains voltage requires appropriately rated, enclosed, fused, isolated, and code-compliant equipment installed by a qualified person.
- Select fail-safe or fail-secure operation from the actual risk assessment and applicable egress, fire, building, and accessibility requirements.
- Use listed power supplies and door hardware where the application requires them.
- Provide hardware interlocks and emergency behavior that do not depend on the microcontroller remaining healthy.

## Before deployment

Verify startup state, loss of power, network loss, sensor disconnection, stuck outputs, reboot behavior, simultaneous loads, and recovery after faults. Perform those tests with the real hardware and document the results.
