"""Generate a unique key and provision a physically connected engineering reader.
Never run against a production reader without an authorized maintenance window.
Requires pyserial==3.5; flash the special provision image before running.
"""
import argparse
import json
import secrets
import time
from pathlib import Path
import serial

p = argparse.ArgumentParser()
p.add_argument("--port", required=True)
p.add_argument("--address", type=int, required=True)
p.add_argument("--panel-record", type=Path, required=True,
               help="New private JSON file for this device's CP key; never commit it")
a = p.parse_args()
if not 0 <= a.address <= 126:
    p.error("address must be 0..126")
# Exclusive creation avoids overwriting an existing panel's key record.
key = secrets.token_hex(16)
with a.panel_record.open("x", encoding="utf-8") as f:
    json.dump({"address": a.address, "baud": 38400, "scbk": key,
               "state": "generated; provisioning outcome must be confirmed"}, f, indent=2)
with serial.Serial(a.port, 115200, timeout=1, write_timeout=1) as uart:
    time.sleep(2)
    uart.reset_input_buffer()
    uart.write(f"KEY {a.address} {key}\n".encode("ascii"))
    deadline = time.monotonic() + 5
    success = False
    while time.monotonic() < deadline:
        response = uart.readline().decode("ascii", errors="replace").strip()
        if response.startswith("PROVISIONED:"):
            success = True
            break
    if not success:
        raise SystemExit("No confirmed provision response; preserve record and investigate. Normal image remains fail-closed.")
print("Provisioned. Store panel record securely; load key into CP; flash normal image without erasing NVS.")
