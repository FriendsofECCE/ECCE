#!/usr/bin/env python3
"""
Gaussian 16 Opt=QST2/QST3 through the real Calculation Editor (#419).

A local project holds three Gaussian-16 calculations built from the end
points of tests/parsers/fixtures/gaussian-16/hcn_irc.log: "react" (HCN),
"prod" (HNC written H, N, C) and "ts" (the TS calculation, C, N, H).  The
editor opens "ts", its Runtype Details dialog is started (ECCE_CALCED_SCRIPT
"details runtype"), and the script checks the candidate list handed to the
dialog, picks Transition State (QST), reactant and product through the
message the dialog sends, saves, then ticks the QST3 guess and saves again.
Each save's kept generator directory is read: the .qstN.frag files CalcEd
writes, the deck's Opt=(QST2|QST3), block order and atom order, and the
status line shown to the user.

Same installed tree, isolation and services as run_tests.py.  Exit 77
(skip) without an install or Xvfb.
"""

import getpass
import os
import re
import shutil
import subprocess
import sys
import threading
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402
import symmetry_find_test as sym  # noqa: E402

LOG = os.path.join(REPO, "tests", "parsers", "fixtures", "gaussian-16", "hcn_irc.log")
SYM = {6: "C", 7: "N", 1: "H"}
KEY = "ES.Runtype.GeomOpt."


def points(stop_at_ts=False):
    lines = open(LOG).read().splitlines()
    if stop_at_ts:
        lines = lines[:[i for i, l in enumerate(lines) if "Stationary point found" in l][0]]
    pts, i = [], 0
    while i < len(lines):
        if "Input orientation" in lines[i]:
            j, atoms = i + 5, []
            while "-----" not in lines[j]:
                f = lines[j].split()
                atoms.append((SYM[int(f[1])], [float(x) for x in f[3:6]]))
                j += 1
            if [a[0] for a in atoms] == ["C", "N", "H"]:
                pts.append(atoms)
            i = j
        i += 1
    return pts


def dist(a, b):
    return sum((x - y) ** 2 for x, y in zip(a, b)) ** 0.5


def writeXyz(path, atoms):
    with open(path, "w") as f:
        f.write("%d\nx\n" % len(atoms))
        for e, c in atoms:
            f.write("%s %.8f %.8f %.8f\n" % ((e,) + tuple(c)))


def makeProject(folder):
    """proj-qst with react, prod, ts: ({name: url}, None) or (None, error)."""
    build = os.environ.get("ECCE_TEST_BUILD", os.path.join(REPO, "build-cmake"))
    xyz = os.path.join(fixture.stateDir(), "qst-xyz")
    shutil.rmtree(xyz, ignore_errors=True)
    os.makedirs(xyz)
    pts = points()
    hcn = min(pts, key=lambda a: dist(a[0][1], a[2][1]))
    hnc = min(pts, key=lambda a: dist(a[1][1], a[2][1]))
    writeXyz(os.path.join(xyz, "react.xyz"), hcn)
    writeXyz(os.path.join(xyz, "prod.xyz"), [hnc[2], hnc[1], hnc[0]])
    writeXyz(os.path.join(xyz, "ts.xyz"), points(True)[-1])
    driver = os.path.join(fixture.stateDir(), "resourceTest")
    libs = ["eccedsi", "eccexml", "eccetdat", "eccedav", "eccefaces",
            "eccecipc", "ecceutil", "eccecomm", "eccercmd"]
    built = subprocess.run(
        ["g++", "-O0", "-w", "-I", os.path.join(REPO, "include"), "-o", driver,
         os.path.join(REPO, "tests", "filedsi", "resourceTest.C"), "-L" + build]
        + ["-l" + l for l in libs] * 3
        + ["-lxerces-c", "-lmosquitto", "-lssl", "-lcrypto"],
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if built.returncode != 0:
        return None, built.stdout.decode()[-1500:]
    shutil.rmtree(folder, ignore_errors=True)
    user = os.path.join(folder, "users", "local")
    os.makedirs(user)
    os.environ["ECCE_EXPERIMENTAL_TS"] = "1"   # QST is offered only then (#419)
    env = dict(os.environ, ECCE_HOME=apps.INSTALL, ECCE_LOCAL_DATA=folder,
               ECCE_REALUSER=getpass.getuser(), ECCE_NO_MESSAGING="1",
               LD_LIBRARY_PATH=build)
    done = subprocess.run(
        [driver, "qst", user] + ["%s=%s" % (n, os.path.join(xyz, n + ".xyz"))
                                 for n in ("react", "prod", "ts")],
        env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    out = done.stdout.decode()
    urls = re.findall(r"^(file://\S+)$", out, re.M)
    if done.returncode != 0 or len(urls) != 3:
        return None, out[-1500:]
    return dict(zip(("react", "prod", "ts"), urls)), None


def atomsOf(path):
    lines = open(path).read().splitlines()
    k = lines.index("atom_list:")
    return [l.split() for l in lines[k + 1:] if l.strip()]


def photographer(display, scriptDir, outDir, names):
    """For each "shot NAME" step of the script: grab every window of the
    display to <outDir>/NAME-<title>.png, then release the script."""
    for name in names:
        ready = os.path.join(scriptDir, name + ".ready")
        end = time.time() + 150
        while not os.path.exists(ready) and time.time() < end:
            time.sleep(0.3)
        wins = display.windows()
        print("shot %s: windows %s" % (name, [t for _, t in wins]))
        subprocess.run(["import", "-display", display.name, "-window", "root",
                        os.path.join(outDir, "%s-screen.png" % name)],
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        tree = subprocess.run(["xwininfo", "-display", display.name, "-root",
                               "-tree"], capture_output=True, text=True).stdout
        open(os.path.join(outDir, name + ".tree.txt"), "w").write(tree)
        for wid, title in wins:
            slug = re.sub(r"\W+", "_", title).strip("_") or "window"
            subprocess.run(["import", "-display", display.name, "-window", wid,
                            os.path.join(outDir, "%s-%s.png" % (name, slug))],
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        open(os.path.join(scriptDir, name + ".go"), "w").close()


def main():
    def checkApp(display, name, results, verbose=False):
        folder = os.path.join(fixture.stateDir(), "qst-localdata")
        urls, err = makeProject(folder)
        if urls is None:
            results.fail("calced-qst", "could not build the project: %s" % err)
            return
        os.environ["ECCE_LOCAL_DATA"] = folder
        restorePrefs = fixture.settleUpgradeNotices()
        try:
            steps = ["wait 9000", "ready", "runtype Transition State (QST)", "wait 3000", "ready",
                     "basis 6-31G", "wait 1500",
                     "details runtype", "wait 8000", "qst-candidates",
                     "guiw %sQSTReactant %s" % (KEY, urls["react"]),
                     "guiw %sQSTProduct %s" % (KEY, urls["prod"]),
                     "guivalue %sQSTReactant" % KEY,
                     "button save-keep", "wait 6000"]
            shots = os.environ.get("ECCE_QST_SHOTS")
            if shots:
                # the editor after Save, then the dialog reopened with the picks
                steps += ["shot qst2-editor", "close-details", "details runtype",
                          "wait 7000", "shot qst2-dialog"]
            steps += ["guiw %sQSTGuess 1" % KEY, "button save-keep", "wait 6000"]
            if shots:
                steps += ["shot qst3-editor", "close-details", "details runtype",
                          "wait 7000", "shot qst3-dialog"]
            steps += ["quit"]
            if shots:
                os.makedirs(shots, exist_ok=True)
                threading.Thread(target=photographer, daemon=True, args=(
                    display, fixture.stateDir(), shots,
                    ["qst2-editor", "qst2-dialog", "qst3-editor",
                     "qst3-dialog"])).start()
            log = sym.scripted(display, "calced", "ECCE_CALCED_SCRIPT", steps,
                               urls["ts"] + "/")
            tail = run_tests._tail(log, 40)
            cands = re.findall(r"CALCED: qst-candidate (\S+)\t(\S+)\t(\S+)", log)
            sym.check(sorted(c[0] for c in cands) == ["prod", "react"],
                      "the dialog is offered prod and react, not ts (%s)"
                      % [c[0] for c in cands], tail)
            dirs = re.findall(r"generator files kept in (\S+)", log)
            sym.check(len(dirs) == 2, "two saves kept their files (%s)" % dirs, tail)
            status = re.findall(r"(Calculation saved as [^\n]*)", log)
            print("status lines:", status)
            sym.check(len(status) == 2 and all("Matched" in s for s in status),
                      "the one-line status reports the atom matching (Matched N atoms; moved: ...; RMSD)", tail)
            if len(dirs) == 2:
                for mode, d, want in (("QST2", dirs[0], [1, 2]),
                                      ("QST3", dirs[1], [1, 2, 3])):
                    have = sorted(int(m.group(1)) for m in
                                  (re.match(r"ts\.qst(\d)\.frag$", f)
                                   for f in os.listdir(d)) if m)
                    sym.check(have == want, "%s writes qst%s.frag (got %s)"
                              % (mode, want, have), tail)
                    for k in have:
                        order = [a[0] for a in atomsOf(os.path.join(d, "ts.qst%d.frag" % k))]
                        sym.check(order == ["C", "N", "H"],
                                  "%s: qst%d.frag atoms C, N, H (product "
                                  "renumbered) (%s)" % (mode, k, order), tail)
                    text = ""
                    for f in os.listdir(d):
                        t = open(os.path.join(d, f), errors="replace").read()
                        if f == "g16.g16in":
                            text = t
                            print("----- %s deck (%s)\n%s" % (mode, f, t))
                    sym.check(("Opt=(%s" % mode) in text,
                              "%s: the deck says Opt=(%s" % (mode, mode), tail)
                    blocks = re.findall(r"^0 1[^\n]*\n((?: *[A-Z][a-z]? .*\n)+)", text, re.M)
                    sym.check(len(blocks) == len(want),
                              "%s: %d structure blocks (got %d)"
                              % (mode, len(want), len(blocks)), tail)
                    for b in blocks:
                        sym.check([l.split()[0] for l in b.splitlines()] == ["C", "N", "H"],
                                  "%s: block atom order C, N, H" % mode, tail)
        finally:
            restorePrefs()
            os.environ.pop("ECCE_LOCAL_DATA", None)
        if sym.failures:
            results.fail("calced-qst", "%d wrong: %s"
                         % (len(sym.failures), "; ".join(sym.failures)))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "calced"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
