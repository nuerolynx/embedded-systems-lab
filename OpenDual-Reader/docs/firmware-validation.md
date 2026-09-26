# Firmware validation — 26 September 2026

## Scope and evidence

Arduino ESP32 firmware was cross-compiled for the 4 MB ESP32-WROOM-32E target using PlatformIO Core 6.1.18, espressif32 platform 6.10.0, Arduino ESP32 framework 2.0.17, GCC 8.4.0+2021r2-patch5, Python 3.12.14 on Windows. Exact packages are pinned in `firmware/platformio.ini`; Python build-tool packages are frozen in `firmware/requirements-build.txt`. LibOSDP 3.2.6 is vendored with upstream commit hashes and local port changes documented in `firmware/THIRD_PARTY.md`.

Full compiler evidence and binary hashes are in `firmware/build-artifacts/build.log` and `firmware/build-artifacts/SHA256SUMS.txt`. These binaries are engineering outputs; no reader was flashed. The first compile attempt hit the sandbox's temporary-directory permission restriction and its compiler error dialogue. It was terminated; configuring TEMP/TMP under the workspace allowed subsequent builds to complete. The final build evidence is the applicable result.

| Final environment | Result | Static RAM bytes | Firmware flash bytes |
|---|---|---:|---:|
| opendual (OSDP_PD) | Compile/link PASS | 24,860 | 325,077 |
| wiegand | Compile/link PASS | 24,156 | 286,369 |
| standalone | Compile/link PASS | 23,396 | 280,045 |
| provision | Compile/link PASS | 21,544 | 299,937 |

Reported target budgets are 327,680 bytes RAM and 1,310,720 bytes application flash. Runtime heap/stack high-water marks have not been measured. Final log reports 4 successful environments in 1 minute 57.644 seconds, with no compiler warnings/errors in that run.

## Checks actually performed

- Generated GPIO definitions directly from the authoritative hardware contract. Generator assertions reject duplicate application GPIOs, all boot-strapping/flash pins as application signals, and outputs on input-only pins.
- Verified three mutually exclusive mode compile configurations plus the separately built wired provisioning image. No runtime automatic mode downgrade exists.
- Cross-compiled and linked the real ESP32 firmware, including LibOSDP PD integration and selected peripheral drivers. No mock firmware libraries were substituted.
- Checked the OSDP ELF symbol table: the local `osdp_fill_random`, `esp_fill_random`, `bootloader_random_enable`, local monotonic clock, LibOSDP PD setup, and Espressif AES-CBC/ECB implementations are linked. The vendored PlatformIO source filter excludes libc-rand TinyAES and the upstream millisecond wrapper.
- Python syntax compilation passed for the pin generator and provisioning helper. The provisioning helper was not connected to hardware or used to create any device key.
- Reviewed pinned LibOSDP source for ENFORCE_SECURE processing, event ownership, KEYSET persistence callback behavior, and local tamper report layout. This is source review, not a Secure Channel interoperability test.
- Corrected the UART2-default-TX conflict by using an RX-only ESP-IDF UART setup; corrected tamper to HIGH/open asserted with an assembled-closed loop toward GND; retained TAMPER_N as the hardware net name. The selected remote Y31B13136FPLFG / KSC223G LFG SPST-NO switch is held pressed when assembled and uses a remote 1 kΩ series resistor. This preserves the firmware's LOW-assembled/HIGH-released behavior; physical actuation remains unverified.

## Not performed / release blockers

| Validation | Status |
|---|---|
| C++ parser/queue runtime unit tests | Not executed on a host or ESP32 |
| Device flashing and startup/reset output measurements | Not performed |
| LF EM/HID and PDK wristband captures | Not performed; exact PDK model absent |
| HID raw packing, 26-bit extraction and parity validation | Unresolved; conversion disabled |
| PN532 actual breakout reset, SPI frames, UID reads and recovery | Not bench tested |
| Simultaneous LF/HF RF operation | Not measured; LF reset is manual in Rev A |
| RS-485 direction timing, polarity and termination | Not measured |
| Secure Channel positive/negative-key tests, reconnect and power-loss key rotation | Not tested against a CP |
| Real panel interpretation of all UID lengths / LF ASCII payload | Unvalidated |
| Wiegand 50 µs / 2 ms timing and panel accepted formats | Unmeasured; raw conversion opt-ins default off |
| Relay cutoff, tamper debounce, piezo carrier, worst-loop latency | Unmeasured |
| Secure boot, flash/NVS encryption and physical key-extraction resistance | Not enabled / not assessed |

Compile success validates source/toolchain integration only. It does not establish electrical operation, credential interoperability, timing, RF performance or access-control security. The empty demonstration whitelist and disabled Wiegand mappings intentionally prevent unsupported grants/format conversion until enrollment and panel-format decisions are made.

## Bench acceptance sequence

First confirm GPIO voltages, hardware pulldowns, HF-CS pullup and tamper contact polarity with the ESP32 held in reset. Power up without a key and verify OSDP credentials/commands remain blocked. Provision a unique device key on an isolated UART fixture and verify correct-key acceptance plus wrong-key rejection, unplug/reconnect and reboot persistence. Capture ID-12LA-HE frames for labeled EM and H10301 credentials before writing any 26-bit decoder. Validate PN532 UIDs of every length offered by the test credentials, with LF simultaneously energized. Then measure Wiegand pulses, RS-485 turnaround, finite LED/buzzer patterns, 3-second relay cutoff and open/cut-wire tamper. Repeat with the actual enclosure, mullion and cable. Record measured results separately; none are implied here.
