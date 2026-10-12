#!/usr/bin/env python3
"""Fails when src/qm/input.cpp accepts a keyword that verifyinput's
ecce-qm checker does not list (or the reverse)."""
import os, re, sys
root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
cpp = open(os.path.join(root, "src/qm/input.cpp")).read()
body = cpp[cpp.index('if (k == "title")'):cpp.index("unknown keyword")]
real = set(re.findall(r'k == "([a-z_]+)"', body))
pl = open(os.path.join(root, "scripts/parsers/verifyinput")).read()
m = re.search(r"my %keyword = map \{ \$_ => 1 \}\s+qw\((.*?)\);", pl, re.S)
checker = set(m.group(1).split())
if real != checker:
    print("input.cpp only:", sorted(real - checker))
    print("verifyinput only:", sorted(checker - real))
    sys.exit(1)
print("ok", len(real), "keywords")
