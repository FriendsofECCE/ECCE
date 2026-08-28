#!/usr/bin/env python3
"""
Generates the per-type Gaussian Basis Set alias-index files
(EDSIGaussianBasisSetLibrary::getGbsAliasList()'s file-based lookup) and
sets the per-file DAV custom properties (EDSIGaussianBasisSetLibrary::
lookup()'s per-file name/type/spherical/contraction_type metadata fetch)
needed for Builder/CalcEd's "Quick Basis Menu" to actually resolve basis
sets instead of reporting false "doesn't cover all elements" errors.

Scoped to the 9 (of 17) Quick Basis Picks that have real, unambiguous
underlying data in the vendored GaussianBasisSetLibrary:
  - The 3 "(DFT Orbital)" entries (DZVP, DZVP2, TZVP) have no matching
    .BAS files in this library at all -- a data-completeness gap this
    script can't fix.
  - 6-31++G, 6-31+G*, 6-31++G** only have 0-byte "-AGG" placeholder files
    in this library, with no real diffuse-function component file to
    combine them with -- guessing at a composition risks silently serving
    a scientifically wrong basis set, worse than a clear "not found".

Usage: populate-gbs-metadata.py <GaussianBasisSetLibrary-dir> <base-url>
  e.g. populate-gbs-metadata.py "$STATEDIR/htdocs/Ecce/system/GaussianBasisSetLibrary" \\
                                 "http://localhost:$PORT/Ecce/system/GaussianBasisSetLibrary"
"""
import os
import sys
import urllib.request
import urllib.parse
import urllib.error

if len(sys.argv) != 3:
    sys.exit("usage: populate-gbs-metadata.py <library-dir> <base-url>")

LIBRARY_DIR = sys.argv[1]
BASE_URL = sys.argv[2]
NS = "http://www.emsl.pnl.gov/ecce:"

# (quick-pick name, real filename(s), type)
#
# 6-31++G, 6-31+G*, 6-31++G** deliberately NOT included: their only "files"
# in this library are 0-byte -AGG placeholders with no real component file
# alongside them (confirmed: no plain non-AGG diffuse-function file exists
# anywhere in this library to combine with the base set). Guessing at a
# composition here risks silently serving a WRONG/incomplete basis set for
# a real quantum chemistry calculation -- clearly worse than these 3
# quick-picks continuing to correctly report "not found" until someone
# with the actual NWChem library composition rules can do this properly.
#
# 6-31G* IS included, combining the base 6-31G.BAS (full H-Zn coverage)
# with 6-31GS.BAS (heavy-atom-only polarization data -- correctly excludes
# H/He, standard 6-31G* chemistry, not a data gap) as two real component
# files, not a dummy aggregate.
QUICK_PICKS = [
    ("6-31G",        ["6-31G.BAS"],               "other_generally_contracted"),
    ("6-31G*",       ["6-31G.BAS", "6-31GS.BAS"], "other_generally_contracted"),
    ("cc-pVDZ",       ["cc-pVDZ.BAS"],               "correlation_consistent"),
    ("cc-pVTZ",       ["cc-pVTZ.BAS"],               "correlation_consistent"),
    ("aug-cc-pVDZ",   ["AUG-CC-PVDZ.BAS"],           "correlation_consistent"),
    ("aug-cc-pVTZ",   ["AUG-CC-PVTZ.BAS"],           "correlation_consistent"),
    ("def2-svp",      ["def2-svp.BAS", "def2-svp.POT"],   "ECPOrbital"),
    ("def2-svpd",     ["def2-svpd.BAS", "def2-svpd.POT"], "ECPOrbital"),
    ("def2-tzvp",     ["def2-tzvp.BAS"],             "ECPOrbital"),
]


def atoms_for_file(path):
    atoms = []
    if not os.path.exists(path):
        return atoms
    with open(path, "r", errors="replace") as f:
        for line in f:
            line = line.strip()
            if line.startswith("atom="):
                atoms.append(line[len("atom="):].strip())
    return atoms


def build_descriptor(name, files):
    lines = ["name= %s" % name, "files= %s" % " ".join(files)]
    for fn in files:
        if fn.endswith(".POT"):
            # .POT (ECP potential) files use a different internal format;
            # atom list for ECP components mirrors the paired .BAS file's.
            continue
        lines.append("atoms= %s" % " ".join(atoms_for_file(os.path.join(LIBRARY_DIR, fn))))
    return "\n".join(lines) + "\n"


def proppatch(filename, name, gbstype):
    url = BASE_URL + "/" + urllib.parse.quote(filename)
    body = f"""<?xml version="1.0" encoding="utf-8"?>
<D:propertyupdate xmlns:D="DAV:" xmlns:e="{NS}">
  <D:set>
    <D:prop>
      <e:name>{name}</e:name>
      <e:type>{gbstype}</e:type>
      <e:spherical>Y</e:spherical>
      <e:contraction_type>Segmented</e:contraction_type>
    </D:prop>
  </D:set>
</D:propertyupdate>"""
    req = urllib.request.Request(url, data=body.encode("utf-8"), method="PROPPATCH")
    req.add_header("Content-Type", "text/xml")
    try:
        with urllib.request.urlopen(req) as resp:
            return resp.status
    except urllib.error.HTTPError as e:
        return e.code


def main():
    by_type = {}
    for name, files, gbstype in QUICK_PICKS:
        missing = [f for f in files if not os.path.exists(os.path.join(LIBRARY_DIR, f))]
        if missing:
            print(f"SKIP {name}: missing files {missing}")
            continue

        by_type.setdefault(gbstype, []).append(build_descriptor(name, files))

        # PROPPATCH the .BAS file (and .POT if present) with per-file metadata
        for fn in files:
            gtype = "ecp" if fn.endswith(".POT") else gbstype
            status = proppatch(fn, name, gtype)
            print(f"PROPPATCH {fn} (name={name}, type={gtype}) -> {status}")

    for gbstype, entries in by_type.items():
        out_path = os.path.join(LIBRARY_DIR, gbstype)
        with open(out_path, "w") as f:
            f.write("\n".join(entries))
        print(f"WROTE index file: {out_path} ({len(entries)} entries)")


if __name__ == "__main__":
    main()
