# Proposed six-layer nominal stack — fabrication review required

The design decision is six copper layers while retaining a nominal **1.600 mm copper-and-dielectric board thickness**, the existing 36 × 124 mm outline and bottom notch, and the 131.2 × 40.7 × 17.6 mm enclosure. This is an engineering proposal, not a fabricator-approved construction, impedance specification or manufacturing certification. The layer functions below preserve the routing owner's current order.

| Order, front to back | KiCad name / material | Proposed function or construction | Nominal thickness, mm |
|---|---|---|---:|
| L1 | F.Cu | Front components, signals and power; local GND fill | 0.035 |
| D1 | FR-4 dielectric | Prepreg, pressed target | 0.180 |
| L2 | In1.Cu | Existing routing and local GND fill | 0.018 |
| D2 | FR-4 dielectric | Core, dielectric-only target | 0.360 |
| L3 | In2.Cu | Existing routing and local GND fill | 0.018 |
| D3 | FR-4 dielectric | Central prepreg stack, pressed target | 0.378 |
| L4 | In3.Cu | Dedicated GND; retain RF copper keepouts | 0.018 |
| D4 | FR-4 dielectric | Core, dielectric-only target | 0.360 |
| L5 | In4.Cu | New routing layer and local GND fill | 0.018 |
| D5 | FR-4 dielectric | Prepreg, pressed target | 0.180 |
| L6 | B.Cu | Rear components, signals and power; local GND fill | 0.035 |
| **Total** | **Six copper layers plus five dielectric gaps** | **0.142 copper + 1.458 dielectric** | **1.600** |

The copper thicknesses and dielectric sequence are symmetric about the board mid-plane. The exact resin/glass styles, core stock, number of prepreg plies, pressed thicknesses, copper plating allowances, material grade, solder mask and surface finish remain for the fabricator to confirm. **0.378 mm is an arithmetic design target, not a claim that a particular stock prepreg exists.** Solder mask and final surface finish are not budgeted as zero physical thickness: they are outside this laminate/copper arithmetic and must be included in the assembled maximum-thickness envelope. Define where and how finished thickness is measured when placing an order.

The 35 µm outer and approximately 18 µm inner choices are consistent with publicly offered six-layer capabilities; JLCPCB lists 35 µm outer and 17.5 µm inner copper, a 1.6 mm board option, and ±10% thickness tolerance for boards at least 1.0 mm thick. That establishes feasibility of this class of construction, not approval of this exact table. A tighter thickness tolerance must be explicitly agreed if the enclosure requires it. [JLCPCB six-layer capabilities](https://jlcpcb.com/resources/6-layer-pcbs).

Physical symmetry does not establish copper-density balance. The dedicated GND layer and densely routed layers can retain different copper areas; the fabricator should review layer and panel copper balance, bow and twist. Do not add balancing copper inside the RF keepouts. [Eurocircuits copper-distribution guidance](https://www.eurocircuits.com/technical-guidelines/panel-guidelines/copper-distribution-on-a-panel/).

This preserves the routing decision rather than redesigning the layer roles. One dedicated GND layer does not put a close, uninterrupted reference plane beside every signal layer. RF keepouts and via anti-pads still interrupt GND. Review return paths at layer transitions and across the antenna regions; no controlled impedance, RF performance or EMI compliance is claimed. A later layout revision could evaluate a second dedicated GND layer, but that would consume an existing routing layer and is outside this dimensional proposal.

## Mechanical effect and unchanged blockers

With the existing PCB rear datum at z4.0 and the **same actual 1.6 mm board thickness**, the front seating plane remains z5.6. The four Ø2.2 NPTH holes remain at (103,99), (133,99), (103,169), (133,169). No outline, notch, boss, pillar or enclosure geometry change is required by the copper-layer count itself. Board holes and these coordinates were read back from the six-layer scratch base.

Regulator clearance remains 0.4 mm below its local roof pocket, before other tolerances. Buzzer clearance remains 0.6 mm below its rear pocket. Untrimmed LF pins still have a conservative 1.1 mm rear-plate interference; the proposed trim, projecting wall-screw collision, rear package heights, cable bends and unfinished tamper carrier remain unresolved. Native STL compilation does not validate them.

For scale only, if the board grew by +0.16 mm while its rear datum stayed fixed, regulator headroom would fall from 0.40 to 0.24 mm before solder, coating, plastic and assembly variations. This is a sensitivity example, not a complete worst-case stack or an accepted tolerance. Copper layer count cannot be used as evidence that the actual board will meet the enclosure's maximum-thickness requirement.

