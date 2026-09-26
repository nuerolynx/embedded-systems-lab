# Remote tamper assembly

The selected switch is **Littelfuse/C&K Y31B13136FPLFG**, also described as **KSC223G LFG**. It is a gold-contact, momentary SPST normally open tactile switch. A supported and correctly adjusted actuator must hold it closed while the case is assembled, then release it to assert tamper when the required cover or mounting interface is removed. That mechanical linkage is not yet designed or verified. It replaces the earlier D2F-01L proposal, whose published microload region did not cover the reader's 3.3 V input.

This is an electrical assembly specification. The remote carrier, retention bracket, plunger, cable routing and mechanical stops remain to be designed and verified. No tamper daughterboard or manufacturer switch CAD is represented as complete.

## Parts and wiring

| Assembly item | Exact selection | Placement |
|---|---|---|
| SW1 | Littelfuse/C&K Y31B13136FPLFG / KSC223G LFG | Remote supported switch carrier |
| RT1 | Yageo RC0603FR-071KL, 1 kΩ, 1%, 0603, 0.1 W | Remote carrier, in series with the switch |
| Two internal conductors | Insulated flexible leads; final gauge, length and strain relief unqualified | Between carrier and main-board J4 |

```text
MAIN BOARD                                         REMOTE ASSEMBLY

+3V3 -- R26 10 kΩ --+-- GPIO36 / TAMPER_N
                    |
                    +-- C13 100 nF -- GND
                    |
                    +-- J4.1 -- RT1 1 kΩ -- [ SW1 normally open ] -- J4.2 -- GND
```

RT1 is external to the main PCB. Fit it in series; never bypass it or put it across the switch. The main-board R26 and C13 values and J4 net connections stay unchanged. RT1 limits C13's discharge current when SW1 closes. This is a short internal harness, not a protected outdoor cable input.

The switch package has duplicated contact terminals. Before wiring, identify its two electrically distinct terminal groups with an ohmmeter and the purchased part's drawing: terminals within a group are already common, while opposite groups become connected only when pressed. Connect one group through RT1 to J4.1 and the other to J4.2. A connection to two terminals of the same group would permanently mask tamper. Do not infer the pairing from an unverified generic four-pin tactile footprint or assign invented terminal numbers. Mechanically support all required solder terminals; leads must not carry the force of the plunger.

## Electrical acceptance

The manufacturer's gold-contact limits are 20 mV minimum voltage and 0.1 mA minimum current, with maxima of 32 V DC, 10 mA and 0.2 VA. The exact selected ordering code and gold plating appear in the manufacturer table. [C&K KSC2 datasheet, revised 2026-01-23](https://www.ckswitches.com/media/1968/ksc2.pdf).

At nominal 3.3 V, the assembled loop carries 3.3 V/(10 kΩ+1 kΩ) = **0.30 mA**, and GPIO36 sits near **0.30 V**. For a 3.0–3.6 V rail and independent 1% resistor tolerances, closed current is approximately **0.270–0.331 mA**, and the largest calculated GPIO LOW is **0.334 V**. These are circuit calculations, not measured values. Both contact load and ESP32 input thresholds have nominal design margin.

At 3.6 V with RT1 at its 990 Ω minimum, the ideal initial capacitor discharge is bounded to about **3.64 mA**, below the switch's 10 mA maximum. This calculation excludes parasitic spikes and requires RT1 to be present. Firmware debounce and contact/ramp tests remain required. [RT1 manufacturer specification](https://yageogroup.com/component-documentation/download/specsheet/RC0603FR-071KL), [ESP32 input limits](https://documentation.espressif.com/esp32-wroom-32e_esp32-wroom-32ue_datasheet_en.html).

## Mechanical acceptance

The nominal switch body is 6.2 × 6.2 mm and the height is 3.5 mm. Its gullwing terminals require more space than the body: the manufacturer's G-termination land diagram shows a **12 mm minimum outer span**, before carrier edge allowance and wiring. A 7 × 9 mm carrier therefore does not accommodate this selected terminal style. The final carrier must also accommodate RT1 and strain relief. The drawing was visually checked in the [manufacturer-authored 2024 sheet, page 2, mirrored by Components101](https://components101.com/sites/default/files/2024-08/ksc2-datasheet.pdf); its 12 mm land span agrees with the current manufacturer document's indexed figure. See the tentative 7 × 13 mm front carrier envelope and its unresolved support/clearance conditions in [mechanical fit](../mechanical/mechanical-fit.md).

The selected force is 2.25 ± 0.55 N; electrical travel is 0.35 mm with +0.25/−0.15 mm tolerance. Electrical travel is not a maximum safe overtravel specification. Use a compliant plunger and a separate hard stop, obtain the supplier's permissible sustained force/overtravel limits, and verify closure and release across enclosure, board, adhesive and temperature tolerances. The switch's component sealing rating does not establish enclosure ingress protection.

Before accepting an assembled unit:

1. With power off and the remote assembly disconnected, confirm it measures about 1 kΩ only while pressed and is open when released. Check both wire-to-terminal paths and strain relief.
2. Power the reader from its normal supply. Confirm GPIO36 reads LOW assembled and HIGH released, with stable transitions and no switch bounce events escaping the firmware debounce.
3. Measure the node voltage and RT1 current transient. Verify tamper is reported on opening or disconnecting either lead. This simple loop cannot distinguish an intentional short from a closed switch.
4. Repeat removal/replacement, temperature and cable-flex testing. Record the actual plunger stop, preload and release margin; no values are released from the nominal dimensions alone.
