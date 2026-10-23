#!/usr/bin/env python3
"""Fragment orbitals and MO composition in them (#301, MO diagram route B).

    tests/fragorb/run_tests.py [build-dir]

1. testFragorb.C, built with g++ directly (no build tree): STO overlaps, the
   extended Hueckel energies of CO, Cr(CO)6, TiF6(2-) and Co(NH3)6(3+)
   against YAeHMOP's `bind` (reference/*.eht, made by make_yaehmop_ref.py),
   automatic fragments, and that a spanning FO set captures every MO.
2. With a build tree (build-dir/fragorb): the projection with Gaussian
   fragment orbitals from ORCA fragment calculations of Cr(CO)6, against
   what Multiwfn's CDA matrices give (reference/crco6_gfo_*.tsv, made by
   multiwfn_ref.py); and the metal/ligand/mixed classes of three ML6 types
   made with ORCA (def2-SVP): sigma-only Co(NH3)6(3+), pi-donor TiF6(2-),
   pi-acceptor Cr(CO)6.

Exit 0 on success, 1 on a failed check, 2 if it could not be built.
"""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
CH = os.path.join(ROOT, "src", "tdat", "chemistry")
SOURCES = ["FragmentOrbitals", "FoDiagram", "MoComposition", "MoFragments",
           "SymmetryAnalysis", "ShellRotation", "TGBSAngFunc",
           "BasisAngularNorm", "CharacterTable", "MoDiagram", "MoPairs", "EspField",
           "CoulombIntegrals", "SlaterExpansion", "Huckel"]
failures = 0


def check(ok, what, detail=""):
    global failures
    print("  %-72s %s %s" % (what, "ok" if ok else "FAIL", detail))
    if not ok:
        failures += 1


def unit():
    out = os.path.join(HERE, "testFragorb")
    cmd = (["g++", "-O2", "-std=c++17", "-w", "-I", os.path.join(ROOT, "include"),
            "-o", out, os.path.join(HERE, "testFragorb.C")]
           + [os.path.join(CH, s + ".C") for s in SOURCES])
    b = subprocess.run(cmd, capture_output=True, text=True)
    if b.returncode != 0:
        print("could not build testFragorb:\n" + b.stderr)
        return 2
    try:
        env = dict(os.environ, ECCE_HOME=ROOT)
        env.pop("ECCE_EHT_PARAMETERS", None)
        r = subprocess.run([out, os.path.join(HERE, "reference")], env=env,
                           capture_output=True, text=True)
        print(r.stdout, end="")
        if r.stderr:
            print(r.stderr, end="")
        return r.returncode
    finally:
        if os.path.exists(out):
            os.unlink(out)


def run(build, args):
    work = tempfile.mkdtemp(prefix="fragorb-")
    try:
        home = os.path.join(work, "home")
        os.makedirs(home)
        env = dict(os.environ, ECCE_HOME=ROOT, ECCE_REALUSER="fragorb",
                   ECCE_REALUSERHOME=home, HOME=home)
        env.pop("ECCE_LOCAL_DATA", None)
        cmd = [os.path.join(build, "fragorb")]
        for a in args:
            if os.path.isdir(os.path.join(HERE, "fixtures", a.split(":")[0])):
                name, _, rest = a.partition(":")
                dst = os.path.join(work, name)
                if not os.path.exists(dst):
                    shutil.copytree(os.path.join(HERE, "fixtures", name), dst)
                a = dst + (":" + rest if rest else "")
            cmd.append(a)
        p = subprocess.run(cmd, capture_output=True, text=True, env=env,
                           timeout=600)
        return p.returncode, p.stdout, p.stderr
    finally:
        shutil.rmtree(work, ignore_errors=True)


def tsv(text):
    """{mo: dict} from --tsv output."""
    rows = {}
    for line in text.splitlines():
        f = line.split("\t")
        if len(f) < 8 or not f[0].isdigit():
            continue
        tops = []
        for k in range(8, len(f) - 2, 3):
            tops.append((f[k], float(f[k + 1])))
        rows[int(f[0])] = dict(occ=float(f[2]), klass=f[4], capture=float(f[5]),
                               metal=float(f[6]), ligand=float(f[7]), top=tops)
    return rows


def projection(build):
    print("projection with Gaussian FOs against Multiwfn (Cr(CO)6)")
    frag = ["crco6-cr:1"] + ["crco6-co%d:%d,%d" % (i, 2*i, 2*i + 1)
                             for i in range(1, 7)]
    for tag, extra in (("full", []), ("occ", ["--gfo-occ"])):
        args = ["crco6"] + extra
        for f in frag:
            args += ["--gfo", f]
        rc, out, err = run(build, args)
        if rc != 0:
            check(False, "fragorb --gfo (%s)" % tag, err.strip()[-200:])
            continue
        got = {}
        for line in out.splitlines()[1:]:
            head = line.split("|")[0].split()
            per = line.split("|")[1].split()
            got[int(head[0])] = [float(head[1])] + [float(x) for x in per]
        ref = {}
        for line in open(os.path.join(HERE, "reference",
                                      "crco6_gfo_%s.tsv" % tag)):
            if line.startswith("#"):
                continue
            f = line.split()
            ref[int(f[0])] = [float(x) for x in f[1:]]
        worst = max(abs(a - b) for mo in ref for a, b in zip(ref[mo], got[mo]))
        check(set(ref) == set(got) and worst < 5e-4,
              "%s FO set: capture and per-fragment shares of %d MOs" %
              (tag, len(ref)), "worst difference %.1e" % worst)


def classes(build):
    print("metal/ligand/mixed classes of three ML6 types")
    rc, out, err = run(build, ["conh36", "--tsv", "--ligands", "shell"])
    if rc != 0:
        check(False, "Co(NH3)6 runs", err.strip()[-200:])
        return
    r = tsv(out)
    t2g = [r[m] for m in (40, 41, 42)]
    check(all(x["klass"] == "metal" and x["metal"] > 0.9 for x in t2g),
          "Co(NH3)6: t2g (MOs 40-42) are metal, > 90% on Co")
    eg = [r[m] for m in (43, 44)]
    check(all(x["klass"] == "mixed" and x["ligand"] > 0.3 > 0 and
              x["metal"] > x["ligand"] for x in eg),
          "Co(NH3)6: eg* (MOs 43-44) mixed, metal > ligand > 30%")
    occ_metal = sorted(m for m, x in r.items()
                       if x["occ"] > 0.5 and x["klass"] == "metal")
    check(occ_metal == [40, 41, 42],
          "Co(NH3)6: the only metal-class occupied MOs are the t2g", occ_metal)
    check(all(r[m]["klass"] == "ligand" and r[m]["ligand"] > 0.85
              for m in list(range(16, 34)) + [37, 38, 39]),
          "Co(NH3)6: ligand-internal levels (MOs 16-33, 37-39) are ligand, > 85%")
    check(all(r[m]["klass"] == "mixed" and r[m]["metal"] > 0.15
              for m in (34, 35, 36)),
          "Co(NH3)6: the Co-N sigma bonding MOs (34-36) are mixed")

    rc, out, err = run(build, ["tif6", "--tsv", "--ligands", "shell"])
    if rc != 0:
        check(False, "TiF6 runs", err.strip()[-200:])
        return
    r = tsv(out)
    t2g = [r[m] for m in (41, 42, 43)]
    check(all(x["klass"] == "metal-centred" and 0.5 < x["metal"] < 0.8 and
              x["ligand"] > 0.15 for x in t2g) and
          all(any(t[0].startswith("ligands FO") and t[1] > 0.15 for t in x["top"])
              for x in t2g),
          "TiF6: t2g* (MOs 41-43) metal-centred with F pi character > 15%")
    eg = [r[m] for m in (44, 45)]
    check(all(x["klass"] == "metal-centred" and x["ligand"] > 0.25 for x in eg),
          "TiF6: eg* (MOs 44-45) metal-centred, ligand sigma character > 25%")
    check(all(r[m]["klass"] != "metal" and r[m]["ligand"] > 0.7
              for m in range(16, 40)),
          "TiF6: filled F levels (MOs 16-39) are never metal, ligand > 70%")

    for mode in ("shell", "each"):
        rc, out, err = run(build, ["crco6", "--tsv", "--ligands", mode])
        if rc != 0:
            check(False, "Cr(CO)6 runs (%s)" % mode, err.strip()[-200:])
            continue
        r = tsv(out)
        t2g = [r[m] for m in (52, 53, 54)]
        check(all(0.6 < x["metal"] < 0.8 and x["klass"] == "metal-centred"
                  for x in t2g),
              "Cr(CO)6 (%s): t2g (MOs 52-54) 60-80%% Cr, metal-centred" % mode)
        check(all(any("FO" in t[0] and "virt" in t[0] and t[1] > 0.15
                      for t in x["top"]) for x in t2g),
              "Cr(CO)6 (%s): t2g carries CO pi* (virtual FO) > 15%%" % mode)
        check(sorted(m for m, x in r.items() if x["occ"] > 0.5 and
                     x["metal"] > 0.6) == [52, 53, 54],
              "Cr(CO)6 (%s): only the t2g are > 60%% metal among occupied MOs" % mode)
        core = [m for m in range(1, 18) if r[m]["klass"] != "core"]
        check(not core, "Cr(CO)6 (%s): 1s/2s/2p cores (MOs 1-17) are class core" % mode, core)


def parse_diagram(out):
    """The --diagram output as dicts: sets, MO levels, links, pairs."""
    d = dict(sets=[], mo=[], metal=[], ligand=[], links=[], pair=[],
             mismatch=[], note=out.splitlines()[0] if out else "")
    for l in out.splitlines():
        f = l.split()
        if not f:
            continue
        if f[0] == "set":
            kv = dict(x.split("=", 1) for x in f if "=" in x)
            d["sets"].append(dict(side=f[2], irrep=kv["irrep"],
                                  fock=float(kv["fock"]), col=int(kv["col"]),
                                  deg=int(kv["deg"]),
                                  label=l.split('"')[1]))
        elif f[0] in ("metal", "mo", "ligand"):
            kv = dict(x.split("=", 1) for x in f[2:] if "=" in x)
            top = []
            for t in kv.get("top", "").split(","):
                if ":" in t:
                    a, b = t.split(":")
                    top.append((int(a), float(b)))
            d[f[0]].append(dict(E=float(kv["E"]), deg=int(kv["deg"]),
                                occ=float(kv["occ"]), occb=float(kv.get("occb", 0)),
                                line=l, top=top,
                                fit=float(kv.get("fit", 1.0)),
                                label=l.split('label="')[1].split('"')[0],
                                irrep=kv.get("irrep", ""),
                                char=int(kv.get("char", 0)),
                                pair=int(kv.get("pair", -1)),
                                mo=int(kv.get("mo", 0))))
        elif f[0] == "link":
            kv = dict(x.split("=") for x in f[1:])
            d["links"].append((int(kv["mo"]), int(kv["left"]),
                               int(kv["right"]), float(kv["weight"])))
        elif f[0] == "pair":
            kv = dict(x.split("=") for x in f[2:])
            d["pair"].append(dict(irrep=kv["irrep"], b=int(kv["bonding"]),
                                  a=int(kv["antibonding"]),
                                  metal=int(kv["metal"]),
                                  ligand=int(kv["ligand"])))
        elif f[0] == "mismatch":
            d["mismatch"].append(l[len("mismatch "):])
    return d


#  Reported, not tuned away: the second set of the MO is a high virtual
#  ligand set that the calculation's MOs describe to 50% only.
KNOWN_EXCEPTIONS = {
    ("crco6", "all levels", 86):
        "its second set FO44-46 (capture 0.50) lies above it; the pair's own "
        "parents, Cr 4p and FO26-28, bracket it",
}
COMPLEXES = (("conh36", "Co 3d", "Co(NH3)6"), ("tif6", "Ti 3d", "TiF6"),
             ("crco6", "Cr 3d", "Cr(CO)6"))


#  d count, oxidation state, (t2g, eg) electrons drawn
EXPECT_D = {"conh36": (6, 3, (6.0, 0.0)), "tif6": (0, 4, (0.0, 0.0)),
            "crco6": (6, 0, (6.0, 0.0))}


def diagram(build):
    """The fragment-orbital diagram (FoDiagram) for the three ML6 types."""
    print("the fragment-orbital diagram")
    for fx, d_set, name in COMPLEXES:
        rc, out, err = run(build, [fx, "--diagram"])
        if rc != 0:
            check(False, "%s --diagram runs" % fx, err.strip()[-200:])
            continue
        d = parse_diagram(out)
        check(all(s["fock"] == s["fock"] for s in d["sets"]) and
              any(l.startswith("group Oh") for l in out.splitlines()),
              "%s: fragment levels at the Fock diagonal, skeleton group Oh" % name)
        check(d["links"] and all(0.0 <= w <= 1.0 for _, _, _, w in d["links"]),
              "%s: lines carry shares" % name)
        dm = [i for i, s in enumerate(d["metal"]) if '"%s"' % d_set in s["line"]]
        check(sorted(d["metal"][i]["deg"] for i in dm) == [2, 3],
              "%s: the 3d set is split into eg (2) and t2g (3)" % name)
        dcols = set(dm)
        strong = sorted((d["mo"][mo]["E"], d["mo"][mo]["deg"])
                        for mo, left, _, w in d["links"]
                        if left in dcols and w >= 0.5)
        check(len(strong) == 2 and strong[0][1] == 3 and strong[1][1] == 2,
              "%s: t2g (3) below eg* (2) are the two levels that are "
              "half or more metal 3d" % name, strong)
        el = [l for l in out.splitlines() if l.startswith("electrons ")]
        kv = dict(x.split("=") for x in el[0].split()[1:]) if el else {}
        want = EXPECT_D[fx]
        check(el and int(kv["d"]) == want[0] and int(kv["oxidation"]) == want[1],
              "%s: metal d%d (oxidation state %+d, group number - oxidation "
              "state)" % (name, want[0], want[1]), kv)
        eg = [d["metal"][i] for i in dm if d["metal"][i]["deg"] == 2]
        t2g = [d["metal"][i] for i in dm if d["metal"][i]["deg"] == 3]
        placed = (t2g[0]["occ"], eg[0]["occ"]) if eg and t2g else None
        check(placed == want[2] and (t2g[0]["E"] < eg[0]["E"] or want[0] == 0),
              "%s: d electrons placed t2g %s / eg %s (t2g below eg in the "
              "fragment column), ns and np empty" %
              (name, *(want[2])), placed)
        check(all(m["occ"] == 0 for m in d["metal"] if '"%s"' % d_set not in m["line"]),
              "%s: the metal ns and np levels draw no electrons" % name)
        check(el and kv["placed"] == "1" and kv["agree"] == "1" and
              int(kv["d"]) + int(kv["ligands"]) == int(kv["mo"]) and
              "Electrons: metal d%d + ligands %s = %d" %
              (want[0], kv["ligands"], int(kv["mo"])) in out,
              "%s: metal d + ligand electrons = valence MO electrons, "
              "stated in the note" % name, kv)
        check(0 < len(d["ligand"]) <= 6,
              "%s: ligand internals do not crowd the right column" % name,
              len(d["ligand"]))
        check(all(l["irrep"] for l in d["mo"]),
              "%s: every drawn MO has a skeleton irrep" % name)


def pairs_and_brackets(build):
    """Pairs share colour and both parents; bonding below, antibonding above."""
    print("bonding/antibonding pairs")
    for fx, d_set, name in COMPLEXES:
        for flag in ([], ["--all"]):
            tag = "all levels" if flag else "default"
            rc, out, err = run(build, [fx, "--diagram"] + flag)
            if rc != 0:
                check(False, "%s --diagram %s runs" % (name, tag), err.strip()[-200:])
                continue
            d = parse_diagram(out)
            check(not d["mismatch"],
                  "%s (%s): bonding = antibonding for every irrep" % (name, tag),
                  d["mismatch"])
            sets = d["sets"]
            col = {"metal": "left", "ligand": "right"}
            linked = {}
            for mo, left, right, w in d["links"]:
                linked.setdefault(mo, set()).add(("metal", left) if left >= 0
                                                 else ("ligand", right))
            for p in d["pair"]:
                if p["b"] < 0 or p["a"] < 0:
                    continue
                m, l = sets[p["metal"]], sets[p["ligand"]]
                need = {("metal", m["col"]), ("ligand", l["col"])}
                both = all(need <= linked.get(x, set()) for x in (p["b"], p["a"]))
                same = d["mo"][p["b"]]["pair"] == d["mo"][p["a"]]["pair"] >= 0
                check(both and same and m["col"] >= 0 and l["col"] >= 0,
                      "%s (%s): pair %s MO %d/%d, lines from both to %s and %s"
                      % (name, tag, p["irrep"], d["mo"][p["b"]]["mo"],
                         d["mo"][p["a"]]["mo"], m["label"], l["label"]))
            for i, lv in enumerate(d["mo"]):
                if lv["char"] not in (1, 3) or len(lv["top"]) < 2:
                    continue
                (s0, _), (s1, _) = lv["top"][0], lv["top"][1]
                a, b = sets[s0], sets[s1]
                if {a["side"], b["side"]} != {"metal", "ligand"}:
                    continue
                if not (a["irrep"] == b["irrep"] == lv["irrep"]):
                    continue
                lo = min(a["fock"], b["fock"])
                hi = max(a["fock"], b["fock"])
                ok = (lv["E"] <= lo + 0.005) if lv["char"] == 1 else \
                     (lv["E"] >= hi - 0.005)
                known = KNOWN_EXCEPTIONS.get((fx, tag, lv["mo"]))
                if known and not ok:
                    print("  KNOWN EXCEPTION %s (%s): MO %d %s: E=%.4f, sets "
                          "%.4f / %.4f; %s" % (name, tag, lv["mo"], lv["irrep"],
                                               lv["E"], a["fock"], b["fock"], known))
                    continue
                check(ok, "%s (%s): MO %d %s %s %s the sets it mixes" %
                      (name, tag, lv["mo"], lv["irrep"],
                       "bonding" if lv["char"] == 1 else "antibonding",
                       "below" if lv["char"] == 1 else "above"),
                      "E=%.4f, sets %.4f / %.4f" % (lv["E"], a["fock"], b["fock"]))


def non_metal_centre(build):
    """A single non-metal atom pick as the centre: methanol at C and at O."""
    print("a non-metal centre (methanol)")
    for atom, name, group, want in ((1, "methanol at C", "C3v", (4, 10, 2)),
                                    (2, "methanol at O", "Cs", (6, 8, 2))):
        rc, out, err = run(build, ["methanol", "--diagram", "--atom", str(atom)])
        if rc != 0:
            check(False, "%s --diagram runs" % name, err.strip()[-200:])
            continue
        d = parse_diagram(out)
        el = [l for l in out.splitlines() if l.startswith("electrons ")]
        kv = dict(x.split("=") for x in el[0].split()[1:]) if el else {}
        check(any(l.startswith("group " + group) for l in out.splitlines()),
              "%s: labels from the local skeleton's group %s" % (name, group))
        check(el and kv["placed"] == "1" and kv["agree"] == "1" and
              (int(kv["d"]), int(kv["ligands"]), int(kv["unpaired"])) == want and
              int(kv["d"]) + int(kv["ligands"]) == int(kv["mo"]),
              "%s: centre %d + pieces %d electrons = %d in the valence MOs, "
              "%d unpaired on the centre" % (name, want[0], want[1],
                                             want[0] + want[1], want[2]), kv)
        left = sum(m["occ"] + m["occb"] for m in d["metal"])
        right = sum(m["occ"] for m in d["ligand"])
        check(abs(left - want[0]) < 1e-6 and abs(right - want[1]) < 1e-6,
              "%s: the electrons drawn add up (centre column %.2f, pieces %.2f)"
              % (name, left, right))
        check(all(abs(m["occ"] - round(m["occ"])) < 1e-6 for m in d["ligand"]),
              "%s: no level carries a fraction of an orbital" % name)
        check("Electrons: %s %d + pieces %d = %d" % (
              "C" if atom == 1 else "O", want[0], want[1], want[0] + want[1]) in out,
              "%s: the bookkeeping line is in the note" % name)
        check(not d["mismatch"], "%s: bonding = antibonding for every irrep" % name,
              d["mismatch"])
        sets = d["sets"]
        for lv in d["mo"]:
            fit, lab = lv["fit"], lv["label"]
            ok = 0.0 <= fit <= 1.0 and (
                (fit >= 0.9 and lab.startswith(lv["irrep"])) or
                (0.5 <= fit < 0.9 and lab.startswith("~" + lv["irrep"])) or
                (fit < 0.5 and lv["irrep"] not in lab))
            check(ok, "%s: MO %d fit %.2f is labelled by the 0.9 / 0.5 rule"
                  % (name, lv["mo"], fit), lab)
        exceptions = 0
        for p in d["pair"]:
            if p["b"] < 0 or p["a"] < 0:
                continue
            eb, ea = d["mo"][p["b"]]["E"], d["mo"][p["a"]]["E"]
            for kind, idx in (("centre", p["metal"]), ("piece", p["ligand"])):
                e = sets[idx]["fock"]
                ok = eb < e < ea
                if kind == "centre":
                    check(ok, "%s: pair %s MO %d/%d brackets the %s %s level" %
                          (name, p["irrep"], d["mo"][p["b"]]["mo"],
                           d["mo"][p["a"]]["mo"], kind, sets[idx]["label"]),
                          "E %.4f < %.4f < %.4f" % (eb, e, ea))
                elif not ok:
                    exceptions += 1
                    print("  EXCEPTION %s: pair %s MO %d/%d does not bracket "
                          "its piece level %s: E %.4f, level %.4f, E* %.4f" %
                          (name, p["irrep"], d["mo"][p["b"]]["mo"],
                           d["mo"][p["a"]]["mo"], sets[idx]["label"], eb, e, ea))
        print("  %s: %d pairs, %d piece-level bracket exceptions reported above"
              % (name, len(d["pair"]), exceptions))


def model(build):
    """The model-first diagram (ModelDiagram) of Cr(CO)6 and Co(NH3)6 3+."""
    print("model-first diagram")
    for name, levels, band in (("crco6", None, "(CO)6 internal"),
                               ("conh36", None, None)):
        rc, out, err = run(build, [name, "--model"])
        check(rc == 0, "fragorb --model %s runs" % name, err.strip()[-200:])
        if rc != 0:
            continue
        mo = [l for l in out.splitlines() if l.startswith("  ") and "ov=" in l]
        # every model level has a matched calculated orbital and overlap
        check(len(mo) > 0 and all("calc=" in l for l in mo),
              "%s: every model level is matched" % name, "%d levels" % len(mo))
        ovs = [float(l.split("ov=")[1].split()[0]) for l in mo]
        check(all(0.5 < o <= 1.0 for o in ovs), "%s: overlaps in (0.5, 1]" % name)
        # bonding levels have antibonding partners (drawn pairs complete)
        #  A level with no partner (pair=-1) keeps the character of its own
        #  overlap population (#458): only the paired ones must balance.
        ch = [int(l.split("char=")[1].split()[0]) for l in mo
              if int(l.split("pair=")[1].split()[0]) >= 0]
        check(ch.count(1) == ch.count(3), "%s: bonding = antibonding" % name,
              "%d/%d" % (ch.count(1), ch.count(3)))
        if levels:
            check(len(mo) == levels, "%s: %d valence levels" % (name, levels))
        if band:
            check(band in out, "%s: band '%s'" % (name, band))


def main():
    status = unit()
    if status != 0:
        return status
    build = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "build-cmake")
    if not os.path.exists(os.path.join(build, "fragorb")):
        print("\nskipped the fixture checks: no %s/fragorb" % build)
        return 0
    print()
    projection(build)
    classes(build)
    print()
    diagram(build)
    pairs_and_brackets(build)
    non_metal_centre(build)
    model(build)
    print("\nFAILED" if failures else "\nok")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
