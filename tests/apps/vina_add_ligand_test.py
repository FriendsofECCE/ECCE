#!/usr/bin/env python3
"""A structure added to a Vina calculation is placed clear of the receptor and
becomes its ligand (#1297); in any other calculation Add Structure is as before.

A receptor-only PDB is stored as a Vina calculation and as an xTB one.  The
Builder script's `import` (File > Add Structure from File) adds a ligand file
whose coordinates sit 200 A away.  In the Vina calculation the added atoms must
end up just outside the receptor's bounding box, be selected and be the stored
ES.Runtype.Docking.LigandAtoms; in the xTB one they keep their own coordinates
and nothing is stored or selected.  `vinaligandinfo` reports the state.

    tests/apps/vina_add_ligand_test.py [--build build-cmake] [--keep]

Exit status 77 (skip) without Xvfb or the build.
"""
import argparse
import os
import re
import shutil
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))

import builder_overlay_test as bo  # noqa: E402
import harness  # noqa: E402
from harness import say  # noqa: E402

FIXTURE = os.path.join(HERE, "fixtures", "select-protein-ligand.pdb")
LIGAND = ["HETATM%5d  C%-2d LIG B 900    %8.3f%8.3f%8.3f  1.00  0.00           C"
          % (i + 1, i + 1, 200.0 + 1.5 * i, 200.0, 200.0) for i in range(4)]
failures = []


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def parse(line):
    m = re.search(r"ligandatoms=(\S+) selected=(\S+) natoms=(\d+) receptor ([-\d. ]+) ligand ([-\d. ]+)", line)
    if not m:
        return None
    return {"stored": m.group(1), "sel": m.group(2), "n": int(m.group(3)),
            "rec": [float(x) for x in m.group(4).split()],
            "lig": [float(x) for x in m.group(5).split()]}


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    if not shutil.which("Xvfb"):
        harness.skip("Xvfb is not installed")
    harness.prerequisites(build, ())
    for exe in bo.GUI:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    codes = {"Vina": shutil.which("vina") or "/bin/true", "xTB": shutil.which("xtb") or "/bin/true"}
    s = harness.Session(build, "vinaadd", codes, local=True, keep=args.keep)
    for exe in bo.GUI:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    work = os.path.join(s.state, "decks")
    rundir = os.path.join(s.state, "run")
    os.makedirs(work, exist_ok=True)
    os.makedirs(rundir, exist_ok=True)
    receptor = os.path.join(work, "receptor.pdb")
    with open(FIXTURE) as src, open(receptor, "w") as out:
        out.writelines(l for l in src if l.startswith("ATOM"))
    ligand = os.path.join(work, "ligand.pdb")
    with open(ligand, "w") as out:
        out.write("\n".join(LIGAND) + "\nEND\n")

    display = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        display = xdisplay.Display().__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        results = {}
        for tag, rtype, deck, dname, code in (
                ("vina", "vina_es", "vina.vinain", "vina.vinain", "Vina"),
                ("other", "xtb_es", "xtb.xtbin", "xtb.xtbin", "xTB")):
            name = "add-%s-%d" % (tag, int(time.time()))
            stub = os.path.join(work, name + "." + dname.split(".")[1])
            open(stub, "w").write("# placeholder\n")
            url, out = s.create(name, rtype, stub, dname, rundir,
                                extra=("procs=1", "code=" + code, "pdb=" + receptor))
            if not check(url is not None, "%s calculation created" % tag):
                say(out)
                return 1
            text, _ = bo.runScript(
                s, display, env, wrappers, ["-context", url],
                ["wait 8000", "vinaligandinfo", "import " + ligand, "wait 2000",
                 "vinaligandinfo", "quit"], os.path.join(s.state, "png"), tag)
            infos = [parse(l) for l in text.splitlines() if "vinaligandinfo:" in l]
            for l in text.splitlines():
                if "vinaligandinfo:" in l:
                    say("    " + l.strip()[:220])
            results[tag] = [i for i in infos if i]

        v = results["vina"]
        if check(len(v) == 2, "Vina: the state was reported before and after the add"):
            before, after = v
            check(after["n"] == before["n"] + 4, "Vina: 4 atoms added (%d -> %d)" % (before["n"], after["n"]))
            first, last = before["n"] + 1, after["n"]
            want = "%d-%d" % (first, last)
            check(after["stored"] == want, "Vina: LigandAtoms is %s (got %s)" % (want, after["stored"]))
            check(after["sel"] == want, "Vina: the added atoms are selected (%s)" % after["sel"])
            rec, lig = after["rec"], after["lig"]
            check(lig[0] > rec[3] and lig[0] - rec[3] < 15,
                  "Vina: ligand min x %.1f is clear of the receptor max x %.1f, within 15 A" % (lig[0], rec[3]))
            check(all(rec[k] <= lig[k + 3] and lig[k] <= rec[k + 3] for k in (1, 2)),
                  "Vina: ligand overlaps the receptor box in y and z, so it is not far off")
        o = results["other"]
        if check(len(o) == 2, "xTB: the state was reported before and after the add"):
            before, after = o
            check(after["n"] == before["n"] + 4, "xTB: 4 atoms added")
            check(after["stored"] == "-", "xTB: no LigandAtoms stored (%s)" % after["stored"])
            check(after["sel"] != "%d-%d" % (before["n"] + 1, after["n"]),
                  "xTB: the added atoms are not selected as a ligand (%s)" % after["sel"])
            check(after["lig"][0] > 150, "xTB: added atoms keep their own coordinates (min x %.1f)" % after["lig"][0])
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    if failures:
        say("FAILED: %d" % len(failures))
        return 1
    say("ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
