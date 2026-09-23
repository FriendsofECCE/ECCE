#!/usr/bin/env python3
"""
Fingerprint the ECCE basis set library.

Step 0 of the re-sourcing plan (issue #116): a complete, exact record of
what the library contains TODAY, so that anything regenerated from the
Basis Set Exchange can be diffed against it rather than eyeballed.

Deliberately NOT a set of code runs.  A full energy baseline over every
named basis in every code would be thousands of jobs and almost all of
them wasted -- only the basis sets whose NUMBERS actually move need an
energy comparison, and that belongs in step 5, applied to the handful
that change.  What is needed now is the content, exactly.

Reads the per-type alias files the GUI itself reads (pople,
correlation_consistent, ...), so a basis appears here under the name the
Basis Set Tool offers, composed of the same files, rather than as a
directory listing.

    ./snapshot.py <library-dir> [-o baseline.json]

Default library: the running data server's copy.
"""

import argparse
import hashlib
import json
import os
import re
import sys

TYPE_FILES = ("pople", "other_segmented", "correlation_consistent",
              "other_generally_contracted", "ECPOrbital", "DFTOrbital",
              "Charge", "Exchange", "diffuse", "polarization", "rydberg",
              "ecp")


def parse_alias(path):
    """Yield {name, files, atoms} records from one alias file."""
    record = None
    with open(path, errors="replace") as handle:
        for line in handle:
            line = line.rstrip("\n")
            if line.startswith("name="):
                if record:
                    yield record
                record = {"name": line.split("=", 1)[1].strip(),
                          "files": [], "atoms": []}
            elif record is not None and line.startswith("files="):
                record["files"] = line.split("=", 1)[1].split()
            elif record is not None and line.startswith("atoms="):
                #  One atoms= line per file in an aggregate set.
                record["atoms"].append(line.split("=", 1)[1].split())
    if record:
        yield record


def fingerprint(path, full=False):
    """Structure and an exact checksum for one .BAS or .POT file.

    The checksum is over the NUMBERS, normalised, not the raw text -- the
    shipped library and anything regenerated will format differently
    (3047.5249 against 0.3047524880E+04) and a textual hash would report
    every basis as changed while saying nothing about the data.
    """
    if not os.path.exists(path):
        return None
    atoms, current, digest = {}, None, hashlib.sha256()
    for line in open(path, errors="replace"):
        m = re.match(r"^atom=(\S+)", line)
        if m:
            current = m.group(1)
            atoms.setdefault(current, [])
            digest.update(("atom:" + current + "\n").encode())
            continue
        m = re.match(r"^contraction shell=(\S+) num_primitives=(\d+) "
                     r"num_coefficients=(\d+)", line)
        if m and current:
            atoms[current].append([m.group(1), int(m.group(2)),
                                   int(m.group(3))])
            digest.update(("shell:%s:%s:%s\n" % m.groups()).encode())
            continue
        if current and line.strip() and not line.startswith(("ecp_", "#")):
            try:
                values = [float(x) for x in line.split()]
            except ValueError:
                digest.update(("raw:" + line.strip() + "\n").encode())
                continue
            #  %.10e is well beyond what the library stores, so this
            #  distinguishes real differences without tripping on format.
            digest.update((" ".join("%.10e" % v for v in values)
                           + "\n").encode())
    summary = {"atoms": sorted(atoms),
               "nshells": sum(len(v) for v in atoms.values()),
               "sha256": digest.hexdigest()}
    if full:
        #  Per-atom shell detail, for investigating a specific difference.
        #  Left out by default: it is 50x the size and the checksum already
        #  says whether anything moved.
        summary["shells"] = atoms
    return summary


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("library", nargs="?",
                    default=os.path.expanduser(
                        "~/.ECCE/dataserver/htdocs/Ecce/system/"
                        "GaussianBasisSetLibrary"))
    ap.add_argument("-o", "--output", default="baseline.json")
    ap.add_argument("--full", action="store_true",
                    help="include per-atom shell detail (50x larger; only "
                         "needed when investigating a specific difference)")
    args = ap.parse_args()

    if not os.path.isdir(args.library):
        print("no such library: %s" % args.library, file=sys.stderr)
        return 2

    out, missing = {}, []
    for type_name in TYPE_FILES:
        path = os.path.join(args.library, type_name)
        if not os.path.exists(path):
            continue
        for record in parse_alias(path):
            entry = {"type": type_name, "files": record["files"],
                     "atoms": record["atoms"], "content": {}}
            for filename in record["files"]:
                fp = fingerprint(os.path.join(args.library, filename),
                                 full=args.full)
                if fp is None:
                    missing.append("%s -> %s" % (record["name"], filename))
                else:
                    entry["content"][filename] = fp
            out.setdefault(type_name, {})[record["name"]] = entry

    with open(args.output, "w") as handle:
        json.dump(out, handle, indent=1, sort_keys=True)

    total = sum(len(v) for v in out.values())
    print("library : %s" % args.library)
    print("types   : %d" % len(out))
    print("names   : %d" % total)
    for type_name in sorted(out):
        print("   %-30s %3d" % (type_name, len(out[type_name])))
    if missing:
        print("\nreferenced but MISSING from the library (%d):" % len(missing))
        for m in missing[:20]:
            print("   %s" % m)
    print("\nwritten: %s" % args.output)
    return 0


if __name__ == "__main__":
    sys.exit(main())
