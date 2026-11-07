#!/usr/bin/env python3
"""Fit Morse and Compare in the Relaxed Scan panel (#858), headless.

    tests/apps/morse_panel_test.py [--build build-cmake] [--png DIR] [--keep]

The stored copies of the H2 examples (General Chemistry h2_bond_curve,
Computational h2_*_scan) are put in one project.  The Builder opens the
Relaxed Scan panel of each; "scanmorse on [window]" must log the numbers and
the text of `ecce-morse <calc> --json [--window]` for the same folder, and
"scancompare" must list the scans beside it and draw the chosen ones (each
relative to its own lowest point) next to the fitted curve.  PNGs of the plot
(white) go to --png.  Exit status 77 (skip) without Xvfb, import, binaries or
the stored examples.
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))

import builder_overlay_test as bo  # noqa: E402
import harness  # noqa: E402
from harness import say  # noqa: E402

check = bo.check
EX = os.path.join(REPO, "data", "client", "examples")
STORED = {
    "h2_bond_curve": os.path.join(EX, "genchem", "bond-formation", "orca-h2_bond_curve"),
}
STORED.update({n: os.path.join(EX, "computational", "h2-dissociation", "orca-" + n)
               for n in ("h2_rhf_scan", "h2_uhf_scan", "h2_ccsd_scan", "h2_uhf_scan_out")})
FIELDS = {"re": "re_A", "De_eV": "De_eV", "De_kJ": "De_kJ_per_mol", "a": "a_per_A",
          "omega": "omega_e_cm-1", "omegaX": "omega_e_xe_cm-1", "D0_eV": "D0_eV",
          "D0_kJ": "D0_kJ_per_mol", "rms": "rms_kJ_per_mol", "used": "points_used",
          "total": "points_total", "window": "window_kJ_per_mol"}


def cli(calc, window=None):
    cmd = [sys.executable, os.path.join(REPO, "scripts", "ecce_morse.py"), calc, "--json"]
    if window:
        cmd += ["--window", str(window)]
    r = subprocess.run(cmd, capture_output=True, text=True)
    return json.loads(r.stdout) if r.returncode == 0 else None


def morseLines(text):
    states = re.findall(r"^BUILDER: scanmorse (.*)$", text, re.M)
    texts = re.findall(r"^BUILDER: scanmorsetext \[(.*)\]$", text, re.M)
    return states, texts


def same(state, ref, what):
    got = dict(kv.split("=", 1) for kv in state.split()[1:] if "=" in kv)
    bad = []
    for k, j in FIELDS.items():
        a, b = float(got.get(k, "nan")), float(ref[j])
        if not abs(a - b) <= 1e-8 * max(1.0, abs(b)):
            bad.append("%s %s != %s" % (k, a, b))
    check(state.startswith("ok ") and not bad, "%s: panel fit equals ecce-morse --json %s"
          % (what, "; ".join(bad)))


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--png")
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    for tool in ("Xvfb", "import"):
        if not shutil.which(tool, path=os.environ.get("PATH", "") + os.pathsep + "/usr/sbin"):
            harness.skip("%s is not installed" % tool)
    harness.prerequisites(build, ())
    for exe in bo.GUI:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    stored = {}
    for name, folder in STORED.items():
        src = os.path.join(folder, "stored", name)
        if not os.path.isdir(src):
            harness.skip("no stored copy %s (tools/examples/preparse.py)" % src)
        stored[name] = src
    import xdisplay

    s = harness.Session(build, "morsepanel", {}, local=True, keep=args.keep)
    png = args.png or os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
    for exe in bo.GUI:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    harness.link(os.path.join(REPO, "scripts", "ecce_morse.py"),
                 os.path.join(s.home, "bin", "ecce-morse"))
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
        xdisplay.SCREEN = "1500x950x24"
        display = xdisplay.Display().__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        project = os.path.join(s.localData(), "users", "local", "h2")
        os.makedirs(project, exist_ok=True)
        shutil.copy(bo.PROJECT_META, os.path.join(project, ".ecce-meta"))
        calcs = {}
        for name, src in stored.items():
            calcs[name] = os.path.join(project, name)
            shutil.rmtree(calcs[name], ignore_errors=True)
            shutil.copytree(src, calcs[name], symlinks=True)
        extra = {"ECCE_PANEL_MODE": "detail", "ECCE_OPEN_PANEL": "Relaxed Scan"}

        # The General Chemistry curve: the fit, then a 150 kJ/mol window.
        bond = calcs["h2_bond_curve"]
        lines = ["wait 6000", "scanmorse on", "scanpng %s" % os.path.join(png, "h2-bond-morse.png"),
                 "scanmorse on 150", "scanmorse off", "quit"]
        text, _ = bo.runScript(s, display, env, wrappers, ["-context", "file://" + bond + "/"],
                               lines, png, "bondcurve", extra)
        states, texts = morseLines(text)
        ref, ref150 = cli(bond), cli(bond, 150)
        check(ref is not None and ref150 is not None, "ecce-morse runs on the stored calculation")
        if len(states) != 3 or ref is None or ref150 is None:
            say(text[-2500:])
            check(False, "three scanmorse answers (%d)" % len(states))
            return 1
        same(states[0], ref, "all bound points")
        check(texts[0] == ref["report"].replace("\n", " | "),
              "the text box holds ecce-morse's report")
        same(states[1], ref150, "window 150 kJ/mol")
        check(ref150["points_used"] < ref["points_used"], "the window drops points")
        check(states[2] == "off" and texts[2] == "", "Fit Morse off hides the text")
        check("scanpng saved" in text, "the plot PNG was written")
        check(0.70 < ref["re_A"] < 0.78, "re of the H2 curve near 0.74 A (%.4f)" % ref["re_A"])
        check("Å⁻¹" in texts[0] and "cm⁻¹" in texts[0] and "e-0" not in texts[0],
              "units with superscripts, no e-notation")

        # The Computational comparison: three curves and the CCSD fit.
        ccsd = calcs["h2_ccsd_scan"]
        lines = ["wait 6000", "scancompare", "scancompare h2_rhf_scan h2_uhf_scan",
                 "scanmorse on", "scanpng %s" % os.path.join(png, "h2-three-curves-morse.png"),
                 "shot panel", "scancompare nosuch", "quit"]
        text, shots = bo.runScript(s, display, env, wrappers, ["-context", "file://" + ccsd + "/"],
                                   lines, png, "compare", extra)
        cands = re.findall(r"^BUILDER: scancompare candidates:(.*)$", text, re.M)
        want = sorted(n for n in calcs if n != "h2_ccsd_scan")
        check(len(cands) >= 2 and sorted(cands[0].split()) == want,
              "Compare lists the other scans of the folder (%s)" % (cands[:1],))
        states, _texts = morseLines(text)
        r2 = cli(ccsd)
        check(bool(states) and r2 is not None, "fit beside the compared curves")
        if states and r2:
            same(states[0], r2, "CCSD scan with two compared scans")
        check("No scan named nosuch" in text, "an unknown name is refused")
        check("segmentation" not in text.lower(), "no crash in the Builder log")
        say("PNGs: %s" % ", ".join(sorted(os.listdir(png))))
    finally:
        if display:
            display.__exit__(None, None, None)
        s.stop()
    failures = bo.failures
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
