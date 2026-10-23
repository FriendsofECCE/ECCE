#!/usr/bin/env python3
"""Remove a duplicate 'Imported' orbital set from BasisSet.ecce_basisset.

A calculation made by script can list both the real basis (def2-tzvp) and an
'Imported' copy of the same shells for every element; ECCE then counted every
function twice and could not draw the MOs (#497).  This drops the 'Imported'
set from a group only when another set of that group has identical shells for
every element the 'Imported' set holds, so nothing a calculation needs is lost.

    dedupe_imported_basis.py [--dry-run] [--no-backup] PATH...

PATH is a BasisSet.ecce_basisset file, or a directory searched recursively for
them (a project folder).  The original is kept as BasisSet.ecce_basisset.bak.
"""
import argparse
import os
import re
import shutil
import sys

IMPORTED = "Imported"


def data_blocks(text):
    """name -> {element: [shell lines]} from the <Data> section."""
    m = re.search(r"<Data>\n(.*?)</Data>", text, re.S)
    blocks = {}
    if not m:
        return blocks
    for b in re.finditer(r"BasisSet=(.*?)\n(.*?)EndBasisSet\n", m.group(1), re.S):
        elems, cur = {}, None
        for line in b.group(2).splitlines():
            if line.startswith("element="):
                cur = line[8:].strip()
                elems[cur] = []
            elif cur is not None:
                elems[cur].append(line.split())
        blocks[b.group(1).strip()] = elems
    return blocks


def same(a, b):
    if len(a) != len(b):
        return False
    for x, y in zip(a, b):
        if len(x) != len(y):
            return False
        for u, v in zip(x, y):
            try:
                if abs(float(u) - float(v)) > 1e-9 * max(1.0, abs(float(u))):
                    return False
            except ValueError:
                if u != v:
                    return False
    return True


def repair(text):
    """Return (new text, list of messages)."""
    blocks = data_blocks(text)
    msgs = []
    imported = blocks.get(IMPORTED)

    def fix_group(m):
        g = m.group(0)
        tag = re.search(r'<Group Tag="([^"]*)"', g).group(1)
        names = re.findall(r'<OrbitalBasisSet Name="([^"]*)"', g)
        others = [n for n in names if n != IMPORTED]
        if IMPORTED not in names or not others or imported is None:
            return g
        for n in others:
            o = blocks.get(n, {})
            if all(e in o and same(imported[e], o[e]) for e in imported):
                break
        else:
            msgs.append("group %s: 'Imported' differs from %s, kept"
                        % (tag, ", ".join(others)))
            return g
        g = re.sub(r'<BasisSet Type="[^"]*">Imported</BasisSet>\n', "", g)
        g = re.sub(r'<OrbitalBasisSet Name="Imported">.*?</OrbitalBasisSet>\n',
                   "", g, flags=re.S)
        g = re.sub(r'<BasisSet uniqueKey="Imported:[^"]*">.*?</BasisSet>\n',
                   "", g, flags=re.S)
        msgs.append("group %s: removed duplicate 'Imported' (same as %s)"
                    % (tag, n))
        return g

    text = re.sub(r"<Group Tag=.*?</Group>\n?", fix_group, text, flags=re.S)
    if not re.search(r"Imported", text.split("<Data>")[0]):
        text = re.sub(r"BasisSet=Imported\n.*?EndBasisSet\n", "", text,
                      flags=re.S)
    return text, msgs


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("paths", nargs="+")
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--no-backup", action="store_true")
    args = ap.parse_args()
    files = []
    for p in args.paths:
        if os.path.isdir(p):
            for root, _, names in os.walk(p):
                files += [os.path.join(root, n) for n in names
                          if n == "BasisSet.ecce_basisset"]
        else:
            files.append(p)
    changed = 0
    for f in sorted(files):
        with open(f) as fh:
            old = fh.read()
        new, msgs = repair(old)
        if new == old:
            continue
        changed += 1
        print("%s%s" % ("would fix " if args.dry_run else "fixed ", f))
        for m in msgs:
            print("    " + m)
        if not args.dry_run:
            if not args.no_backup:
                shutil.copy2(f, f + ".bak")
            with open(f, "w") as fh:
                fh.write(new)
    print("%d of %d file(s) %s" % (changed, len(files),
          "need repair" if args.dry_run else "repaired"))


if __name__ == "__main__":
    sys.exit(main())
