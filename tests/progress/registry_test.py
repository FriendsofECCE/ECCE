#!/usr/bin/env python3
"""Every code that runs jobs has [PROGRESS] entries in its parse descriptor
(#970), so a new code cannot ship without a Progress box.

The codes are read from the registry, not listed here: each <ResourceType>
of ResourceDescriptor.xml and ResourceDescriptorRxn.xml that has a Launcher
tool names its code's EDML (<DescriptorFile>), and the EDML names the
descriptor (<ParseSpecification>).  Retired codes (Gaussian-03/-98/-09,
GAMESS-UK, Amica, MetaDyn) are not registered there, and the import
descriptors are no ResourceType's, so neither is checked.
"""

import os
import re
import sys
import xml.etree.ElementTree as ET

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
CONFIG = os.path.join(ROOT, "data", "client", "config")
CAP = os.path.join(ROOT, "data", "client", "cap")
PARSERS = os.path.join(ROOT, "scripts", "parsers")

# Registered codes still without entries, each with the reason; empty this
# as they get them.  NWChemMD: no real NWChem MD output to write them from
# yet (#970).
PENDING = {"NWChemMD.edml": "no real NWChem MD output yet (#970)"}


def registered():
    """{edml file: [resource type names]} for every type with a Launcher."""
    codes = {}
    for name in ("ResourceDescriptor.xml", "ResourceDescriptorRxn.xml"):
        root = ET.parse(os.path.join(CONFIG, name)).getroot()
        for rt in root.iter("ResourceType"):
            edml = (rt.findtext("DescriptorFile") or "").strip()
            launches = any(t.get("name") == "Launcher" for t in rt.iter("Tool"))
            if edml and launches:
                codes.setdefault(edml, set()).add(rt.get("name"))
    return codes


def main():
    failures = []
    codes = registered()
    if len(codes) < 5:
        failures.append("only %d registered codes found; registry not read?" % len(codes))
    for edml, types in sorted(codes.items()):
        text = open(os.path.join(CAP, edml), encoding="utf-8").read()
        m = re.search(r"<ParseSpecification>\s*([^<\s]+)\s*</ParseSpecification>", text)
        if not m:
            failures.append("%s: no ParseSpecification" % edml)
            continue
        desc = m.group(1)
        with open(os.path.join(PARSERS, desc), encoding="utf-8", errors="replace") as fh:
            n = sum(1 for line in fh if line.strip() == "[PROGRESS]")
        if edml in PENDING:
            ok = n == 0
            print("%s %s (%s): pending, %s" % ("ok   " if ok else "FAIL ", edml, desc,
                                                PENDING[edml]))
            if not ok:
                failures.append("%s now has [PROGRESS] entries: remove it from PENDING" % edml)
            continue
        print("%s %s (%s): %d [PROGRESS] entries" % ("ok   " if n else "FAIL ", edml, desc, n))
        if not n:
            failures.append("%s (%s, used by %s) has no [PROGRESS] entry"
                            % (desc, edml, ", ".join(sorted(types))))
    for edml in PENDING:
        if edml not in codes:
            failures.append("PENDING names %s, which is not a registered code" % edml)
    if failures:
        print("FAILED: " + "; ".join(failures))
        return 1
    print("all %d registered codes have progress entries" % (len(codes) - len(PENDING)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
