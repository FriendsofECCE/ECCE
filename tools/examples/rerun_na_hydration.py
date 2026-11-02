#!/usr/bin/env python3
"""Rerun the Na+(H2O)6 workflow example the way ECCE writes and runs it (#712).

    tools/examples/rerun_na_hydration.py [--build build-cmake] [--arr 5_1 ...]
        [--only xtb|orca] [--procs 2] [--keep]

For each arrangement (6_0, 5_1, 4_2): the xtb deck is made by the real
ai.xtb from the stored starting structure, run through Launch (gensub, so
--molden and the appended frames/Molden/charges come with it), then the ORCA
deck is made by the real ai.orca (it always writes the MO print) from the
xtb result and run the same way.  The two stored outputs replace those in
data/client/examples/workflows/na-hydration/; run tools/examples/preparse.py
workflows afterwards.  Both outputs are self-contained (the xtb output holds
its frames and Molden file), so the old companion files are removed.

Needs xtb on PATH and ORCA 6 under /opt/orca, and the built binaries as
tests/launch/xtb_test.py does.
"""
import argparse
import glob
import os
import re
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))

import harness  # noqa: E402
from harness import say  # noqa: E402

TOPIC = os.path.join(REPO, "data", "client", "examples", "workflows",
                     "na-hydration")
PARSERS = os.path.join(REPO, "scripts", "parsers")
ARRANGEMENTS = ("6_0", "5_1", "4_2")


def readXyz(path, frame=-1):
    lines = open(path).read().split("\n")
    frames, i = [], 0
    while i < len(lines) and lines[i].strip():
        n = int(lines[i])
        frames.append([l.split()[:4] for l in lines[i + 2:i + 2 + n]])
        i += n + 2
    return frames[frame]


def lastFrame(output):
    """The last ECCE-XTB-FRAME of an xtb output, as [element, x, y, z]."""
    blocks = re.findall(r"ECCE-XTB-FRAME \d+ (\d+)\n(.*?)ECCE-XTB-ENDFRAME",
                        output, re.S)
    n, body = blocks[-1]
    return [l.split()[:4] for l in body.strip("\n").split("\n")[1:]]


def aiRun(code, work, atoms, param, basis=None):
    """Run ai.<code> on a fresh folder; returns the deck text."""
    os.makedirs(work, exist_ok=True)
    frag = ["x", "title:\tx", "type:\tmolecule", "num_atoms:\t%d" % len(atoms),
            "atom_info:\tsymbol cart", "atom_list:"]
    frag += [" %s %s %s %s" % tuple(a) for a in atoms]
    open(os.path.join(work, "x.frag"), "w").write("\n".join(frag) + "\n")
    open(os.path.join(work, "x.param"), "w").write(param)
    flags = ["-p", "-f"]
    if code == "orca":
        open(os.path.join(work, "x.basis"), "w").write(basis)
        flags.append("-b")
        tpl, name = "orca.tpl", "deck"
    else:
        tpl, name = "xtb.xtbin", "deck"
    deck = os.path.join(work, name)
    if code == "orca":
        shutil.copy(os.path.join(PARSERS, tpl), deck)
    else:
        open(deck, "w").write("##XtbHeader##\n##chemsys##\n")
    subprocess.run(["perl", os.path.join(PARSERS, "ai." + code)] + flags +
                   ["-n", "x", "-t", deck], cwd=work, check=True,
                   env=dict(os.environ, ECCE_HOME=REPO), capture_output=True)
    return deck


def runStep(s, name, resource, deck, deckName, rundir, procs, atoms, work):
    xyz = os.path.join(work, name + ".xyz")
    with open(xyz, "w") as h:
        h.write("%d\n%s\n" % (len(atoms), name))
        for a in atoms:
            h.write("%s %s %s %s\n" % tuple(a))
    code = "xTB" if resource == "xtb_es" else "ORCA"
    url, out = s.create(name, resource, deck, deckName, rundir,
                        extra=("procs=%d" % procs, "code=" + code,
                               "xyz=" + xyz))
    if url is None:
        raise SystemExit("create %s failed:\n%s" % (name, out))
    t0 = time.time()
    rc, out = s.launch(url)
    if rc != 0:
        raise SystemExit("launch %s failed:\n%s" % (name, out))
    state = s.waitState(url, seconds=6 * 3600)
    say("%s: %s after %.1f min" % (name, state, (time.time() - t0) / 60))
    if state != "completed":
        raise SystemExit("%s: %s\n%s" % (name, state, s.driver("reason", url)[1]))
    time.sleep(2)
    return url[len("file://"):] if url.startswith("file://") else url


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--arr", nargs="*", default=list(ARRANGEMENTS))
    ap.add_argument("--only", choices=("xtb", "orca"))
    ap.add_argument("--procs", type=int, default=2)
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    xtb = shutil.which("xtb")
    orcas = sorted(glob.glob("/opt/orca/orca_6*/orca"))
    if not xtb or not orcas:
        harness.skip("xtb or ORCA 6 is not installed")
    harness.prerequisites(build, ())
    s = harness.Session(build, "na712", {"xTB": xtb, "ORCA": orcas[-1]},
                        local=True, keep=args.keep)
    work = os.path.join(s.state, "decks")
    rundir = os.path.join(s.state, "run")
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(rundir, exist_ok=True)
    try:
        if not s.services(True):
            raise SystemExit("services did not come up")
        for arr in args.arr:
            xtbDir = os.path.join(TOPIC, "xtb-na_" + arr)
            orcaDir = os.path.join(TOPIC, "orca-na_" + arr)
            start = readXyz(os.path.join(xtbDir, "na_%s.xyz" % arr)) \
                if os.path.exists(os.path.join(xtbDir, "na_%s.xyz" % arr)) \
                else None
            xtbOut = os.path.join(xtbDir, "na_%s_xtb.out" % arr)
            if args.only != "orca":
                #  The start structure is kept in the repo as the first
                #  frame of the stored output.
                start = start or lastFrame(open(xtbOut).read())
                deck = aiRun("xtb", os.path.join(work, "x" + arr), start,
                             "title:\tx\nTheory:\tGFN2-xTB\nCategory:\tSE\n"
                             "RunType:\tGeometry\nCharge:\t1\n"
                             "ES.ChemSys.Multiplicity:\t1\n"
                             "ES.Runtype.GeomOpt.Level:\tnormal\n")
                say("xtb deck line 1: " + open(deck).readline().strip())
                calc = runStep(s, "xtb-na_" + arr, "xtb_es", deck,
                               "xtb.xtbin", rundir, args.procs, start, work)
                out = open(os.path.join(calc, "Outputs", "xtb.xtbout"),
                           errors="replace").read()
                shutil.copy(os.path.join(calc, "Outputs", "xtb.xtbout"), xtbOut)
                for old in ("charges", "xtbopt.log", "xtbopt.xyz"):
                    if os.path.exists(os.path.join(xtbDir, old)):
                        os.unlink(os.path.join(xtbDir, old))
            else:
                out = open(xtbOut).read()
            if args.only == "xtb":
                continue
            geom = lastFrame(out)
            deck = aiRun("orca", os.path.join(work, "o" + arr), geom,
                         "Category: DFT\nTheory: RDFT\nRunType: GeoVib\n"
                         "Charge: 1\nChemSys.Multiplicity: 1\n"
                         "ES.Theory.DFT.XCFunctionals: r2SCAN-3c\n",
                         basis="useRouteCard\n")
            say("orca route: " + [l for l in open(deck)
                                  if l.startswith("!")][0].strip())
            calc = runStep(s, "orca-na_" + arr, "orca_es", deck,
                           "orca.orcain", rundir, args.procs, geom, work)
            shutil.copy(os.path.join(calc, "Outputs", "orca.orcaout"),
                        os.path.join(orcaDir, "na_%s_orca.out" % arr))
            shutil.copy(os.path.join(calc, "Inputs", "orca.orcain"),
                        os.path.join(orcaDir, "na_%s_orca.inp" % arr))
            say("step %s done" % arr)
    finally:
        s.stop()


if __name__ == "__main__":
    main()
