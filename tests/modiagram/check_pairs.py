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
    #  The model's own pairs (extended Hueckel overlap population): the book
    #  pairs 2sg/2su* and 3sg/3su* too, but in the model 2su and 3sg are
    #  sp-mixed and non-bonding, so the sigma pair is 2sg/4su*.
    "ecceqm-c2":  [("2SGG", "4SGU"), ("1PIU", "1PIG")],
    "ecceqm-n2":  [("2SGG", "4SGU"), ("1PIU", "1PIG")],
    "ecceqm-he2": [],
}


def molecular_levels(path):
    levels, inside = [], False
    for line in open(path):
        if line.startswith("column "):
            inside = line.strip() == "column centre"
        elif inside and line.lstrip().startswith("level "):
            f = dict(re.findall(r'(\w+)=("[^"]*"|\S+)', line))
            levels.append((f["label"].rstrip("*").upper().replace(" NB", " nb"), f["character"],
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
            labels.append(re.search(r'label=(\S+)', line).group(1).rstrip("*").upper())
        elif line.lstrip().startswith("link "):
            f = dict(re.findall(r'(\w+)=(-?\d+)', line))
            c = int(f["centre"])
            for side in ("left", "right"):
                if int(f[side]) >= 0:
                    out.setdefault(labels[c], set()).add((side, int(f[side])))
    return out


def check_sp_mixing(d):
    """3sg of C2 and N2 is linked to the 2p (1) of both atoms (the model's 2s share is under 10 %)."""
    bad = 0
    for name in ("ecceqm-c2", "ecceqm-n2"):
        links = mixing_links(os.path.join(d, name + ".dump")).get("3SGG", set())
        want = {("left", 1), ("right", 1)}
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
            if fn.endswith("-lfview.dump"):
                break        # the Ligand-field view hides the antibonding partners on purpose
            if p < 0 or "BONDING" not in chs or "ANTIBONDING" not in chs:
                bad += 1
                print("%s: pairing %d has %s" % (fn, p, chs))
    path = os.path.join(d, "orca-bf3.dump")
    if os.path.exists(path):
        want = {"3A1'": "BONDING", "5A1'": "ANTIBONDING", "2E'": "BONDING",
                "6E'": "ANTIBONDING", "1A2''": "BONDING", "2A2''": "ANTIBONDING"}
        #  4A1', 3E' (and 4E', 1E'', 1A2') are nb and sit in the lone-pair block
        have = dict((l[0].replace(" nb", ""), l[1])
                    for l in molecular_levels(path))
        for k, v in want.items():
            if have.get(k) != v:
                bad += 1
                print("orca-bf3: %s is %s, want %s" % (k, have.get(k), v))
    if os.path.exists(path):
        text = open(path).read()
        if "label=F3 internal: 6 levels, 12 e" not in text:
            bad += 1
            print("orca-bf3: no 'F3 internal' block")
        if "levels=6 " not in text:
            bad += 1
            print("orca-bf3: the block should stand for 6 MOs")
    print("pair rule over every golden: %s" % ("FAIL" if bad else "ok"))
    return bad


def check_lfview(d):
    """Ligand-field view of Cr(CO)6 (#301): t2g, then eg* straight above the
    HOMO (a Cr d level), and nothing that is 80 % or more CO drawn above the d
    levels; the full view tags every level and still shows the CO pi* levels."""
    bad = 0
    def centre(path):
        rows, inside = [], False
        for line in open(path):
            if line.startswith("column "):
                inside = line.strip() == "column centre"
            elif inside and line.lstrip().startswith("level "):
                f = dict(re.findall(r'(\w+)=("[^"]*"|\S+)', line))
                tag = re.search(r' tag=(.*?) character=', line)   # the tag holds spaces
                if tag:
                    f["tag"] = tag.group(1)
                rows.append(f)
        return rows
    lf = centre(os.path.join(d, "orca-crco6-lfview.dump"))
    full = centre(os.path.join(d, "orca-crco6.dump"))
    homo = max((i for i, f in enumerate(lf) if float(f["occupancy"]) > 0 and float(f["shareLeft"]) >= 0.5),
               key=lambda i: float(lf[i]["energy"]))
    above = sorted((i for i in range(len(lf)) if float(lf[i]["energy"]) > float(lf[homo]["energy"])),
                   key=lambda i: float(lf[i]["energy"]))
    ok = bool(above) and lf[above[0]]["label"].lower().startswith("5eg*") and \
        lf[above[0]].get("tag", "").startswith("Cr d")
    print("orca-crco6-lfview    level above the HOMO: %s tag %s %s" % (
        lf[above[0]]["label"] if above else None, lf[above[0]].get("tag") if above else None,
        "ok" if ok else "FAIL (want eg*, Cr d)"))
    bad += not ok
    hidden = [f for f in lf if float(f["occupancy"]) == 0 and float(f["shareRight"]) >= 0.5]
    print("orca-crco6-lfview    empty levels >= 50 %% on CO drawn: %d %s" % (len(hidden), "ok" if not hidden else "FAIL"))
    bad += bool(hidden)
    untagged = [f["label"] for f in lf + full if "tag" not in f]
    print("orca-crco6           every model level tagged: %s" % ("ok" if not untagged else "FAIL %s" % untagged))
    bad += bool(untagged)
    pistar = [f for f in full if "CO \u03c0*" in f.get("tag", "")]
    print("orca-crco6           full view has CO pi* levels: %d %s" % (len(pistar), "ok" if pistar else "FAIL"))
    bad += not pistar
    return bad


def check_d_electrons(d):
    """Metal d electrons are the ionic count (group number - charge) in the one
    d set of the left column: [PtCl4]2- d8, [CoCl4]2- d7, Ni(CO)4 d10."""
    want = {"orca-ptcl4": 8, "orca-cocl4-quartet": 7, "orca-nico4": 10}
    bad = 0
    for name, n in want.items():
        got = None
        for line in open(os.path.join(d, name + ".dump")):
            f = dict(re.findall(r'(\w+)=("[^"]*"|\S+)', line))
            if line.lstrip().startswith("level ") and f.get("label") in ("5d", "3d"):
                got = int(float(f["occupancy"]))
                break
        ok = got == n
        bad += not ok
        print("%-20s d electrons %s %s" % (name, got, "ok" if ok else "FAIL (want %d)" % n))
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
    bad += check_d_electrons(d)
    bad += check_lfview(d)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
