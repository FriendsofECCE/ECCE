#!/usr/bin/env python3
"""The fragment-orbital interaction diagram of the Details window (#201), headless.

The Organizer hook "fragdiagram" opens the Fragment orbitals Details window on
the Cr(CO)6 fixtures of tests/fragorb_h5 (Cr ROHF septet | (CO)6, ORCA
RHF/def2-SVP) and prints the Diagram page's layout.  Checked:
- every level and line the GUI draws is one of `ecce-fragorb diagram --json`
  for the same files and window, and every one of those is drawn (ids,
  columns, fragments, MO/FO numbers, energies, one bar per orbital, shares);
- no label touches a line, a bar or another label, or leaves the picture
  (profile_layout.py), on screen and in the exported PNG;
- the verdict line is the CLI's, and a click on a level shows its composition.
ECCE_FRAGORB_SHOTS=<dir> keeps the PNGs.  Exit 77 (skip) without an install
or Xvfb.
"""
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import apps            # noqa: E402
import isolate         # noqa: E402
import profile_layout  # noqa: E402
import xdisplay        # noqa: E402

FIXTURES = os.path.join(ROOT, "tests", "fragorb_h5", "fixtures")
NAMES = ["Cr", "C₆O₆"]
CASES = [("default", None), ("narrow-window", (1, 1)), ("wide-window", (8, 8))]
failures = []


def check(what, cond, why=""):
    print(("ok    " if cond else "FAIL  ") + what + ("" if cond else "\n      " + str(why)[:1500]))
    if not cond:
        failures.append(what)
    return cond


def cli(args):
    env = dict(os.environ, ECCE_HOME=apps.INSTALL)
    r = subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-fragorb")] + args + ["--json"],
                       env=env, capture_output=True, text=True)
    return json.loads(r.stdout) if r.returncode == 0 else None


def drawn(lines, prefix=""):
    """levels {id: (column, fragment, energy, numbers, bars)}, lines {(fo, mo): share}"""
    levels, conn = {}, {}
    for l in lines:
        if prefix:
            if not l.startswith(prefix):
                continue
            l = l[len(prefix):]
        f = l.split()
        if f and f[0] == "level":
            levels[int(f[1])] = (int(f[2]), int(f[3]), f[4], f[5], f[6])
        elif f and f[0] == "line":
            conn[(int(f[1]), int(f[2]))] = f[3]
    return levels, conn


def expected(ref):
    ch = ref["channels"][0]
    levels = {l["id"]: (l["column"], l["fragment"], "%.6f" % l["energy_ev"],
                        ",".join(str(n) for n in l["numbers"]), "bars=%d" % len(l["numbers"]))
              for l in ch["levels"]}
    conn = {(x["fo"], x["mo"]): "%.6f" % x["share"] for x in ch["lines"]}
    return levels, conn


def run(display, work, shots):
    folders = [os.path.join(FIXTURES, n) for n in ("crco6", "cr7", "co6")]
    cmds = []
    for name, window in CASES:
        png = os.path.join(shots, "fragorb-diagram-%s.png" % name)
        extra = " window=%d,%d" % window if window else ""
        shot = " shot=" + os.path.join(shots, "fragorb-details-%s.png" % name) if name == "default" else ""
        dump = os.path.join(work, "dump-%s.txt" % name)
        cmds.append("fragdiagram %s %s names=%s%s click=0%s dump=%s" % (png, " ".join(folders), ";".join(NAMES),
                                                                       extra, shot, dump))
    script = os.path.join(work, "commands")
    with open(script, "w") as f:
        f.write("\n".join(cmds) + "\n")
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"), exist_ok=True)
    result = apps.run(display, "organizer", windowTimeout=60, settle=60,
                      env={"ECCE_TEST_ORGANIZER": script, "ECCE_LOCAL_DATA": data})
    log = result.log or ""
    with open(os.path.join(work, "organizer.log"), "w") as f:
        f.write(log)
    if result.crashed:
        check("the Organizer ran", False, "CRASHED (%s)\n%s" % (result.signalName, log[-1500:]))
        return
    got = re.findall(r"ECCE_TEST_ORGANIZER: fragdiagram .*?: (ok.*|error.*)$", log, re.M)
    if not check("every case answered", len(got) == len(CASES), log[-2000:]):
        return
    for (name, window), answer in zip(CASES, got):
        if not check("%s: report written" % name, "dump saved" in answer, answer[:300]):
            continue
        report = open(os.path.join(work, "dump-%s.txt" % name), encoding="utf-8").read()
        lines = report.split("\n")
        args = ["diagram"] + folders + ["--name", NAMES[0], "--name", NAMES[1]]
        if window:
            args += ["--window", "%d,%d" % window]
        ref = cli(args)
        if not check("%s: ecce-fragorb diagram runs" % name, ref is not None):
            continue
        want_levels, want_lines = expected(ref)
        for where, prefix in (("screen", ""), ("export", "export ")):
            lv, ln = drawn(lines, prefix)
            check("%s (%s): the levels drawn are the CLI's" % (name, where), lv == want_levels,
                  "GUI %s\nCLI %s" % (sorted(lv.items()), sorted(want_levels.items())))
            check("%s (%s): the lines drawn are the CLI's" % (name, where), ln == want_lines,
                  "GUI %s\nCLI %s" % (sorted(ln.items()), sorted(want_lines.items())))
            lay = ["layout %s %s" % (where, l[len(prefix):]) for l in lines
                   if l.startswith(prefix) and re.match(r"(size|text|seg|unplaced)\b", l[len(prefix):])]
            bad = profile_layout.overlaps(lay)
            check("%s (%s): no label touches a line, a bar or another label" % (name, where), not bad,
                  "; ".join(bad[:10]))
            labels = [l for l in lay if " label " in l]
            check("%s (%s): every level has a label" % (name, where), len(labels) == len(want_levels),
                  "%d labels, %d levels" % (len(labels), len(want_levels)))
        verdict = ref["channels"][0]["verdict"]
        shown = " ".join(l.split(": ", 1)[1] for l in lines if re.match(r"text .* verdict: ", l))
        check("%s: the verdict line is the CLI's" % name, shown == verdict, "%r vs %r" % (shown, verdict))
        gap = [l for l in lines if re.match(r"text .* gap: ", l)]
        check("%s: the HOMO-LUMO gap is given as an orbital-energy gap" % name,
              gap and "orbital-energy gap %.2f eV" % ref["channels"][0]["gap_ev"] in gap[0], gap)
        click = report.split("\nclick 0\n", 1)[1] if "\nclick 0\n" in report else ""
        l0 = ref["channels"][0]["levels"][0]
        check("%s: a click on level 0 shows its composition" % name,
              click.startswith("MO %d" % l0["numbers"][0]) and "per electron" in click and
              "1 % or more" in click, click[:400])
        check("%s: PNG written" % name, "png saved" in lines and
              os.path.getsize(os.path.join(shots, "fragorb-diagram-%s.png" % name)) > 10000)
    check("window picture taken", "shot saved" in got[0], got[0][-100:])


def main():
    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-organizer")):
        print("SKIP: no ecce-organizer under %s" % apps.WRAPPERS)
        return 77
    if not shutil.which("Xvfb"):
        print("SKIP: no Xvfb")
        return 77
    try:
        settings = isolate.apply(apps.INSTALL)
    except isolate.IsolationError as exc:
        print("refusing to run: %s" % exc)
        return 1
    print(isolate.describe(settings))
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    work = tempfile.mkdtemp(prefix="fragorb-diagram-", dir=os.environ.get("ECCE_REALUSERHOME"))
    shots = os.environ.get("ECCE_FRAGORB_SHOTS") or os.path.join(work, "shots")
    os.makedirs(shots, exist_ok=True)
    with xdisplay.Display() as display:
        gateway = os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")
        subprocess.run([gateway], env=display.env(), timeout=180)
        try:
            run(display, work, shots)
        finally:
            subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-stop")],
                           env=display.env(), timeout=120)
    if not failures:
        shutil.rmtree(work, ignore_errors=True)
    else:
        print("kept " + work)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
