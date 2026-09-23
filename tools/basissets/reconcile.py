#!/usr/bin/env python3
"""
Match ECCE's basis set names against the Basis Set Exchange.

Step 1 of the re-sourcing plan (issue #116).  Produces the mapping that
step 4 would regenerate from, and -- just as important -- the list of
names that have no BSE counterpart and must therefore keep their current
data whatever else happens.

Classification is driven by the per-type alias files, not by guessing from
the name.  ECCE's library holds two different kinds of thing:

  ORBITAL types    pople, other_segmented, correlation_consistent,
                   other_generally_contracted, ECPOrbital, DFTOrbital
                   -- these are what the Basis Set Tool offers, and what
                   needs reconciling.

  COMPONENT types  polarization, diffuse, rydberg, ecp, Charge, Exchange
                   -- pieces ECCE composes INTO the orbital sets.  BSE has
                   no equivalent because it ships complete sets, so these
                   are not missing; they are a consequence of ECCE's
                   structure, which we are keeping.

    ./reconcile.py --baseline baseline.json --bse bse_metadata.json
"""

import argparse
import json
import re
import sys

ORBITAL = ("pople", "other_segmented", "correlation_consistent",
           "other_generally_contracted", "ECPOrbital", "DFTOrbital")


def bse_index(meta):
    """Every name BSE answers to -> its canonical key."""
    index = {}
    for key, value in meta.items():
        for name in [key, value.get("display_name", "")] + \
                    list(value.get("other_names", [])):
            if name:
                index.setdefault(name.lower(), key)
    return index


#  Curated spellings, each CONFIRMED to exist in the BSE metadata rather
#  than guessed at.  These are proposals about identity, not just naming:
#  "ADZP is jorge-adzp" is an assertion that the two hold the same data,
#  which only the step-5 numerical diff can settle.  They are reported
#  separately from automatic matches for exactly that reason.
CURATED = {
    "adzp": "jorge-adzp",
    "aqzp": "jorge-aqzp",
    "atzp": "jorge-atzp",
    "j-dzp": "jorge-dzp",
    "j-tzp": "jorge-tzp",
    "j-qzp": "jorge-qzp",
    "j-dzp-dkh": "jorge-dzp-dkh",
    "j-tzp-dkh": "jorge-tzp-dkh",
    "j-qzp-dkh": "jorge-qzp-dkh",
    "dzvp (dft orbital)": "dgauss-dzvp",
    "dzvp2 (dft orbital)": "dgauss-dzvp2",
    "tzvp (dft orbital)": "dgauss-tzvp",
    "def2-svp(p)": "def2-sv(p)",
    "lanl2tzf": "lanl2tz(f)",
    "sbkjc vdz ecp": "sbkjc-vdz",
    "stuttgart rsc ano/ecp": "stuttgart rsc ano",
    "stuttgart rsc segmented/ecp": "stuttgart rsc segmented + ecp",
    "partridge uncontr. 1": "partridge uncontracted 1",
    "partridge uncontr. 2": "partridge uncontracted 2",
    "partridge uncontr. 3": "partridge uncontracted 3",
    "partridge uncontr. 4": "partridge uncontracted 4",
}


def candidates(name):
    """ECCE spellings to try against BSE, most literal first."""
    low = name.lower().strip()
    yield low
    #  ECCE marks ECP-bearing sets with a trailing " ECP"; BSE does not.
    if low.endswith(" ecp"):
        yield low[:-4]
    #  "Binning/Curtiss SV" style -- BSE uses hyphens and no spaces.
    yield low.replace("/", "-").replace(" ", "-")
    yield low.replace("/", "").replace(" ", "")
    yield low.replace(" ", "")
    #  "(old)" and similar trailing qualifiers.
    stripped = re.sub(r"\s*\((old|fi/sf/\w+|pt/sf/\w+)\)\s*$", "", low)
    if stripped != low:
        yield stripped


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--baseline", default="baseline.json")
    ap.add_argument("--bse", required=True,
                    help="BSE /api/metadata/ response")
    ap.add_argument("-o", "--output", default="namemap.json")
    args = ap.parse_args()

    baseline = json.load(open(args.baseline))
    index = bse_index(json.load(open(args.bse)))

    result = {"direct": {}, "alias": {}, "curated": {},
              "composed": [], "absent": [], "component": []}
    for type_name, entries in baseline.items():
        if type_name not in ORBITAL:
            result["component"] += sorted(entries)
            continue
        for name in sorted(entries):
            low = name.lower().strip()
            if low in CURATED and CURATED[low] in index:
                result["curated"][name] = index[CURATED[low]]
                continue
            tried = list(candidates(name))
            hit = next((t for t in tried if t in index), None)
            if hit is None:
                #  A COMPOSED set (more than one file) having no BSE name
                #  is expected rather than a gap: ECCE builds 3-21G* from
                #  3-21G plus a polarization component, while BSE ships
                #  only complete sets and has no "3-21G*" at all. Keeping
                #  ECCE's structure means re-sourcing the PARTS, so what
                #  matters for these is whether each part resolves, not
                #  the composite name.
                if len(entries[name]["files"]) > 1:
                    result["composed"].append(name)
                else:
                    result["absent"].append(name)
            elif hit == tried[0]:
                result["direct"][name] = index[hit]
            else:
                result["alias"][name] = index[hit]

    with open(args.output, "w") as handle:
        json.dump(result, handle, indent=1, sort_keys=True)

    orbital = (len(result["direct"]) + len(result["alias"])
               + len(result["curated"]) + len(result["composed"])
               + len(result["absent"]))
    print("orbital basis sets to reconcile : %d" % orbital)
    print("  direct name match             : %d" % len(result["direct"]))
    print("  matched via an alias spelling : %d" % len(result["alias"]))
    print("  curated, NEEDS data confirm   : %d" % len(result["curated"]))
    print("  composed, no composite in BSE : %d  (parts re-sourced instead)"
          % len(result["composed"]))
    print("  NO BSE counterpart at all     : %d" % len(result["absent"]))
    print("composition components (kept as-is, ECCE structure retained): %d"
          % len(result["component"]))
    print("\nwritten: %s" % args.output)
    return 0


if __name__ == "__main__":
    sys.exit(main())
