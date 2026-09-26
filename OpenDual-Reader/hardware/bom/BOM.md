# OpenDual Reader A0 BOM

Prototype selections; no purchasing or fabrication release. PCB solder lands are fabricated features, not purchased connectors.

|Ref|Value|MPN|Notes|
|---|---|---|---|
|J1|PANEL CABLE|PCB solder lands / 6 conductors|1 +9..16V,2 GND,3 A+,4 B-,5 D0,6 D1|
|F1|0.75A PTC|1812L075/33DR||
|D1|B340A-13-F|B340A-13-F|40V Schottky reverse polarity series diode|
|D2|SMBJ18A|SMBJ18A|18V standoff; 29.2V clamp at rated pulse|
|C1|10u/50V|GRM32ER71H106KA12L||
|C2|100n/50V|GRM188R71H104KA93D||
|U1|TSR 1-2433|TSR 1-2433||
|U2|TSR 1-2450|TSR 1-2450||
|C3|10u/10V|GRM188R61A106KE69D||
|C4|10u/10V|GRM188R61A106KE69D||
|U3|ESP32-WROOM-32E-N4|ESP32-WROOM-32E-N4||
|R1|10k|RC0603FR-0710KL||
|C5|10u/10V|GRM188R61A106KE69D||
|C6|100n/50V|GRM188R71H104KA93D||
|C7|1u/10V|GRM188R61A105KA61D||
|R2|10k|RC0603FR-0710KL||
|JP1|RESET short|PCB service shorting pads||
|JP2|BOOT short|PCB service shorting pads||
|J2|PROGRAM 3.3V LOGIC|PCB solder lands / pogo fixture|3V3 pin is sense-only; do not inject external power|
|U4|ID-12LA-HE|ID-12LA-HE|Internal antenna. Pin9 MCU UART; 3/4 do not connect; sample verify before fab|
|R3|10k|RC0603FR-0710KL||
|C8|100n/50V|GRM188R71H104KA93D||
|C9|10u/10V|GRM188R61A106KE69D||
|JP3|LF reset short|PCB service shorting pads||
|J3|PN5321 MINI V2 HARNESS|PCB solder lands; module SKU NFC_EX_MINI|Map to module MX1.25 pins1..7. Supplied harness identity/continuity must be verified|
|R4|10k|RC0603FR-0710KL||
|R5|10k|RC0603FR-0710KL||
|C10|10u/10V|GRM188R61A106KE69D||
|C11|100n/50V|GRM188R71H104KA93D||
|U5|THVD2410DR|THVD2410DR||
|R6|100k|RC0603FR-07100KL||
|C12|100n/50V|GRM188R71H104KA93D||
|R7|120|CRCW1206120RFKEAHP|120ohm termination >=0.5W; inspect thermal conditions|
|JP4|TERM CLOSE AT END|PCB solder jumper / default OPEN||
|D3|SM712-02HTG|SM712-02HTG||
|R8|2.2k|RC0603FR-072K2L||
|R9|100k|RC0603FR-07100KL||
|Q1|MMBT3904LT1G|MMBT3904LT1G||
|R10|47|RC0603FR-0747RL||
|D4|SMBJ15A|SMBJ15A|Littelfuse unidirectional TVS: cathode to panel line, anode GND|
|R11|2.2k|RC0603FR-072K2L||
|R12|100k|RC0603FR-07100KL||
|Q2|MMBT3904LT1G|MMBT3904LT1G||
|R13|47|RC0603FR-0747RL||
|D5|SMBJ15A|SMBJ15A|Littelfuse unidirectional TVS: cathode to panel line, anode GND|
|D6|RGB 150141M173100|150141M173100||
|R14|2.2k|RC0603FR-072K2L||
|R15|100k|RC0603FR-07100KL||
|Q3|MMBT3904LT1G|MMBT3904LT1G||
|R16|330|RC0603FR-07330RL||
|R17|2.2k|RC0603FR-072K2L||
|R18|100k|RC0603FR-07100KL||
|Q4|MMBT3904LT1G|MMBT3904LT1G||
|R19|220|RC0603FR-07220RL||
|R20|2.2k|RC0603FR-072K2L||
|R21|100k|RC0603FR-07100KL||
|Q5|MMBT3904LT1G|MMBT3904LT1G||
|R22|220|RC0603FR-07220RL||
|R23|2.2k|RC0603FR-072K2L||
|R24|2.2k|RC0603FR-072K2L|Strong pulldown overrides GPIO14 reset weak pullup|
|Q6|MMBT3904LT1G|MMBT3904LT1G||
|R25|1k|RC0603FR-071KL||
|BZ1|PKMCS0909E4000-R1|PKMCS0909E4000-R1|Passive piezo 4kHz / provisional backside fit|
|J4|TAMPER CASE LOOP|PCB solder lands / remote KSC223G LFG assembly|Remote Y31B13136FPLFG SPST-NO plus 1k series resistor; pressed closed, released open; carrier and actuator fit unresolved|
|R26|10k|RC0603FR-0710KL||
|C13|100n/50V|GRM188R71H104KA93D||
|R27|100k|RC0603FR-07100KL||
|J5|DEMO RELAY LOGIC|PCB solder lands|Logic only3.3V. No coil/lock supply on reader.|
|RF1|HF module + supplied ferrite antenna|ELECHOUSE NFC_EX_MINI / hardware V2|Purchased closed-ROM PN5321 MINI; revision must be verified|
|SW1|Remote tamper SPST-NO gold tactile|Y31B13136FPLFG|C&K KSC223G LFG; pressed closed; 1k series resistor required; carrier and preload unresolved|
|RT1|1k remote tamper series resistor|RC0603FR-071KL|Yageo 1%; limits C13 discharge; install between J4.1 and SW1|
