#!/usr/bin/env python3
"""Builder Tools > Diffraction...: the dialog's CSV equals `ecce-xrd fibre` (#971).

    tests/apps/run_tests.py --check fibre_dialog_test
    tests/apps/fibre_dialog_test.py --keep DIR     keep the Builder's DNA as DIR/dna.xyz

Headless through ECCE_BUILDER_SCRIPT: the Builder's DNA generator makes a
20 bp poly(dA).poly(dT) duplex, the "diffraction" hook opens the dialog on that
structure with its default fields, calculates, and saves the PNG and CSV.  The
CSV must equal the CLI's for the .xyz the dialog was given.
Exit 77 (skip) without an install or Xvfb.
"""
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402

CLI = os.path.join(ROOT, "scripts", "ecce_xrd.py")
failures = []
KEEP = None


def check(ok, what, detail=""):
    print(("ok   " if ok else "FAIL ") + what + ("" if ok else "  " + str(detail)))
    if not ok:
        failures.append(what)
    return ok


def run(display):
    work = os.path.join(fixture.stateHome(), "builder-fibre")
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    png, csv, xyz = (os.path.join(work, n) for n in ("dialog.png", "dialog.csv", "dialog.xyz"))
    script = os.path.join(work, "fibre.script")
    with open(script, "w") as f:
        f.write("\n".join(["wait 3000", "dna " + "A" * 20, "wait 1000", "diffraction %s %s %s" % (png, csv, xyz),
                           "quit"]) + "\n")
    try:
        auth = fixture.authFile(os.path.join(work, "auth.pipe"), user=fixture.realUser())
        result = apps.run(display, "builder", args=("-pipe", auth), windowTimeout=60, settle=240,
                          env={"ECCE_BUILDER_SCRIPT": script, "ECCE_FEEDBACK_STDERR": "1"})
        log = result.log or ""
        if result.crashed:
            check(False, "the Builder ran", "CRASHED (%s)" % result.signalName)
            return
        if not check("BUILDER: diffraction: done" in log and os.path.exists(csv),
                     "the hook calculated and saved", log[-1500:]):
            return
        if KEEP:
            shutil.copy(xyz, os.path.join(KEEP, "dna.xyz"))
        check(open(png, "rb").read(8) == b"\x89PNG\r\n\x1a\n", "the PNG is a PNG")
        ref = os.path.join(work, "cli.csv")
        r = subprocess.run([sys.executable, CLI, "fibre", xyz, "--out", ref],
                           capture_output=True, text=True)
        check(r.returncode == 0, "the CLI ran on the same structure", r.stderr[-300:])
        check(open(csv).read() == open(ref).read(), "the dialog's CSV equals the CLI's")
    finally:
        shutil.rmtree(work, ignore_errors=True)


def main():
    global KEEP
    if "--keep" in sys.argv:
        i = sys.argv.index("--keep")
        KEEP = os.path.abspath(sys.argv[i + 1])
        os.makedirs(KEEP, exist_ok=True)
        del sys.argv[i:i + 2]

    def checkApp(display, name, results, verbose=False):
        run(display)
        if failures:
            results.fail("fibre", "Diffraction dialog wrong: %s" % ", ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "builder"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
