#!/usr/bin/env python3
"""Batch Add from SMILES (#961): a table becomes xTB calculations, launched.

Local data.  ecce-batch-smiles reads a CSV of five drug-like molecules (two of
them ions: a protonated amine and a carboxylate) and one unreadable SMILES,
makes one calculation per good row in a project folder with the default
setup (a "QSAR template": xTB GFN2-xTB optimisation), and lists the bad row
with its reason.  Each calculation must carry the charge of its SMILES in its
deck and its measured values in ecce:qsar.measured; all are launched on
localhost through the real Launch and must complete in their own run
directories.  --organizer does the same through the Organizer under Xvfb
(hooks batchsmilesdialog, and batchsmiles ... launch, which is Create and
Launch Batch) and --png FILE keeps the dialog's capture.

    tests/launch/batch_smiles_test.py [--build build-cmake] [--keep]
        [--organizer] [--png FILE]

Exit status 77 (CTest SKIP) without xtb or the built binaries.
"""

import argparse
import json
import os
import re
import shutil
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "apps"))

import harness  # noqa: E402
from harness import say  # noqa: E402

PROJECT_META = os.path.join(REPO, "tools", "screenshots", "data", "project.ecce-meta")

# name, SMILES, IC50 (uM), logP; the expected charge last.
ROWS = [
    ("aspirin", "CC(=O)Oc1ccccc1C(=O)O", "12.5", "1.19", 0),
    ("paracetamol", "CC(=O)Nc1ccc(O)cc1", "3.1", "0.46", 0),
    ("amphetamine-H+", "C[C@H]([NH3+])Cc1ccccc1", "0.8", "1.8", 1),
    ("ibuprofen-anion", "CC(C)Cc1ccc(cc1)C(C)C(=O)[O-]", "45", "3.97", -1),
    ("caffeine", "Cn1cnc2c1c(=O)n(C)c(=O)n2C", "", "-0.07", 0),
]
BAD = ("broken", "C1CC(", "1.0", "1.0")

failures = []


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def calcDir(url):
    path = url.rstrip("/")
    return path[len("file://"):] if path.startswith("file://") else path


def meta(calc):
    path = os.path.join(calc, ".ecce-meta")
    return open(path, errors="replace").read() if os.path.exists(path) else ""


def prop(text, key):
    m = re.search(r"ecce:%s\t\t([^\n]*)" % re.escape(key), text)
    return m.group(1) if m else None


def project(s, name):
    path = os.path.join(calcDir(s.userUrl()), name)
    os.makedirs(path)
    shutil.copy(PROJECT_META, os.path.join(path, ".ecce-meta"))
    return path


def checkMade(folder, urls, withLogP=True):
    """The calculations: charge in the deck, the values, the template."""
    check(os.path.isdir(os.path.join(folder, "QSAR template")), "the QSAR template was made")
    tdeck = os.path.join(folder, "QSAR template", ".ecce-meta")
    check("GFN2-xTB" in open(tdeck, errors="replace").read() if os.path.exists(tdeck) else False,
          "the template is GFN2-xTB")
    for name, smi, ic50, logp, charge in ROWS:
        calc = os.path.join(folder, name)
        m = meta(calc)
        inp = os.path.join(calc, "Inputs", "xtb.xtbin")
        deck = open(inp, errors="replace").read() if os.path.exists(inp) else ""
        check("--chrg %d" % charge in deck and "--opt" in deck and "--gfn 2" in deck,
              "%s: deck has --gfn 2 --chrg %d --opt" % (name, charge))
        check(prop(m, "qsar.smiles") == smi, "%s: the SMILES is stored" % name)
        measured = json.loads(prop(m, "qsar.measured") or "{}")
        want = {"IC50 (uM)": float(ic50) if ic50 else ""}
        if withLogP:
            want["logP"] = float(logp)
        check(measured == want, "%s: measured values %s" % (name, measured))


def launchAll(s, urls):
    dirs, t0 = [], time.time()
    for u in urls:
        rc, out = s.launch(u)
        check(rc == 0, "launch %s" % u.rsplit("/", 1)[-1])
        dirs += [l.split(": ", 1)[1] for l in out.splitlines() if l.startswith("run directory: ")]
    check(len(set(dirs)) == len(urls), "one run directory per calculation")
    waitAll(s, urls, t0)


def waitAll(s, urls, t0):
    for u in urls:
        state = s.waitState(u, seconds=600)
        check(state == "completed", "%s completed (%s)" % (u.rsplit("/", 1)[-1], state))
    say("  all finished %.0f s after the launch" % (time.time() - t0))
    for u in urls:
        out = os.path.join(calcDir(u), "Outputs", "xtb.xtbout")
        text = open(out, errors="replace").read() if os.path.exists(out) else ""
        name = u.rstrip("/").rsplit("/", 1)[-1]
        charge = [r[4] for r in ROWS if r[0] == name][0]
        m = re.search(r":\s+net charge\s+(-?\d+)", text)
        check(m is not None and int(m.group(1)) == charge,
              "%s: xtb ran with charge %d (%s)" % (name, charge, m.group(1) if m else "-"))


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--organizer", action="store_true")
    ap.add_argument("--png", help="keep the dialog's capture (needs --organizer)")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    xtb = shutil.which("xtb")
    if not xtb:
        harness.skip("xtb is not installed")
    harness.prerequisites(build, ())
    if not os.path.exists(os.path.join(build, "ecce-batch-smiles")):
        harness.skip("ecce-batch-smiles is not built")
    if args.organizer and not shutil.which("Xvfb"):
        harness.skip("Xvfb is not installed")
    s = harness.Session(build, "batchsmiles", {"xTB": xtb}, local=True, keep=args.keep)
    work = os.path.join(s.state, "tables")
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    table = os.path.join(work, "drugs.csv")
    with open(table, "w") as h:
        h.write("name,SMILES,IC50 (uM),logP\n")
        for r in ROWS:
            h.write(",".join(r[:4]) + "\n")
        h.write(",".join(BAD) + "\n")
    org = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        stamp = int(time.time())
        say("--- ecce-batch-smiles")
        folder = project(s, "qsar-%d" % stamp)
        #  logP is chosen by hand: only the activity column is guessed.
        rc, out = s.run([os.path.join(s.home, "bin", "ecce-batch-smiles"), "--json",
                         "--response", "IC50 (uM)", "--response", "4", folder, table],
                        timeout=300)
        js = [l for l in out.splitlines() if l.startswith("{")]
        data = json.loads(js[-1]) if js else {}
        say("  rc=%s made=%s failed=%s" % (rc, data.get("made"), data.get("failed")))
        check(rc == 1 and data.get("made") == 5 and data.get("failed") == 1,
              "five made, one not (exit 1)")
        bad = [r for r in data.get("rows", []) if not r["made"]]
        check(len(bad) == 1 and bad[0]["name"] == "broken" and "unclosed" in bad[0]["note"]
              and bad[0]["line"] == 7, "the bad row is listed with line and reason (%s)" % bad)
        got = {r["name"]: r["charge"] for r in data.get("rows", []) if r["made"]}
        check(got == {r[0]: r[4] for r in ROWS}, "charges from the SMILES: %s" % got)
        urls = [r["url"] for r in data.get("rows", []) if r["made"]]
        checkMade(folder, urls)
        rc, out = s.run([os.path.join(s.home, "bin", "ecce-batch-smiles"), folder, table])
        check("a calculation of that name exists" in out, "a second run makes no duplicates")
        rc, out = s.run([os.path.join(s.home, "bin", "ecce-batch-smiles"), "--replace",
                         "--json", "--response", "IC50 (uM)", "--response", "logP",
                         folder, table], timeout=300)
        js = [l for l in out.splitlines() if l.startswith("{")]
        again = json.loads(js[-1]) if js else {}
        check(again.get("made") == 5, "--replace makes the five not launched again")
        urls = [r["url"] for r in again.get("rows", []) if r["made"]] or urls
        launchAll(s, urls)

        if args.organizer:
            from site_screen_test import Organizer
            say("--- Organizer: Tools > Batch Add from SMILES")
            #  Made before the Organizer starts, so that its tree holds it.
            folder2 = project(s, "qsar-gui-%d" % stamp)
            org = Organizer(s, build)
            if not check(org.start(), "the Organizer is up"):
                return 1
            url2 = "file://" + folder2
            shot = args.png or os.path.join(s.state, "batch-smiles-dialog.png")
            got = org.send("batchsmilesdialog %s %s %s" % (url2, table, shot))
            say("  %s" % got)
            check(got.startswith("ok | name=name smiles=SMILES responses=IC50 (uM) "
                                 "rows=6 problems=1"), "the dialog guessed the columns")
            check(os.path.exists(shot), "the dialog was captured")
            menu = org.send("toolsmenu %s" % url2)
            check("item=Batch Add from SMILES..." in menu,
                  "Tools offers Batch Add from SMILES... (%s)" % menu[:200])
            t0 = time.time()
            got = org.send("batchsmiles %s %s launch" % (url2, table), timeout=600)
            say("  %s" % got[:400])
            check(got.startswith("5 calculations made") and "broken (line 7)" in got,
                  "Create made five and listed the bad row")
            urls2 = ["file://" + os.path.join(folder2, r[0]) for r in ROWS]
            checkMade(folder2, urls2, withLogP=False)
            check("launch:" in got, "Launch Batch ran")
            waitAll(s, urls2, t0)
    finally:
        if org:
            org.close()
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
