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
SOURCES = ["FragmentOrbitals", "MoComposition", "MoFragments",
           "SymmetryAnalysis", "ShellRotation", "TGBSAngFunc",
           "BasisAngularNorm", "CharacterTable", "MoDiagram", "EspField",
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
    check(all(x["klass"] == "mixed" and 0.5 < x["metal"] < 0.8 and
              x["ligand"] > 0.15 for x in t2g) and
          all(any(t[0].startswith("ligands FO") and t[1] > 0.15 for t in x["top"])
              for x in t2g),
          "TiF6: t2g* (MOs 41-43) mixed with F pi character > 15%")
    eg = [r[m] for m in (44, 45)]
    check(all(x["klass"] == "mixed" and x["ligand"] > 0.25 for x in eg),
          "TiF6: eg* (MOs 44-45) mixed, ligand sigma character > 25%")
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
        check(all(0.6 < x["metal"] < 0.8 and x["klass"] == "mixed" for x in t2g),
              "Cr(CO)6 (%s): t2g (MOs 52-54) 60-80%% Cr, mixed" % mode)
        check(all(any("FO" in t[0] and "virt" in t[0] and t[1] > 0.15
                      for t in x["top"]) for x in t2g),
              "Cr(CO)6 (%s): t2g carries CO pi* (virtual FO) > 15%%" % mode)
        check(sorted(m for m, x in r.items() if x["occ"] > 0.5 and
                     x["metal"] > 0.6) == [52, 53, 54],
              "Cr(CO)6 (%s): only the t2g are > 60%% metal among occupied MOs" % mode)
        core = [m for m in range(1, 18) if r[m]["klass"] != "core"]
        check(not core, "Cr(CO)6 (%s): 1s/2s/2p cores (MOs 1-17) are class core" % mode, core)


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
    print("\nFAILED" if failures else "\nok")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
