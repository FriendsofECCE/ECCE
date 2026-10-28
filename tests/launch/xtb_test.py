#!/usr/bin/env python3
"""xtb through the real Launch (#362): water energy and a methane optimisation.

Local data, no data server.  The decks are made by the real ai.xtb, stored
with a new calculation and run on `localhost` through Launch, gensub (sub
xtb), eccejobmaster, eccejobstore and eccejobmonitor.  Each must reach
"completed" with what the Builder reads stored in the calculation:

  * water, GFN2-xTB energy: TE equal to the energy in the stored output,
    HLGAP, DIPOLE and a one-frame GEOMTRACE;
  * methane, GFN2-xTB --opt normal: GEOMTRACE with one frame per cycle and a
    TEVEC of the same length, TE equal to the stored output's final energy.

    tests/launch/xtb_test.py [--build build-cmake] [--keep]

Exit status 77 (CTest SKIP) without xtb or the built binaries.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import harness  # noqa: E402
from harness import say  # noqa: E402

failures = []

WATER = [("O", 0.0, 0.0, 0.1173), ("H", 0.0, 0.7572, -0.4692),
         ("H", 0.0, -0.7572, -0.4692)]
METHANE = [("C", 0.0, 0.0, 0.0), ("H", 0.7, 0.7, 0.7), ("H", -0.7, -0.7, 0.8),
           ("H", -0.7, 0.7, -0.7), ("H", 0.7, -0.8, -0.7)]


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def deck(atoms, runtype, level="normal", theory="GFN2-xTB"):
    """The input deck ai.xtb writes for a molecule."""
    tmp = tempfile.mkdtemp(prefix="xtbdeck-")
    try:
        open(os.path.join(tmp, "x.param"), "w").write(
            "title:\txtbtest\nTheory:\t%s\nCategory:\tSE\n"
            "RunType:\t%s\nCharge:\t0\nES.ChemSys.Multiplicity:\t1\n"
            "ES.Runtype.GeomOpt.Level:\t%s\n" % (theory, runtype, level))
        frag = ["x", "title:\txtbtest", "type:\tmolecule",
                "num_atoms:\t%d" % len(atoms), "atom_info:\tsymbol cart",
                "atom_list:"]
        frag += [" %s %s %s %s" % a for a in atoms]
        open(os.path.join(tmp, "x.frag"), "w").write("\n".join(frag) + "\n")
        open(os.path.join(tmp, "xtb.xtbin"), "w").write(
            "##XtbHeader##\n##chemsys##\n")
        env = dict(os.environ, ECCE_HOME=REPO)
        res = subprocess.run(
            ["perl", os.path.join(REPO, "scripts", "parsers", "ai.xtb"),
             "-p", "-f", "-n", "x", "-t", "xtb.xtbin"],
            cwd=tmp, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        text = open(os.path.join(tmp, "xtb.xtbin")).read()
        return text, res.returncode
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def calcDir(url):
    path = url.rstrip("/")
    return path[len("file://"):] if path.startswith("file://") else path


def numbers(calc, key):
    with open(os.path.join(calc, "Props", key), errors="replace") as handle:
        body = handle.read().split(">", 2)[2]
    return [float(v) for v in re.findall(r"[-+]?\d+\.\d+(?:[eE][-+]?\d+)?", body)]


def runJob(s, name, text, work, rundir, atoms=None):
    path = os.path.join(work, name + ".xtbin")
    open(path, "w").write(text)
    #  The code and the structure, as the Calculation Editor stores them:
    #  the Builder's orbital views look the basis ordering up by code.
    extra = ["procs=1", "code=xTB"]
    if atoms:
        xyz = os.path.join(work, name + ".xyz")
        with open(xyz, "w") as h:
            h.write("%d\n%s\n" % (len(atoms), name))
            for a in atoms:
                h.write("%s %.6f %.6f %.6f\n" % a)
        extra.append("xyz=" + xyz)
    url, out = s.create(name, "xtb_es", path, "xtb.xtbin", rundir,
                        extra=tuple(extra))
    if not check(url is not None, "%s created" % name):
        say(out)
        return None
    rc, out = s.launch(url)
    say("\n".join("  | " + l for l in out.strip().splitlines()[-6:]))
    if not check(rc == 0, "%s: Launch ran to the end" % name):
        return None
    state = s.waitState(url, seconds=300)
    if not check(state == "completed",
                 "%s reached completed (last: %s)" % (name, state)):
        say(s.driver("reason", url)[1])
        return None
    time.sleep(2)
    return calcDir(url)


def storedOutput(calc):
    path = os.path.join(calc, "Outputs", "xtb.xtbout")
    check(os.path.exists(path), "the output was stored as Outputs/xtb.xtbout")
    return open(path, errors="replace").read() if os.path.exists(path) else ""


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    xtb = shutil.which("xtb")
    if not xtb:
        harness.skip("xtb is not installed")
    harness.prerequisites(build, ())

    s = harness.Session(build, "xtb", {"xTB": xtb}, local=True, keep=args.keep)
    work = os.path.join(s.state, "decks")
    rundir = os.path.join(s.state, "run")
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    os.makedirs(rundir, exist_ok=True)
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1

        say("--- water, GFN2-xTB energy")
        text, rc = deck(WATER, "Energy")
        check(rc == 0 and text.startswith("xtb --gfn 2 --chrg 0 --uhf 0 --sp"),
              "ai.xtb wrote the deck: %r" % text.splitlines()[0])
        calc = runJob(s, "w-%d" % int(time.time()), text, work, rundir,
                      WATER)
        if calc:
            out = storedOutput(calc)
            props = s.props("file://" + calc)
            say("  properties: " + " ".join(props))
            for key in ("TE", "HLGAP", "DIPOLE"):
                check(key in props, "%s present in Props/" % key)
            #  PropertyTask::propertyNames lists GEOMTRACE only for more than
            #  one step; the single frame is still stored (checked below).
            check("GEOMTRACE" not in props and os.path.exists(
                os.path.join(calc, "Props", "GEOMTRACE")),
                "GEOMTRACE stored as one frame, unlisted for a single point")
            check("TEVEC" not in props, "no energy trace for a single point")
            for key in ("MO", "ORBENG", "ORBOCC", "MOAOORDER", "GTOBASIS"):
                check(key in props, "%s present in Props/ (xtb --molden)" % key)
            check(sum(numbers(calc, "ORBOCC")) == 8, "ORBOCC sums to 8 electrons")
            check(len(numbers(calc, "MO")) == 36, "MO is 6 orbitals x 6 basis functions")
            e = re.findall(r"\| TOTAL ENERGY\s+(\S+) Eh", out)
            te = numbers(calc, "TE")
            check(e and te and abs(te[-1] - float(e[-1])) < 1e-9,
                  "TE %s is the energy in the stored output %s" % (te[-1:], e[-1:]))
            check(len(numbers(calc, "GEOMTRACE")) == 9, "one frame of 3 atoms")

        say("--- methane, GFN2-xTB geometry optimisation")
        text, rc = deck(METHANE, "Geometry", "normal")
        check(rc == 0 and "--opt normal" in text.splitlines()[0],
              "ai.xtb wrote the deck: %r" % text.splitlines()[0])
        calc = runJob(s, "m-%d" % int(time.time()), text, work, rundir,
                      METHANE)
        if calc:
            out = storedOutput(calc)
            props = s.props("file://" + calc)
            say("  properties: " + " ".join(props))
            for key in ("TE", "HLGAP", "GEOMTRACE", "TEVEC"):
                check(key in props, "%s present in Props/" % key)
            n = re.search(r"CONVERGED AFTER (\d+) ITERATIONS", out)
            check(n is not None, "the stored output says the optimisation converged")
            if n:
                cycles = int(n.group(1))
                geo = numbers(calc, "GEOMTRACE")
                tev = numbers(calc, "TEVEC")
                check(len(geo) == cycles * 15,
                      "GEOMTRACE has %d frames of 5 atoms (%d numbers)" % (cycles, len(geo)))
                check(len(tev) == cycles, "TEVEC has the same length (%d)" % len(tev))
            e = re.findall(r"\| TOTAL ENERGY\s+(\S+) Eh", out)
            te = numbers(calc, "TE")
            check(e and te and abs(te[-1] - float(e[-1])) < 1e-9,
                  "TE %s is the final energy in the stored output %s" % (te[-1:], e[-1:]))

        say("--- water, GFN2-xTB geometry + vibration (--ohess)")
        text, rc = deck(WATER, "GeoVib", "normal")
        check(rc == 0 and "--ohess normal" in text.splitlines()[0],
              "ai.xtb wrote the deck: %r" % text.splitlines()[0])
        calc = runJob(s, "v-%d" % int(time.time()), text, work, rundir,
                      WATER)
        if calc:
            props = s.props("file://" + calc)
            say("  properties: " + " ".join(props))
            for key in ("MULLIKEN", "VIBFREQ", "VIBIR", "VIB", "EZEROPT",
                        "ENTHALPY", "EGIBBS", "ENTROPY", "GEOMTRACE"):
                check(key in props, "%s present in Props/" % key)
            check(len(numbers(calc, "MULLIKEN")) == 3, "3 atomic charges")
            freq = numbers(calc, "VIBFREQ")
            check(len(freq) == 9 and sum(1 for f in freq if f > 1000) == 3,
                  "9 modes, 3 of them vibrations: %r" % freq)
            ir = numbers(calc, "VIBIR")
            check(len(ir) == 9 and max(ir) > 100, "IR intensities: %r" % ir)

        say("--- water, GFN2-xTB vibration at the given geometry (--hess)")
        text, rc = deck(WATER, "Vibration")
        check(rc == 0 and " --hess" in text.splitlines()[0],
              "ai.xtb wrote the deck: %r" % text.splitlines()[0])

        say("--- water, GFN-FF energy: no orbitals, no error")
        text, rc = deck(WATER, "Energy", theory="GFN-FF")
        calc = runJob(s, "f-%d" % int(time.time()), text, work, rundir,
                      WATER)
        if calc:
            props = s.props("file://" + calc)
            check("TE" in props, "TE present in Props/")
            check(os.path.exists(os.path.join(calc, "Props", "GEOMTRACE")),
                  "GEOMTRACE stored for the force-field run too")
            check("MO" not in props and "ORBENG" not in props,
                  "a force-field run stores no orbitals")
    finally:
        left = s.stop()
        if left:
            failures.append("processes left running: %r" % left)

    say("")
    if failures:
        say("FAILED (%d): %s" % (len(failures), "; ".join(failures)))
        return 1
    say("PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
