#!/usr/bin/env python3
"""
Write each orbital-only aggregate basis set as ONE whole file.

An aggregate such as 6-31G* is filed in the library as a list of component
files (6-31G.BAS + 6-31GS.BAS) plus a placeholder "6-31GS-AGG.BAS".  The
placeholder used to be empty, and the loader assembled the set from its
components.  This script fills the placeholder with the merged content, so
the loader can read the set the way it reads cc-pVDZ: one file, one identity.

    merge_aggregates.py [--dir DIR] [--check]      write (or, with --check,
                                                   verify) the -AGG.BAS files
    merge_aggregates.py --list                     what is and is not merged
                                                   (writes nothing)

The merged file must be what the component load produces, shell for shell:

  * per element, the orbital components' shells come first, then the
    auxiliary ones (polarization, diffuse, rydberg), each group in the
    order the index lists its files.  That is the order TGBSGroup keeps its
    basis sets in and TGBSConfig::dump() prints them, and MO-coefficient
    pairing depends on it;
  * each component contributes only the elements its atoms= line lists, and
    an element repeated within a file keeps its last block -- the two rules
    EDSIGaussianBasisSetLibrary::parseGbsData() applies.

Aggregates with an ECP (.POT) or DFT fitting component are left alone: their
placeholder stays empty and the loader keeps assembling them.  The component
files are never touched; they remain the add-on lists of the Basis Set Tool
and what an older client, which skips "-AGG." files, still reads.

Component identity is worked out as EDSIGaussianBasisSetLibrary::lookup()
does: the single-file index entry that names the file (compared exactly,
case included), else the aggregate's own name and type.
"""

import argparse
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULT_DIR = os.path.join(os.path.dirname(os.path.dirname(HERE)),
                           "data", "admin", "basissets")

#  TGaussianBasisSet::GBSType order; lookup() walks it, first match wins.
TYPES = ["pople", "other_segmented", "correlation_consistent",
         "other_generally_contracted", "ECPOrbital", "DFTOrbital",
         "polarization", "diffuse", "rydberg", "ecp", "Exchange", "Charge"]
ORBITAL = TYPES[:5]
AUXILIARY = ["polarization", "diffuse", "rydberg"]


def read_index(directory, gbs_type):
    path = os.path.join(directory, gbs_type)
    aliases = []
    if not os.path.exists(path):
        return aliases
    with open(path, errors="replace") as handle:
        for line in handle:
            line = line.rstrip("\n")
            if line.startswith("name= "):
                aliases.append(dict(name=line[6:], type=gbs_type,
                                    files=[], atoms=[]))
            elif line.startswith("files= ") and aliases:
                aliases[-1]["files"] += line[7:].split()
            elif line.startswith("atoms= ") and aliases:
                aliases[-1]["atoms"].append(line[7:].split())
    return aliases


def read_all_indexes(directory):
    return {t: read_index(directory, t) for t in TYPES}


def read_bas(path, allowed):
    """{element: [raw lines of its shells]}, as parseGbsData() reads it."""
    blocks, element = {}, None
    with open(path, errors="replace") as handle:
        for line in handle.read().split("\n"):
            if line.startswith("atom="):
                element = line[5:]
                blocks[element] = []
            elif element is not None and line.strip():
                blocks[element].append(line)
    return {e: b for e, b in blocks.items() if e in allowed}


def component_atoms(alias, index, has_dummy):
    """The atoms= list the loader uses for component number `index`."""
    if index >= len(alias["atoms"]):
        return alias["atoms"][0]
    return alias["atoms"][index]


def own_type(indexes, filename):
    for t in TYPES:
        for cand in indexes[t]:
            if len(cand["files"]) == 1 and cand["files"][0] == filename:
                return t
    return None


def plan(directory):
    """[(alias, verdict, detail)] for every alias that has an -AGG file."""
    indexes = read_all_indexes(directory)
    result = []
    for t in TYPES:
        for alias in indexes[t]:
            files = alias["files"]
            if not files or "-AGG." not in files[0]:
                continue
            comps = files[1:]
            if len(comps) < 2:
                result.append((alias, "single", "one component"))
                continue
            if any(c.upper().endswith(".POT") for c in comps):
                result.append((alias, "ecp", "has an ECP component"))
                continue
            #  A component the index does not file on its own (the
            #  aggregate spells the file differently, e.g. CC-PVDZ.BAS
            #  against cc-pVDZ.BAS) is loaded under the aggregate's own
            #  name and type.
            kinds = [own_type(indexes, c) or alias["type"] for c in comps]
            if any(k not in ORBITAL + AUXILIARY for k in kinds) or \
                    alias["type"] == "DFTOrbital":
                result.append((alias, "fitting", "types %s" % kinds))
                continue
            result.append((alias, "merge", kinds))
    return result


def merged_text(directory, alias, kinds):
    comps = alias["files"][1:]
    per = []
    for i, name in enumerate(comps):
        atoms = set(component_atoms(alias, i, True))
        per.append((kinds[i], read_bas(os.path.join(directory, name), atoms)))
    ordered = [p for p in per if p[0] in ORBITAL] + \
              [p for p in per if p[0] not in ORBITAL]
    elements = []
    for _, blocks in ordered:
        for e in blocks:
            if e not in elements:
                elements.append(e)
    out = []
    for e in elements:
        out.append("atom=%s" % e)
        for _, blocks in ordered:
            out += blocks.get(e, [])
    return "\n".join(out) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--dir", default=DEFAULT_DIR)
    parser.add_argument("--check", action="store_true",
                        help="write nothing; fail if a file is out of date")
    parser.add_argument("--list", action="store_true")
    args = parser.parse_args()

    stale, counts = [], {}
    for alias, verdict, detail in plan(args.dir):
        counts[verdict] = counts.get(verdict, 0) + 1
        agg = os.path.join(args.dir, alias["files"][0])
        if args.list:
            print("%-8s %-40s %s" % (verdict, alias["name"], detail))
        if verdict != "merge":
            continue
        text = merged_text(args.dir, alias, detail)
        current = open(agg).read() if os.path.exists(agg) else ""
        if current != text:
            stale.append(alias["files"][0])
            if not (args.check or args.list):
                with open(agg, "w") as handle:
                    handle.write(text)
    print("aggregates: " + ", ".join("%s %d" % kv for kv in sorted(counts.items())))
    if args.check and stale:
        print("out of date (run merge_aggregates.py): " + " ".join(stale))
        return 1
    if stale and not (args.check or args.list):
        print("wrote %d file(s)" % len(stale))
    return 0


if __name__ == "__main__":
    sys.exit(main())
