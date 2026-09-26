"""Generate every firmware GPIO definition from the shared hardware contract."""
import json
from pathlib import Path
if "__file__" in globals():
    ROOT = Path(__file__).resolve().parents[1]
else:
    Import("env")
    ROOT = Path(env.subst("$PROJECT_DIR"))
def generate():
    c = json.loads((ROOT / "hardware_contract.json").read_text())
    pins = c["pins"]
    assert len({p["gpio"] for p in pins}) == len(pins), "GPIO conflict"
    assert not {p["gpio"] for p in pins} & {0,2,5,6,7,8,9,10,11,12,15}
    assert all(p["direction"] == "in" for p in pins if p["gpio"] >= 34)
    header = "// GENERATED from hardware_contract.json; do not edit.\n#pragma once\nnamespace Pins {\n"
    header += "".join(f'constexpr int {p["name"]} = {p["gpio"]};\n' for p in pins)
    header += "constexpr int LF_RESET = -1;\n}\n"
    (ROOT / "include/pins.h").write_text(header)
    md = "# Generated GPIO contract\n\nSource: `firmware/hardware_contract.json`.\n\n| GPIO | Net | Firmware name | Function | Startup |\n|---|---|---|---|---|\n"
    md += "".join(f'| {p["gpio"]} | {p["net"]} | {p["name"]} | {p["function"]} | {p["startup"]} |\n' for p in pins)
    (ROOT.parent / "docs/pinout-generated.md").write_text(md)
generate()
