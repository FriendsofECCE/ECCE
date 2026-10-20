#!/usr/bin/env python3
"""A CP2K calculation in the real Calculation Editor (#329), headless.

Local data, private Xvfb, the tree's own binaries.  A CP2K calculation holding
two silicon atoms in a cell is made through the classes the Builder uses
(tests/filedsi/resourceTest cp2k) and opened in the real editor, which is
driven through its script hook (ECCE_CALCED_SCRIPT, the handlers a click
reaches; no synthetic input):

  * the editor offers CP2K's theory and runtypes, has no Basis Set box, and
    starts the two Details dialogs (the real Python dialogs);
  * Cutoff, the functional and dispersion are sent as the dialogs send them
    (a U row is not: the dialog enables its fields itself, a script cannot); Save writes the deck with the real ai.cp2k, and the stored
    deck holds those settings, the cell of the structure, and a basis set
    and potential for silicon found in the library files;
  * a Geometry calculation gets a &MOTION section with the optimizer and
    step limit sent, and Verify (the checker) finds nothing wrong.

    tests/launch/cp2k_calced_test.py [--build build-cmake] [--keep]

Exit status 77 (CTest SKIP) without Xvfb, wxPython, the data directory or the
built binaries.
"""

import argparse
import math
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
sys.path.insert(0, os.path.join(REPO, "tests", "filedsi"))

import harness  # noqa: E402
from harness import say  # noqa: E402

LIBS = ["eccedsi", "eccexml", "eccetdat", "eccedav", "eccefaces",
        "eccecipc", "ecceutil", "eccecomm", "eccercmd"]
failures = []


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def buildResourceTest(build, work):
    driver = os.path.join(work, "resourceTest")
    cmd = (["g++", "-O0", "-w", "-I", os.path.join(REPO, "include"), "-o", driver,
            os.path.join(REPO, "tests", "filedsi", "resourceTest.C"), "-L" + build]
           + ["-l" + l for l in LIBS] * 3
           + ["-lxerces-c", "-lmosquitto", "-lssl", "-lcrypto"])
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0:
        say(proc.stderr[-2000:])
        return None
    return driver


def runEditor(s, display, wrappers, url, script, name):
    path = os.path.join(s.state, name + ".script")
    open(path, "w").write("\n".join(script) + "\n")
    env = s.env({"DISPLAY": display.name, "ECCE_CALCED_SCRIPT": path,
                 "ECCE_FEEDBACK_STDERR": "1",
                 "PATH": wrappers + os.pathsep + s.env()["PATH"]})
    log = os.path.join(s.state, name + ".log")
    with open(log, "w") as handle:
        proc = subprocess.Popen([os.path.join(wrappers, "ecce-calced"), "-context", url],
                                env=env, cwd=os.path.join(s.home, "bin"),
                                stdout=handle, stderr=subprocess.STDOUT,
                                start_new_session=True)
        try:
            proc.wait(timeout=240)
        except subprocess.TimeoutExpired:
            os.killpg(proc.pid, 9)
            proc.wait()
    return open(log, errors="replace").read()


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    if not shutil.which("Xvfb"):
        harness.skip("Xvfb is not installed")
    if not os.path.isfile("/usr/share/cp2k/BASIS_MOLOPT"):
        harness.skip("the CP2K data directory /usr/share/cp2k is missing")
    harness.prerequisites(build, ())
    if not os.access(os.path.join(build, "calced"), os.X_OK):
        harness.skip("calced is not built in %s" % build)
    import xdisplay

    s = harness.Session(build, "cp2kced", {"CP2K": "cp2k.psmp"}, local=True, keep=args.keep)
    harness.link(os.path.join(build, "calced"), os.path.join(s.home, "bin", "calced"))
    gw = os.path.join(REPO, "packaging", "gateway")
    for name in os.listdir(gw):
        if name.startswith("ecce-"):
            harness.link(os.path.join(gw, name), os.path.join(s.home, "bin", name))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)

    display = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        driver = buildResourceTest(build, s.state)
        if not check(driver is not None, "resourceTest built"):
            return 1
        root = os.path.join(s.localData(), "users", "local")
        os.makedirs(root, exist_ok=True)
        proc = subprocess.run([driver, "cp2k", root], env=s.env({"ECCE_HOME": REPO}),
                              capture_output=True, text=True)
        urls = [l for l in proc.stdout.splitlines() if l.startswith("file://")]
        if not check(proc.returncode == 0 and len(urls) == 2, "the CP2K calculations were made"):
            say(proc.stdout)
            return 1
        url, urlGeom = [u.rstrip("/") + "/" for u in urls]
        calc = url[len("file://"):]
        display = xdisplay.Display().__enter__()

        say("--- opened, set up and saved as Energy")
        log = runEditor(s, display, wrappers, url, [
            "wait 3000", "ready", "info",
            "gui ES.Theory.GPW.Cutoff 120", "gui ES.Theory.GPW.RelCutoff 25",
            "gui ES.Theory.GPW.Functional BLYP",
            "gui ES.Theory.GPW.Dispersion DFT-D3 (zero damping)",
            "info", "button save", "wait 6000", "quit"], "energy")
        info = [l for l in log.splitlines() if "CALCED" in l]
        say("\n".join("  | " + l for l in info[:6]))
        check("Theory: " in log or "theory=" in log or info, "the editor opened and answered")
        check(re.search(r"basisbox=hidden", log) is not None, "no Basis Set box for CP2K")
        deck = os.path.join(calc, "Inputs", "cp2k.cp2kin")
        if not check(os.path.exists(deck), "Save wrote Inputs/cp2k.cp2kin"):
            say("\n".join(log.splitlines()[-30:]))
            return 1
        text = open(deck).read()
        check("RUN_TYPE ENERGY_FORCE" in text, "RUN_TYPE ENERGY_FORCE")
        check("CUTOFF 120" in text and "REL_CUTOFF 25" in text, "the cutoffs sent by the dialog")
        check("&BECKE88" in text and "GTH-BLYP-q4" in text, "BLYP with the GTH-BLYP potential for Si")
        check("TYPE DFTD3\n" in text and "REFERENCE_FUNCTIONAL BLYP" in text, "D3 for BLYP")
        check("DFT_PLUS_U" not in text, "no +U without an element typed")
        # The structure's cell is a = b = c = 3.84 A at 60 degrees (primitive
        # Si); the deck's vectors must have those lengths and angles.
        vec = {}
        for k in "ABC":
            m = re.search(k + r" \[angstrom\]\s+(\S+)\s+(\S+)\s+(\S+)", text)
            vec[k] = [float(x) for x in m.groups()] if m else None
        ok = all(vec.values())
        if ok:
            L = {k: sum(x * x for x in v) ** .5 for k, v in vec.items()}
            def ang(u, v):
                return math.degrees(math.acos(sum(a * b for a, b in zip(vec[u], vec[v])) / (L[u] * L[v])))
            ok = (all(abs(l - 3.84) < 1e-3 for l in L.values()) and
                  all(abs(ang(u, v) - 60) < 1e-2 for u, v in ("AB", "BC", "AC")))
        check(ok, "the cell is the structure's lattice (lengths 3.84 A, angles 60 deg): %s" % vec)
        check(len(re.findall(r"^\s+Si\s+-?\d", text, re.M)) == 2, "both Si atoms are in &COORD")
        check("BASIS_SET DZVP-MOLOPT-SR-GTH-q4" in text, "a basis set for silicon was found in the library")
        rc = subprocess.run(["perl", os.path.join(REPO, "scripts", "parsers", "verifyinput"),
                             "-f", deck, "-c", "CP2K", "-a", "2"],
                            capture_output=True, text=True)
        check(rc.returncode == 0 and "BAD|" not in rc.stdout, "Verify finds nothing wrong with the saved deck")
        exe = shutil.which("cp2k.psmp")
        if exe:
            r = subprocess.run([exe, "--check", deck], capture_output=True, text=True, timeout=120)
            check("SUCCESS" in r.stdout, "cp2k --check accepts the deck the editor saved")

        say("--- a Geometry calculation")
        log = runEditor(s, display, wrappers, urlGeom, [
            "wait 3000", "ready", "info", "gui ES.Runtype.GPW.MaxIter 9",
            "gui ES.Runtype.GPW.Optimizer LBFGS", "button save", "wait 6000", "quit"], "geometry")
        deck = os.path.join(urlGeom[len("file://"):], "Inputs", "cp2k.cp2kin")
        if check(os.path.exists(deck), "Save wrote the deck"):
            text = open(deck).read()
            check("RUN_TYPE GEO_OPT" in text and "&MOTION" in text and "MAX_ITER 9" in text
                  and "OPTIMIZER LBFGS" in text,
                  "a Geometry run writes GEO_OPT and &MOTION with the optimizer and step limit")
            r = subprocess.run(["perl", os.path.join(REPO, "scripts", "parsers", "verifyinput"),
                                "-f", deck, "-c", "CP2K", "-a", "2"], capture_output=True, text=True)
            check(r.returncode == 0 and re.search(r"^GOOD\|[1-9]\d*\|run\|", r.stdout, re.M),
                  "Verify marks the run type on its RUN_TYPE line")
        else:
            say("\n".join(log.splitlines()[-30:]))
    finally:
        if display is not None:
            display.__exit__(None, None, None)
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
