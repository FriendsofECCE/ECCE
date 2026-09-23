#!/usr/bin/env python3
"""
Compare what BSE would give us against what the library holds today.

Step 5 of the re-sourcing plan (issue #116), run early and READ-ONLY:
nothing is replaced, nothing in the live library is touched.  The output is
the report step 6 reviews.

For every orbital basis set that step 1 resolved to a BSE name and that is
a single file (not one of ECCE's composed aggregates), this fetches the BSE
data for exactly the elements ECCE covers, converts it, and compares value
by value against the shipped file.

Each set lands in one of:

    identical    every value agrees to the precision the library stores
    precision    differs only below ECCE's stored precision
    coverage     same values, but the element sets differ
    DIFFERS      at least one value genuinely moved -- needs a human
    structural   shell layout differs -- needs a human
    error        could not be fetched or converted

A "DIFFERS" verdict is not evidence of a bug in either source. The Helium
case in 6-31G was a contraction normalisation convention that changed no
energy at all (8e-9 Hartree in ORCA). It means a human looks.
"""

import argparse
import json
import os
import re
import sys
import time
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from bse2bas import convert                                    # noqa: E402

API = "https://www.basissetexchange.org/api/basis/%s/format/json/"
SYMBOLS = ("H He Li Be B C N O F Ne Na Mg Al Si P S Cl Ar K Ca Sc Ti V Cr Mn "
           "Fe Co Ni Cu Zn Ga Ge As Se Br Kr Rb Sr Y Zr Nb Mo Tc Ru Rh Pd Ag "
           "Cd In Sn Sb Te I Xe Cs Ba La Ce Pr Nd Pm Sm Eu Gd Tb Dy Ho Er Tm "
           "Yb Lu Hf Ta W Re Os Ir Pt Au Hg Tl Pb Bi Po At Rn Fr Ra Ac Th Pa "
           "U Np Pu Am Cm Bk Cf Es Fm Md No Lr").split()
Z = {s: i + 1 for i, s in enumerate(SYMBOLS)}
SYM = {i + 1: s for i, s in enumerate(SYMBOLS)}


def parse_bas(path):
    """{atom: [(shell, nprim, ncoef, [[floats]...])]} from a .BAS file."""
    out, cur = {}, None
    for line in open(path, errors="replace"):
        m = re.match(r"^atom=(\S+)", line)
        if m:
            cur = m.group(1)
            out[cur] = []
            continue
        m = re.match(r"^contraction shell=(\S+) num_primitives=(\d+) "
                     r"num_coefficients=(\d+)", line)
        if m and cur:
            out[cur].append([m.group(1), int(m.group(2)), int(m.group(3)), []])
            continue
        if cur and line.strip() and out[cur]:
            try:
                out[cur][-1][3].append([float(x) for x in line.split()])
            except ValueError:
                pass
    return out


def compare(regen, shipped, tolerance):
    """Classify one basis set. Returns (verdict, detail)."""
    only_shipped = [a for a in shipped if a not in regen]
    only_regen = [a for a in regen if a not in shipped]
    shared = [a for a in shipped if a in regen]
    if not shared:
        return "structural", "no elements in common"

    worst, nvalues, structural, reordered = 0.0, 0, [], []
    #  Ratios of differing values, grouped by contraction. A CONSTANT
    #  ratio across a whole contraction is a normalisation convention,
    #  not a data difference -- the library stores Helium's 6-31G
    #  coefficients a factor 1.688956 below BSE's, and running both in
    #  ORCA gave energies 8e-9 Hartree apart because the code renormalises.
    #  Ratios that VARY within one contraction are a real difference.
    ratios = {}
    for atom in shared:
        a, b = regen[atom], shipped[atom]
        #  Shell ORDER within an atom is not chemistry -- a basis with its
        #  D shell before its SP shell is the same basis. Sort both by
        #  (shell, nprim, ncoef, first exponent) so a mere reordering is
        #  reported as such rather than as a structural difference.
        def key(sh):
            first = sh[3][0][0] if sh[3] and sh[3][0] else 0.0
            return (sh[0], sh[1], sh[2], round(first, 8))
        if len(a) == len(b) and [key(x) for x in a] != [key(x) for x in b] \
                and sorted(key(x) for x in a) == sorted(key(x) for x in b):
            reordered.append(atom)
            a, b = sorted(a, key=key), sorted(b, key=key)
        if len(a) != len(b):
            structural.append("%s: %d shells vs %d" % (atom, len(a), len(b)))
            continue
        for sa, sb in zip(a, b):
            if sa[0] != sb[0] or sa[1] != sb[1]:
                structural.append("%s: shell %s(%d) vs %s(%d)"
                                  % (atom, sa[0], sa[1], sb[0], sb[1]))
                continue
            for ra, rb in zip(sa[3], sb[3]):
                for column, (va, vb) in enumerate(zip(ra, rb)):
                    nvalues += 1
                    relative = abs(va - vb) / max(abs(vb), 1e-30)
                    worst = max(worst, relative)
                    if relative > tolerance and abs(vb) > 1e-30:
                        ratios.setdefault((atom, sa[0], column),
                                          []).append(va / vb)

    if structural:
        #  Count the ELEMENTS affected, not the individual complaints. A
        #  whole basis set being called "structural" because one element
        #  out of thirty differs is not a useful thing to hand a reviewer:
        #  STO-3G differs from BSE's only on Ga, and 3-21G only on Sc.
        affected = sorted({c.split(":")[0] for c in structural})
        return "structural", ("%d/%d elements differ (%s)%s"
                              % (len(affected), len(shared),
                                 ", ".join(affected[:6]),
                                 "" if len(affected) <= 6 else ", ..."))
    detail = "%d values, worst %.1e" % (nvalues, worst)
    if reordered:
        detail += ", %d atom(s) reordered" % len(reordered)
    if only_shipped or only_regen:
        detail += ", elements +%d/-%d" % (len(only_regen), len(only_shipped))
    if worst > tolerance:
        spans = [max(v) - min(v) for v in ratios.values()]
        if spans and all(span < 1e-6 for span in spans):
            example = next(iter(ratios.values()))[0]
            #  Deliberately a DESCRIPTION, not a verdict of harmless. A
            #  constant ratio is consistent with a normalisation
            #  convention -- Helium's 1.688956 is one, and running both
            #  forms in ORCA differed by 8e-9 Hartree. But this sweep also
            #  turned up a constant ratio of exactly -1.000000 (a sign
            #  flip) and one of exactly 0.100000 (a factor of ten), and
            #  neither of those is a normalisation constant. Calling them
            #  all "normalisation" would wave through the two that most
            #  need looking at.
            return ("constant-ratio",
                    detail + ", constant ratio %.6f" % example)
        return "DIFFERS", detail
    if only_shipped or only_regen:
        return "coverage", detail
    if worst > 0:
        return "precision", detail
    return "identical", detail


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--baseline", default=os.path.join(HERE, "baseline.json"))
    ap.add_argument("--namemap", default=os.path.join(HERE, "namemap.json"))
    ap.add_argument("--library",
                    default=os.path.expanduser(
                        "~/.ECCE/dataserver/htdocs/Ecce/system/"
                        "GaussianBasisSetLibrary"))
    ap.add_argument("-o", "--output", default="diffreport.json")
    ap.add_argument("--limit", type=int, default=0)
    ap.add_argument("--tolerance", type=float, default=1e-4,
                    help="relative difference above which a value counts as "
                         "genuinely moved (default 1e-4, comfortably below "
                         "the library's stored precision)")
    ap.add_argument("--delay", type=float, default=0.5,
                    help="seconds between API calls; be polite")
    args = ap.parse_args()

    baseline = json.load(open(args.baseline))
    namemap = json.load(open(args.namemap))
    resolved = {}
    for bucket in ("direct", "alias", "curated"):
        for name, bse in namemap[bucket].items():
            resolved[name] = (bse, bucket)

    #  Single-file sets only: composed aggregates need their parts handled,
    #  which is a later step.
    work = []
    for type_name, entries in baseline.items():
        for name, entry in entries.items():
            if name in resolved and len(entry["files"]) == 1:
                work.append((name, entry, resolved[name]))
    work.sort()
    if args.limit:
        work = work[:args.limit]

    results, counts = {}, {}
    for i, (name, entry, (bse_name, bucket)) in enumerate(work, 1):
        filename = entry["files"][0]
        atoms = entry["atoms"][0] if entry["atoms"] else []
        zs = sorted({Z[a] for a in atoms if a in Z})
        if not zs:
            verdict, detail = "error", "no recognisable elements"
        else:
            #  Fetch every element BSE has rather than filtering to
            #  ECCE's list: BSE answers 404 for an element it does not
            #  cover, and ECCE's coverage is not always a subset. The
            #  comparison intersects afterwards anyway.
            url = API % urllib.parse.quote(bse_name)
            try:
                with urllib.request.urlopen(url, timeout=90) as response:
                    data = json.load(response)
                regen = parse_text(convert(data, SYM))
                shipped = parse_bas(os.path.join(args.library, filename))
                verdict, detail = compare(regen, shipped, args.tolerance)
            except Exception as exc:                        # noqa: BLE001
                verdict, detail = "error", str(exc)[:120]
            time.sleep(args.delay)

        results[name] = {"bse": bse_name, "bucket": bucket,
                         "verdict": verdict, "detail": detail}
        counts[verdict] = counts.get(verdict, 0) + 1
        print("[%3d/%3d] %-11s %-34s %s"
              % (i, len(work), verdict, name, detail), flush=True)

    with open(args.output, "w") as handle:
        json.dump(results, handle, indent=1, sort_keys=True)
    print("\n--- summary ---")
    for verdict in ("identical", "precision", "coverage",
                    "constant-ratio", "DIFFERS", "structural", "error"):
        if verdict in counts:
            print("  %-11s %d" % (verdict, counts[verdict]))
    print("\nwritten: %s" % args.output)
    return 0


def parse_text(text):
    """parse_bas, but over an in-memory .BAS string."""
    import tempfile
    handle = tempfile.NamedTemporaryFile("w", suffix=".BAS", delete=False)
    handle.write(text)
    handle.close()
    try:
        return parse_bas(handle.name)
    finally:
        os.unlink(handle.name)


if __name__ == "__main__":
    import urllib.parse
    sys.exit(main())
