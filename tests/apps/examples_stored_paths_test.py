#!/usr/bin/env python3
"""No stored example copy may carry the preparsing machine's paths (#888).

Pure Python: scans data/client/examples/**/stored for home and run-directory
paths.  A copy of the example's own output is exempt; it is the program's
text, identical to the file next to the stored folder.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
EXAMPLES = os.path.join(ROOT, "data", "client", "examples")
BAD = re.compile(rb"/home/|/tmp/ecce_|/andy(?![A-Za-z0-9_])|(?<![A-Za-z0-9_])andy/")


def main():
    bad = []
    for base, _dirs, names in os.walk(EXAMPLES):
        parts = os.path.relpath(base, EXAMPLES).split(os.sep)
        if "stored" not in parts:
            continue
        example = os.path.join(EXAMPLES, *parts[:parts.index("stored")])
        for n in names:
            path = os.path.join(base, n)
            data = open(path, "rb").read()
            m = BAD.search(data)
            if not m:
                continue
            if os.path.exists(os.path.join(example, n)):
                continue
            bad.append("%s: %r" % (os.path.relpath(path, EXAMPLES),
                                   data[max(0, m.start() - 20):m.end() + 30]))
    for line in bad[:30]:
        print("FAIL", line)
    print("%d stored files with machine paths" % len(bad))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
