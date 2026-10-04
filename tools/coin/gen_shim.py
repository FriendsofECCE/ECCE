#!/usr/bin/env python3
"""Build an include overlay that maps SGI-style <inv/X/Y.H> includes onto
Coin's <Inventor/X/Y.h>. Headers Coin lacks become empty stubs and are
listed in missing.txt. inv/ChemKit comes from the tree (moiv's own)."""
import os, re, sys
root, out = sys.argv[1], sys.argv[2]
coin = "/usr/include/Inventor"
scan = [os.path.join(root, d) for d in ("src/inv/moiv", "src/inv/wxinv", "include/inv/ChemKit", "include/inv/SoWx")]
# extra roots to scan (headers outside src/inv that include inv/)
scan += sys.argv[3:]
pat = re.compile(r'^\s*#\s*include\s+[<"](inv/[^>"]+)[>"]', re.M)
seen = set()
for d in scan:
    for dp, _, fs in os.walk(d):
        for f in fs:
            if f.endswith(('.C', '.H', '.h', '.c', '.cpp', '.inc')):
                try: txt = open(os.path.join(dp, f), errors='replace').read()
                except OSError: continue
                for m in pat.finditer(txt): seen.add(m.group(1))
missing = []
for inc in sorted(seen):
    if inc.startswith(("inv/ChemKit/", "inv/SoWx/")) or inc == "inv/flclient.h": continue
    rel = inc[len("inv/"):]
    base = re.sub(r'\.[Hh]$', '', rel)
    target = os.path.join(out, "inv", rel)
    os.makedirs(os.path.dirname(target), exist_ok=True)
    c = "Inventor/" + base + ".h"
    if os.path.exists(os.path.join("/usr/include", c)):
        open(target, "w").write('#include <%s>\n' % c)
    else:
        open(target, "w").write('/* not in Coin: %s */\n' % inc)
        missing.append(inc)
os.makedirs(os.path.join(out, "inv"), exist_ok=True)
for n in ("ChemKit", "SoWx", "flclient.h"):   # ours, taken from the tree
    ck = os.path.join(out, "inv", n)
    if not os.path.exists(ck): os.symlink(os.path.join(root, "include/inv", n), ck)
open(os.path.join(out, "missing.txt"), "w").write("\n".join(missing) + "\n")
print(len(seen), "inv/ includes,", len(missing), "not in Coin")
