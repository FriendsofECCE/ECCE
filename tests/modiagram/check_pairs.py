#!/usr/bin/env python3
"""Textbook checks over the ECCE-QM homonuclear diatomic golden dumps (#132).

For C2, N2 and He2 the molecular column must hold as many bonding as
antibonding levels (a degenerate set counts once), and the colour pairs
must be the g/u partners of one type: 2sg/2su*, 3sg/3su*, 1pu/1pg*.
Run: python3 tests/modiagram/check_pairs.py [dir-with-dumps]
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
EXPECT = {
    "ecceqm-c2":  [("2SGG", "2SGU"), ("3SGG", "3SGU"), ("1PIU", "1PIG")],
    "ecceqm-n2":  [("2SGG", "2SGU"), ("3SGG", "3SGU"), ("1PIU", "1PIG")],
    "ecceqm-he2": [("1SGG", "1SGU")],
}


def molecular_levels(path):
    levels, inside = [], False
    for line in open(path):
        if line.startswith("column "):
            inside = line.strip() == "column centre"
        elif inside and line.lstrip().startswith("level "):
            f = dict(re.findall(r'(\w+)=("[^"]*"|\S+)', line))
            levels.append((f["label"].rstrip("*"), f["character"],
                           int(f["pairing"])))
    return levels


def mixing_links(path):
    """For each molecular level label, the set of (side, fragment level) links."""
    labels, out = [], {}
    inside = False
    for line in open(path):
        if line.startswith("column "):
            inside = line.strip() == "column centre"
        elif inside and line.lstrip().startswith("level "):
            labels.append(re.search(r'label=(\S+)', line).group(1).rstrip("*"))
        elif line.lstrip().startswith("link "):
            f = dict(re.findall(r'(\w+)=(-?\d+)', line))
            c = int(f["centre"])
            for side in ("left", "right"):
                if int(f[side]) >= 0:
                    out.setdefault(labels[c], set()).add((side, int(f[side])))
    return out


def check_sp_mixing(d):
    """3sg of C2 and N2 is linked to both the 2s (0) and 2p (1) of both atoms (#287)."""
    bad = 0
    for name in ("ecceqm-c2", "ecceqm-n2"):
        links = mixing_links(os.path.join(d, name + ".dump")).get("3SGG", set())
        want = {("left", 0), ("left", 1), ("right", 0), ("right", 1)}
        ok = want <= links
        bad += not ok
        print("%-12s 3SGG s-p mixing links %s" % (name, "ok" if ok else
              "FAIL (have %s)" % sorted(links)))
    return bad


def check_rules(d):
    """The one pair rule (#301): in every dump each bonding level has an
    antibonding partner of its pairing number and the reverse, nothing is
    MIXED, and the labels follow the character (`*`, `nb`).  BF3: 3A1'/5A1'*,
    2E'/5E'* and the pi pair 1A2''/2A2''*, with 4A1' and 3E' non-bonding (merged into the F lone-pair block)."""
    bad = 0
    for fn in sorted(os.listdir(d)):
        if not fn.endswith(".dump"):
            continue
        lv = molecular_levels(os.path.join(d, fn))
        by = {}
        for label, ch, p in lv:
            if ch == "MIXED":
                bad += 1
                print("%s: %s is MIXED" % (fn, label))
            if ch in ("BONDING", "ANTIBONDING"):
                by.setdefault(p, []).append(ch)
        for p, chs in by.items():
            if p < 0 or "BONDING" not in chs or "ANTIBONDING" not in chs:
                bad += 1
                print("%s: pairing %d has %s" % (fn, p, chs))
    path = os.path.join(d, "orca-bf3.dump")
    if os.path.exists(path):
        want = {"3A1'": "BONDING", "5A1'": "ANTIBONDING", "2E'": "BONDING",
                "5E'": "ANTIBONDING", "1A2''": "BONDING", "2A2''": "ANTIBONDING"}
        #  4A1', 3E' (and 4E', 1E'', 1A2') are nb and sit in the lone-pair block
        have = dict((l[0].replace(" nb", ""), l[1])
                    for l in molecular_levels(path))
        for k, v in want.items():
            if have.get(k) != v:
                bad += 1
                print("orca-bf3: %s is %s, want %s" % (k, have.get(k), v))
    if os.path.exists(path):
        text = open(path).read()
        if "label=F lone pairs (nb)" not in text:
            bad += 1
            print("orca-bf3: no 'F lone pairs (nb)' block")
        if "levels=8 " not in text:
            bad += 1
            print("orca-bf3: the block should stand for 8 MOs")
    print("pair rule over every golden: %s" % ("FAIL" if bad else "ok"))
    return bad


def main():
    d = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, "expected")
    bad = 0
    for name, pairs in EXPECT.items():
        lv = molecular_levels(os.path.join(d, name + ".dump"))
        nb = sum(1 for l in lv if l[1] == "BONDING")
        na = sum(1 for l in lv if l[1] == "ANTIBONDING")
        got = {}
        for label, _, p in lv:
            if p >= 0:
                got.setdefault(p, []).append(label)
        got = sorted(tuple(v) for v in got.values())
        want = sorted(pairs)
        ok = nb == na and got == want
        bad += not ok
        print("%-12s bonding=%d antibonding=%d pairs=%s %s"
              % (name, nb, na, got, "ok" if ok else "FAIL (want %s)" % want))
    bad += check_sp_mixing(d)
    bad += check_rules(d)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
