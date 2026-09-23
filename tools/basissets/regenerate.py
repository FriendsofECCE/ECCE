#!/usr/bin/env python3
"""
Regenerate one basis set's data file from the Basis Set Exchange.

Step 4 of the re-sourcing plan (issue #116).  Writes into a STAGING
directory; nothing in the repository or the live library is touched until
the result has been adjudicated and applied deliberately.

Single-file basis sets only.  ECCE composes aggregates (aug-cc-pVDZ is a
base plus a diffuse component plus the assembled file) and BSE ships only
the complete set, so decomposing one into ECCE's parts is guesswork.  The
structure decision was to KEEP ECCE's layout, so composed sets are left
alone rather than restructured.

    ./regenerate.py "Ahlrichs VDZ" --staging /tmp/staged
"""

import argparse
import json
import os
import sys
import urllib.parse
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from bse2bas import convert, convert_ecp                        # noqa: E402
from diffreport import parse_bas, parse_text, SYM               # noqa: E402

LIB = os.path.expanduser("~/.ECCE/dataserver/htdocs/Ecce/system/"
                         "GaussianBasisSetLibrary")
REPO_LIB = os.path.join(os.path.dirname(os.path.dirname(HERE)),
                        "data", "admin", "basissets")


def fetch(bse_name):
    url = ("https://www.basissetexchange.org/api/basis/%s/format/json/"
           % urllib.parse.quote(bse_name))
    with urllib.request.urlopen(url, timeout=120) as response:
        return json.load(response)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("basis")
    ap.add_argument("--staging", default="/tmp/ecce-basis-staging")
    ap.add_argument("--ecp", action="store_true",
                    help="regenerate a .POT rather than a .BAS")
    args = ap.parse_args()

    baseline = json.load(open(os.path.join(HERE, "baseline.json")))
    namemap = json.load(open(os.path.join(HERE, "namemap.json")))

    entry = None
    for entries in baseline.values():
        if args.basis in entries:
            entry = entries[args.basis]
    bse_name = None
    for bucket in ("direct", "alias", "curated"):
        if args.basis in namemap[bucket]:
            bse_name = namemap[bucket][args.basis]
    if entry is None or bse_name is None:
        print("unknown or unresolved basis: %s" % args.basis, file=sys.stderr)
        return 2
    if len(entry["files"]) > 1:
        print("%s is a COMPOSED set (%d files); ECCE's structure is kept, so "
              "its parts are regenerated individually, not the composite"
              % (args.basis, len(entry["files"])), file=sys.stderr)
        return 3

    filename = entry["files"][0]
    data = fetch(bse_name)
    text = convert_ecp(data, SYM) if args.ecp else convert(data, SYM)
    if not text.strip():
        print("BSE returned nothing usable for %s" % bse_name, file=sys.stderr)
        return 4

    os.makedirs(args.staging, exist_ok=True)
    out = os.path.join(args.staging, filename)
    with open(out, "w") as handle:
        handle.write(text)

    before = parse_bas(os.path.join(LIB, filename))
    after = parse_text(text)
    gained = sorted(set(after) - set(before))
    lost = sorted(set(before) - set(after))

    print("%s  (%s)" % (args.basis, bse_name))
    print("  file      : %s" % filename)
    print("  elements  : %d -> %d" % (len(before), len(after)))
    if gained:
        print("  gained    : %s" % " ".join(gained))
    if lost:
        print("  LOST      : %s   <- would remove coverage" % " ".join(lost))
    print("  staged    : %s" % out)
    return 0


if __name__ == "__main__":
    sys.exit(main())
