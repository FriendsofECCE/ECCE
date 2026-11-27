#!/usr/bin/env python3
# One number per atom becomes a PropVector<Atom> for the shared charge panel.
# The element position and optional colon vary between the three row layouts:
#   ORCA      "   0 O :   -0.252811"      "   0 H    0.155782    0.000000"
#   Gaussian  "     1  O   -0.169203"
#   NBO (NPA) "    O  1   -0.88855      1.99996 ..."   (symbol first)
import re
import sys


def charge_rows():
    values = []
    for line in sys.stdin:
        match = re.match(r"^\s*\d+\s+[A-Za-z]{1,2}\w*\s*:?\s+(-?\d+\.\d+)", line)
        if not match:
            match = re.match(r"^\s*[A-Za-z]{1,2}\w*\s+\d+\s+(-?\d+\.\d+)\s+-?\d+\.\d+\s", line)
        if match:
            # Keep the captured text so precision and signed zero survive.
            values.append(match[1])
    return values


def emit_charges(key, values):
    if not values:
        return
    print(f"key: {key}\nsize:\n{len(values)}\nvalues:")
    print(" ".join(values))
    print("units:\ne\nEND", flush=True)
