#!/usr/bin/env python3
# One number per atom becomes a PropVector<Atom> for the shared charge panel.
# The element position and optional colon vary between the three row layouts:
#   ORCA      "   0 O :   -0.252811"      "   0 H    0.155782    0.000000"
#   Gaussian  "     1  O   -0.169203"
#   NBO (NPA) "    O  1   -0.88855      1.99996 ..."   (symbol first)
import re
import sys


# Perl matched bytes, so \s, \d and \w are ASCII there: without re.ASCII a row
# led by a UTF-8 no-break space, or carrying a non-ASCII digit or letter, is a
# data row to Python and not to Perl, and the charge count silently changes.
def charge_rows():
    values = []
    # The Perl read job output as bytes; a stray byte must not crash the parse.
    sys.stdin.reconfigure(encoding="utf-8", errors="replace")
    for line in sys.stdin:
        match = re.match(r"^\s*\d+\s+[A-Za-z]{1,2}\w*\s*:?\s+(-?\d+\.\d+)", line,
                          re.ASCII)
        if not match:
            match = re.match(r"^\s*[A-Za-z]{1,2}\w*\s+\d+\s+(-?\d+\.\d+)\s+-?\d+\.\d+\s",
                              line, re.ASCII)
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
