# Reference evidence and independent replacement

No hardware reference was attached. The available attachment is a design brief. The exact manufacturer, model, PCB revision, and enclosure internals of the intended reference are **unknown**. No circuit in this project is represented as a recovered reference schematic.

| Classification | Evidence available | Conclusion / action |
|---|---|---|
| Observed reference component | None | No component markings or packages can be identified. |
| Observed reference connection | None | No continuity, trace, or net reconstruction is possible. |
| Observed mechanical requirement | User brief specifies 131.2 × 40.7 × 17.6 mm external target | These are an original-design envelope, not measured available PCB dimensions. The proposed enclosure and carrier geometry must be checked as a complete assembly. |
| Datasheet-supported LF conclusion | ID Innovations dual-reader documentation | ID-12LA-HE is a defensible purchased LF starting point. Its firmware is closed. Supported short HID formats do not establish universal HID support. |
| Datasheet-supported credential conclusion | ProdataKey WBC65/WBC75 documentation | These exact products are suitable compatibility candidates. The user's wristband has not been identified. A read test is still required. |
| Datasheet-supported HF conclusion | ELECHOUSE MINI hardware V2 manual and NXP PN532 documentation | A separately mounted module and antenna provide a compact HF implementation. Manufacturer radio and matching circuitry remain inside the purchased assembly. |
| Proposed replacement circuitry | OpenDual carrier schematic, PCB and firmware | Independently engineered power, ESP32, panel interfaces and user indicators. Circuit provenance is this design, not a teardown. |
| Unknown reference power path | No powered measurements or net photographs | Determine input range, idle/peak current and regulator markings before attempting a reference comparison. |
| Unknown reference antennas | No coil dimensions, turn counts, inductance or matching values | Do not derive a replacement coil from external enclosure dimensions. |
| Unknown reference protocol | No captured serial/Wiegand/RS-485 data | Determine credential outputs and panel behavior using authorized test fixtures. |

## Evidence needed for a future comparison

1. Sharp front, back, label and cable photographs, including every model/revision mark; photographs of both PCB faces with a scale.
2. Enclosure internal dimensions, screw locations, wall thicknesses, cable-exit geometry, and clearance from PCB to front/back surfaces.
3. Unpowered continuity measurements for power return, protection devices, regulators, MCU/module connections, reader antenna connections and output drivers. Record instrument polarity when semiconductors are involved.
4. Component top markings and close-ups of RF passives, antenna terminals, crystals and reader ICs.
5. Current-limited operating measurements: input and regulated rails, idle/read current, RF frequency, and captured output waveforms on a test controller.
6. Exact credential model/marking and known facility/card values for test samples. Keep the full transmitted bit sequence and UID length.

No photo or measurement request blocks the independent prototype files. It does block claims that this design recreates the reference's circuitry, mechanical fit, or compatibility.
