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
    antibonding partner of its pairing number and the reverse (a level with no
    partner has pairing -1 and keeps the character of its own overlap
    population, #458), nothing is
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
            if ch in ("BONDING", "ANTIBONDING") and p >= 0:
                by.setdefault(p, []).append(ch)
        for p, chs in by.items():
            if fn.endswith("-lfview.dump"):
                break        # the Ligand-field view hides the antibonding partners on purpose
            if "BONDING" not in chs or "ANTIBONDING" not in chs:
                bad += 1
                print("%s: pairing %d has %s" % (fn, p, chs))
    path = os.path.join(d, "orca-bf3.dump")
    if os.path.exists(path):
        #  The sigma bonds are F 2p (F 2s is core-like: folded with the lone
        #  pairs): 4a1' and 3e' pair with 5a1'* and 6e'*, 1a2'' with 2a2''*.
        want = {"4A1'": "BONDING", "5A1'": "ANTIBONDING", "3E'": "BONDING",
                "6E'": "ANTIBONDING", "1A2''": "BONDING", "2A2''": "ANTIBONDING"}
        #  4E' is nb (a lone pair); the F 2s levels, 1E'' and 1A2' sit in the block
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


def check_electrons(d):
    """Every electron is somewhere in a model-first diagram: per fragment,
    drawn + folded as core-like ns + folded as internal = the fragment's total
    (and the About's), the drawn sets add up in the dump's columns, and the
    model column's drawn + folded = the molecule's total."""
    import re
    bad = 0
    seen = 0
    for fn in sorted(os.listdir(d)):
        if not fn.endswith(".dump"):
            continue
        text = open(os.path.join(d, fn)).read()
        m = re.search(r"^note: .*?electrons: (.*?)(?: \xc2\xb7 labels|\\n|$)", text, re.M)
        if not m:
            continue
        seen += 1
        part = m.group(1)
        frag = part.split("; model and calculation: ")[0].split("; ")
        mm = re.search(r"model and calculation: (\d+) drawn \+ (\d+) folded .*?= (\d+)", part)
        sums = []
        cols = {}
        cur = None
        for line in text.split("\n"):
            c = re.match(r"column (\w+)", line)
            if c:
                cur = c.group(1)
                cols[cur] = 0.0
            lv = re.match(r"  level \d+ .*? occupancy=([-\d.]+) occupancyBeta=([-\d.]+)", line)
            if lv and cur:
                cols[cur] += float(lv.group(1)) + max(0.0, float(lv.group(2)))
        problems = []
        for i, f in enumerate(frag):
            n = int(re.search(r" (\d+)(?: \(|,)", f).group(1))
            drawn = int(re.search(r"drawn (\d+)", f).group(1))
            core = re.search(r"core-like \d+ levels? (\d+) e", f)
            inner = re.search(r"internal \d+ levels? (\d+) e", f)
            tot = drawn + (int(core.group(1)) if core else 0) + (int(inner.group(1)) if inner else 0)
            sums.append(n)
            if tot != n:
                problems.append("fragment %d: %d drawn+folded of %d" % (i, tot, n))
            if round(cols["left" if i == 0 else "right"]) != drawn:
                problems.append("fragment %d: dump column has %g, note %d" % (i, cols["left" if i == 0 else "right"], drawn))
        if not mm:
            problems.append("no model sum")
        else:
            if int(mm.group(1)) + int(mm.group(2)) != int(mm.group(3)):
                problems.append("model sum")
            if round(cols["centre"]) != int(mm.group(1)):
                problems.append("model column has %g, note %s" % (cols["centre"], mm.group(1)))
            if sum(sums) != int(mm.group(3)):
                problems.append("fragments %d != model %s" % (sum(sums), mm.group(3)))
            ab = re.search(r"Total electrons: (\d+)\*\* \(.*? (\d+), .*? (\d+)\)", text)
            if ab and (int(ab.group(1)) != int(mm.group(3)) or [int(ab.group(2)), int(ab.group(3))] != sums):
                problems.append("About says %s" % ab.group(0))
        if problems:
            bad += 1
            print("%s: electrons: %s" % (fn, "; ".join(problems)))
    print("electron bookkeeping over %d model-first goldens: %s" % (seen, "FAIL" if bad else "ok"))
    return bad


def right_sets(path):
    """The right (ligand) fragment's drawn sets: (label, degeneracy, electrons)."""
    rows, inside = [], False
    for line in open(path):
        if line.startswith("column "):
            inside = line.strip() == "column right"
        elif inside and line.lstrip().startswith("level "):
            f = dict(re.findall(r'(\w+)=("[^"]*"|\S+)', line))
            rows.append((f["label"], int(f["degeneracy"]), float(f["occupancy"])))
    return rows


def check_neutral_ligands(d):
    """Covalent model: the neutral ligand atoms' unpaired electrons sit one to
    each SIGMA ligand group orbital and the pi sets are full.  BF3: a1' 1 e,
    sigma e' 1+1, a2'' and the in-plane pi e' full; water's H2 and NH3's H3
    are the same rule with s sigma orbitals."""
    bad = 0
    want = {
        #  (sets in energy order: label, electrons); core-like F 2s is folded away.
        #  BF3: a1' sigma 1, in-plane pi e' 4, a2'' pi 2, sigma e' 1+1.
        "orca-bf3": [("a1'", 1), ("e'", 4), ("a2''", 2), ("e'", 2)],
        "ecceqm-water": [("a1", 1), ("b2", 1)],
        "orca-nh3-usesym": [("a1", 1), ("e", 2)],
    }
    for name, sets in want.items():
        path = os.path.join(d, name + ".dump")
        if not os.path.exists(path):
            continue
        have = [(l.split(" ")[0].lower() if name != "orca-bf3" else l.split(" ")[0], round(o))
                for l, dg, o in right_sets(path)]
        ok = [(a.lower(), b) for a, b in have] == [(a.lower(), b) for a, b in sets]
        bad += not ok
        print("%-16s neutral ligand fill %s %s" % (name, have, "ok" if ok else "FAIL (want %s)" % sets))
    print("neutral ligand fill: %s" % ("FAIL" if bad else "ok"))
    return bad


def check_ethane(d):
    """Staggered ethane, CH3 + CH3 (#458): the a2u level of FO7 - FO7 (overlap
    population -0.9) is antibonding although it has no partner, and the
    FO7 + FO7 a1g level (population 0.00) is non-bonding."""
    path = os.path.join(d, "ecceqm-c2h6.dump")
    if not os.path.exists(path):
        return 0
    inside, rows = False, []
    for line in open(path):
        if line.startswith("column "):
            inside = line.strip() == "column centre"
        elif inside and line.lstrip().startswith("level "):
            f = dict(re.findall(r'(\w+)=("[^"]*"|\S+)', line))
            op = re.search("overlap population ([\u2212-]?[0-9.]+)", line)
            rows.append((f["label"], f["character"], int(f["pairing"]),
                         float(op.group(1).replace("\u2212", "-")) if op else None))
    bad = 0
    top = [r for r in rows if r[0].lower().startswith("3a2u")]
    nb = [r for r in rows if r[0].lower().startswith("6a1g")]
    if not (top and top[0][1] == "ANTIBONDING" and top[0][2] < 0
            and top[0][3] is not None and top[0][3] < -0.5):
        bad += 1
        print("ecceqm-c2h6: the unpaired a2u level is %s, want ANTIBONDING" % top)
    if not (nb and nb[0][1] == "NONBONDING" and abs(nb[0][3]) < 0.02):
        bad += 1
        print("ecceqm-c2h6: the a1g FO7+FO7 level is %s, want NONBONDING" % nb)
    for r in rows:
        if r[2] < 0 and r[3] is not None:
            want = ("BONDING" if r[3] > 0.02 else
                    "ANTIBONDING" if r[3] < -0.02 else "NONBONDING")
            if r[1] != want:
                bad += 1
                print("ecceqm-c2h6: %s is %s with population %g" % (r[0], r[1], r[3]))
    print("ethane (staggered, CH3 + CH3): %s" % ("FAIL" if bad else "ok"))
    return bad


def check_lines(d):
    """#458: a drawn model level with >= 5 % on a fragment (a 3 % share can be split over sets) has a line to that column."""
    bad = checked = 0
    for name in sorted(os.listdir(d)):
        if not name.endswith(".dump") or "-pi" in name or "lfview" in name or "pick" in name:
            continue
        inside, shares, linked = False, {}, {}
        for line in open(os.path.join(d, name)):
            if line.startswith("column "):
                inside = line.strip() == "column centre"
            elif inside and line.lstrip().startswith("level "):
                f = dict(re.findall(r'(\w+)=("[^"]*"|\S+)', line))
                i = int(line.split()[1])
                if "construct" in line and float(f["shareLeft"]) >= 0 and float(f["shareRight"]) >= 0:
                    shares[i] = (float(f["shareLeft"]), float(f["shareRight"]))
            elif line.lstrip().startswith("link "):
                f = dict(re.findall(r'(\w+)=(-?\d+)', line))
                if "centre" not in f:
                    continue
                c = int(f["centre"])
                linked.setdefault(c, [False, False])
                linked[c][0] |= int(f["left"]) >= 0
                linked[c][1] |= int(f["right"]) >= 0
        for i, (l, r) in shares.items():
            checked += 1
            have = linked.get(i, [False, False])
            if (l >= 0.05 and not have[0]) or (r >= 0.05 and not have[1]):
                print("%s level %d: shares %.3f/%.3f, lines %s: FAIL" % (name, i, l, r, have))
                bad += 1
    print("lines for every share >= 3 %% over %d model levels: %s" % (checked, "ok" if not bad else "FAIL"))
    return bad


def main():
    d = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, "expected")
    bad = 0
    for name, pairs in EXPECT.items():
        lv = molecular_levels(os.path.join(d, name + ".dump"))
        #  A level with no partner keeps its own character (#458): count the paired ones.
        nb = sum(1 for l in lv if l[1] == "BONDING" and l[2] >= 0)
        na = sum(1 for l in lv if l[1] == "ANTIBONDING" and l[2] >= 0)
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
    bad += check_ethane(d)
    bad += check_d_electrons(d)
    bad += check_lfview(d)
    bad += check_electrons(d)
    bad += check_neutral_ligands(d)
    bad += check_lines(d)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
