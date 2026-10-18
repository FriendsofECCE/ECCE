#!/usr/bin/env python3
"""CP2K through the real Launch (#329): bulk silicon and a NiO+U optimisation.

Local data, no data server.  For each system the deck is made by the real
ai.cp2k (the generator test's decks), stored with a new calculation, and run
on `localhost` through Launch, gensub (sub cp2k: mpirun -np 4 cp2k.psmp),
eccejobmaster, eccejobstore and eccejobmonitor.  Each must reach "completed"
with what the Organizer and Builder read stored in the calculation:

  * silicon, ENERGY_FORCE: TE, EWVEC, DELTAE, MULLIKEN, HIRSHFELD, EGRADVEC,
    ETIME, with TE equal to the energy in the stored output;
  * NiO, UKS multiplicity 3, U on Ni, GEO_OPT stopped after 3 steps:
    GEOMTRACE with 3 frames, TEVEC of the same length, SPINMOM adding up to
    the total spin the output prints, and the progress messages the job store
    relays (#284) numbered in order, ending with the optimisation stage.

    tests/launch/cp2k_test.py [--build build-cmake] [--keep] [--no-nio]

Exit status 77 (CTest SKIP) without cp2k.psmp, the CP2K data directory or the
built binaries.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "apps"))

import harness  # noqa: E402
from harness import say  # noqa: E402
import importlib.util  # noqa: E402

#  The generator test's decks; its module is also called run_tests.
_spec = importlib.util.spec_from_file_location(
    "cp2k_features", os.path.join(REPO, "tests", "cp2k_features", "run_tests.py"))
feat = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(feat)

failures = []


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def calcDir(url):
    path = url.rstrip("/")
    return path[len("file://"):] if path.startswith("file://") else path


def propertyText(calc, key):
    with open(os.path.join(calc, "Props", key), errors="replace") as handle:
        return handle.read()


def numbers(calc, key):
    """The numbers of a property file, in order (after the XML header)."""
    body = propertyText(calc, key).split(">", 2)[2]
    return [float(v) for v in re.findall(r"[-+]?\d+\.\d+(?:[eE][-+]?\d+)?", body)]


def startListener(s, name):
    path = os.path.join(s.state, "progress-%s.tsv" % name)
    proc = subprocess.Popen(
        [os.path.join(s.home, "bin", "launchjob"), "-pipe", os.devnull,
         "listen", "ecce_url_progress", "1500", path],
        env=s.env(), cwd=os.path.join(s.home, "bin"),
        stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
    for _ in range(50):
        if os.path.exists(path) and "# listening" in open(path).read():
            break
        time.sleep(0.2)
    return proc, path


def progressMessages(url, proc, path):
    time.sleep(3)
    proc.terminate()
    proc.wait(timeout=30)
    msgs = []
    for line in open(path).read().splitlines():
        if line.startswith("#"):
            continue
        at, murl, text = line.split("\t", 2)
        if murl.rstrip("/") == url.rstrip("/") and text:
            msgs.append(text.replace("\\n", "\n"))
    return msgs


def runJob(s, name, deckText, work, rundir, listen=False):
    deck = os.path.join(work, name + ".cp2kin")
    open(deck, "w").write(deckText)
    url, out = s.create(name, "cp2k_es", deck, "cp2k.cp2kin", rundir, extra=("procs=4",))
    if not check(url is not None, "%s created" % name):
        say(out)
        return None, None
    listener = startListener(s, name) if listen else None
    rc, out = s.launch(url)
    say("\n".join("  | " + l for l in out.strip().splitlines()[-6:]))
    if not check(rc == 0, "%s: Launch ran to the end" % name):
        return None, None
    state = s.waitState(url, seconds=1200)
    if not check(state == "completed", "%s reached completed (last: %s)" % (name, state)):
        rc, why = s.driver("reason", url)
        say(why)
        return None, None
    time.sleep(2)
    msgs = progressMessages(url, *listener) if listener else []
    return calcDir(url), msgs


def storedOutput(calc):
    path = os.path.join(calc, "Outputs", "cp2k.cp2kout")
    check(os.path.exists(path), "the output was stored as Outputs/cp2k.cp2kout")
    return open(path, errors="replace").read() if os.path.exists(path) else ""


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--no-nio", action="store_true", help="silicon only")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    cp2k = shutil.which("cp2k.psmp")
    if not cp2k:
        harness.skip("cp2k.psmp is not installed")
    if not shutil.which("mpirun"):
        harness.skip("mpirun is not installed")
    if not os.path.isfile(os.path.join(feat.LIB, "BASIS_MOLOPT")):
        harness.skip("the CP2K data directory %s is missing" % feat.LIB)
    harness.prerequisites(build, ())

    s = harness.Session(build, "cp2k", {"CP2K": cp2k}, local=True, keep=args.keep)
    work = os.path.join(s.state, "decks")
    rundir = os.path.join(s.state, "run")
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    os.makedirs(rundir, exist_ok=True)
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1

        say("--- bulk silicon, energy and forces")
        text, err, rc = feat.deck(feat.SI, "Energy", feat.CHEAP)
        check(rc == 0, "ai.cp2k wrote the deck")
        calc, _ = runJob(s, "si-%d" % int(time.time()), text, work, rundir)
        if calc:
            out = storedOutput(calc)
            props = s.props("file://" + calc)
            say("  properties: " + " ".join(props))
            for key in ("TE", "EWVEC", "DELTAE", "MULLIKEN", "HIRSHFELD", "EGRADVEC", "ETIME"):
                check(key in props, "%s present in Props/" % key)
            check("GEOMTRACE" not in props, "no geometry trace for an energy")
            e = re.findall(r"ENERGY\| Total FORCE_EVAL \( QS \) energy \[hartree\]\s+(\S+)", out)
            te = numbers(calc, "TE")
            check(e and te and abs(te[-1] - float(e[-1])) < 1e-9,
                  "TE %s is the energy in the stored output %s" % (te[-1:], e[-1:]))
            steps = re.findall(r"^\s+\d+\s+OT DIIS\s+\S+\s+\S+\s+\S+\s+(-\d+\.\d+)\s+\S+\s*$", out, re.M)
            check(len(numbers(calc, "EWVEC")) == len(steps) > 3,
                  "EWVEC has the %d SCF iterations of the output" % len(steps))
            check(len(numbers(calc, "MULLIKEN")) == 2, "MULLIKEN has a charge per atom")

        if not args.no_nio:
            say("--- NiO with U on Ni, UKS, three optimisation steps")
            text, err, rc = feat.deck(feat.NIO, "Geometry", feat.NIO_SPIN + ["ES.Runtype.GPW.MaxIter: 3"])
            check(rc == 0, "ai.cp2k wrote the deck")
            calc, msgs = runJob(s, "nio-%d" % int(time.time()), text, work, rundir, listen=True)
            if calc:
                out = storedOutput(calc)
                props = s.props("file://" + calc)
                say("  properties: " + " ".join(props))
                for key in ("TE", "GEOMTRACE", "TEVEC", "EGRADVEC", "MULLIKEN", "SPINMOM", "HIRSHFELD"):
                    check(key in props, "%s present in Props/" % key)
                check("DFT+U energy" in out, "the stored output has a DFT+U energy")
                frames = re.findall(r"^\s+i =\s+(\d+), E =\s+(\S+)\s*$", out, re.M)
                check(len(frames) == 3, "the output has 3 trajectory frames")
                tev = numbers(calc, "TEVEC")
                check(len(tev) == 3 and all(abs(a - float(b[1])) < 1e-9 for a, b in zip(tev, frames)),
                      "TEVEC %s equals the frame energies" % tev)
                geo = numbers(calc, "GEOMTRACE")
                check(len(geo) == 3 * 2 * 3, "GEOMTRACE has 3 frames of 2 atoms (%d numbers)" % len(geo))
                spin = numbers(calc, "SPINMOM")
                tot = re.findall(r"# Total charge and spin\s+\S+\s+\S+\s+\S+\s+(\S+)", out)
                check(spin and tot and abs(sum(spin) - float(tot[-1])) < 1e-6 and abs(float(tot[-1]) - 2) < 1e-3,
                      "SPINMOM %s adds up to the total spin %s" % (spin, tot[-1:]))
                say("  progress messages: %d" % len(msgs))
                if check(len(msgs) >= 3, "the Organizer's topic carried progress (%d messages)" % len(msgs)):
                    heads = [dict(w.split("=", 1) for w in m.splitlines()[0][2:].split()) for m in msgs]
                    seqs = [int(h["seq"]) for h in heads]
                    check(seqs == sorted(seqs) and len(set(seqs)) == len(seqs),
                          "numbered in order: %s" % seqs)
                    last = msgs[-1]
                    check(re.search(r"^opt step=3 .*converged=no", last, re.M) is not None,
                          "the last snapshot has the optimisation at step 3, not converged")
                    check(re.search(r"^scf cycle=\d+ scferr=\S+/\S+ .*converged=yes", last, re.M) is not None,
                          "and the SCF with its tolerance and a converged mark")
                    check(re.search(r"^opt~ energy=", last, re.M) is not None,
                          "and the energy history of the optimisation")
                    steps = [int(m.group(1)) for t in msgs for m in
                             [re.search(r"^opt step=(\d+)", t, re.M)] if m]
                    check(steps == sorted(steps), "optimisation steps never go back: %s" % steps)
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
