#!/usr/bin/env python3
# Four vectors over Excited State; the panel derives wavelength from eV.
# String-vector values cannot hold spaces, so labels use underscores and
# transitions use occupied>virtual:coefficient, with 1-based MO numbers.
import re


def emit_excited(states):
    if not states:
        return
    labels = " ".join(str(index) for index in range(1, len(states) + 1))
    for key, values, units in (
        ("EXCIENERGY", [state["ev"] for state in states], "eV"),
        ("EXCIOSC", [state["f"] for state in states], "f"),
        ("EXCISYM", [re.sub(r"\s+", "_", state["label"]) or "-"
                     for state in states], None),
        ("EXCITRANS", [",".join(state["trans"]) or "-" for state in states], None),
    ):
        print(f"key: {key}\nsize:\n{len(states)}\nrowlabels:\n{labels}\nvalues:")
        print(" ".join(values))
        if units is not None:
            print(f"units:\n{units}")
        print("END", flush=True)
