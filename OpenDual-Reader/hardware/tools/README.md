# GPIO and connectivity check

Run this script using KiCad 10's bundled Python, which provides `pcbnew`. It checks the authoritative firmware GPIO contract against the design data, then compares exported schematic pin nets with the native PCB. Failure returns a nonzero status and writes `hardware/exports/contract-validation.json`.

After editing the schematic, first refresh `hardware/exports/schematic.net.xml` with:

```text
kicad-cli sch export netlist --format kicadxml --output hardware/exports/schematic.net.xml hardware/kicad/OpenDual-Reader.kicad_sch
```

From this project's root, run `hardware/tools/verify_contract.py` with KiCad's Python executable. Also run the native ERC and DRC with schematic parity; this script is an additional GPIO/pad check, not a replacement.
