#!/usr/bin/env python3
"""Vibrations panel > Match bands (#707): the table equals ecce-bandshift.

    tests/apps/nmode_bandshift_test.py

A Builder opens the Gaussian 16 water frequency calculation; a copy of it
with frequencies scaled by 0.98 sits in the same folder.  The scene command
"vibbandshift" is the Match bands action; the rows it prints from the
table must equal `ecce-bandshift --json` on the two stored calculations
(mode numbers exactly, numbers to the table's one decimal), and
"vibbandclick" on a row must select that mode in the panel.
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

import apps       # noqa: E402
import fixture    # noqa: E402

BASE = os.path.join(HERE, "fixtures", "calc-water-vib")
NAME = "nmode-band-water"
OTHER = NAME + "-scaled"
CLI = os.path.join(ROOT, "scripts", "ecce_bandshift.py")
MARKERS = ("ended by SIG", "Segmentation fault", "double free or corruption",
           "terminate called", "Unhandled standard exception")
fails = []


def check(name, ok, detail=""):
    print(("PASS  " if ok else "FAIL  ") + name + ("" if ok else "  " + str(detail)))
    if not ok:
        fails.append(name)


def run(display, args, work):
    script = os.path.join(work, "scene")
    with open(script, "w") as h:
        h.write("\n".join(["vibfocus", "vibbandshift %s -" % OTHER,
                           "vibbandclick 2", "vibbandclick 1", ""]))
    result = apps.run(display, "builder", args=args, windowTimeout=120, settle=120,
                      env={"ECCE_VIEWER_SCENE": script, "ECCE_VIEWER_SCENE_OUT": work,
                           "ECCE_REALUSER": fixture.USER})
    log = result.log or ""
    if result.crashed or os.path.exists(os.path.join(work, "FAILED")) or \
            not result.exitedAfterWindow or any(m in log for m in MARKERS):
        check("builder ran the band script", False, "\n".join(log.splitlines()[-20:]))
        return None
    return log


def verify(log, a, b):
    check("Match bands succeeded", "VIBBANDSHIFT: ok=1" in log, log[-600:])
    rows = [l.split("\t") for l in re.findall(r"^VIBBAND: (.*)$", log, re.M) if l]
    want = json.loads(subprocess.run([sys.executable, CLI, a, b, "--json"],
                                     capture_output=True, text=True, check=True).stdout)["matches"]
    check("as many rows as matched bands", len(rows) == len(want) == 3, (len(rows), len(want)))
    for r, m in zip(rows, want):
        ok = (int(r[0]) == m["mode_a"] and int(r[3]) == m["mode_b"] and
              abs(float(r[1]) - m["freq_a"]) < 0.051 and abs(float(r[4]) - m["freq_b"]) < 0.051 and
              abs(float(r[2]) - m["ir_a"]) < 0.051 and abs(float(r[5]) - m["ir_b"]) < 0.051 and
              abs(float(r[6]) - m["shift"]) < 0.051 and abs(float(r[7]) - m["overlap"]) < 0.0006)
        check("row of mode %d equals the CLI" % m["mode_a"], ok, (r, m))
    clicks = [int(x) for x in re.findall(r"VIBBANDCLICK: selected=(-?\d+)", log)]
    check("a row click selects that mode", clicks == [want[2]["mode_a"], want[1]["mode_a"]],
          (clicks, want))


def main():
    import isolate
    import xdisplay
    for var in ("ECCE_DATASERVER_PORT", "ECCE_BROKER_PORT"):
        os.environ.pop(var, None)
    os.environ["ECCE_REALUSER"] = fixture.USER
    settings = isolate.apply(apps.INSTALL)
    isolate.killLeftovers(settings["ECCE_REALUSERHOME"])
    os.environ["ECCE_NO_REAP"] = "1"
    work = tempfile.mkdtemp(prefix="ecce-nmode-bandshift-")
    src = os.path.join(work, NAME)
    scl = os.path.join(work, OTHER)
    shutil.copytree(BASE, src, symlinks=True)
    shutil.copytree(BASE, scl, symlinks=True)
    vf = os.path.join(scl, "Props", "VIBFREQ")
    text = open(vf).read()
    old = re.search(r">([^<]*)</vector>", text).group(1).split()
    new = "  ".join("%.4f" % (float(x) * 0.98) for x in old)
    open(vf, "w").write(re.sub(r">[^<]*</vector>", ">" + new + "</vector>", text))
    try:
        with xdisplay.Display() as display:
            log = []
            apps.startServices(display, log)
            try:
                if not all(apps.serviceState(display).values()):
                    print("FAIL  services did not come up: %s" % log)
                    return 1
                fixture.ensureRealUserAccount()
                restore = fixture.settleUpgradeNotices()
                try:
                    url, error = fixture.install(NAME, source_dir=src)
                    if not error:
                        _u, error = fixture.install(OTHER, source_dir=scl)
                    if error:
                        print("FAIL  install: %s" % error)
                        return 1
                    try:
                        args = ("-context", url)
                        state = fixture.stateHome()
                        if os.path.realpath(state) != os.path.realpath(os.path.expanduser("~")):
                            auth = fixture.authFile(os.path.join(state, ".ECCE", "auth.pipe"),
                                                    user=fixture.USER)
                            args = ("-pipe", auth) + args
                        out = run(display, args, work)
                        if out is not None:
                            verify(out, src, scl)
                    finally:
                        fixture.remove(NAME)
                        fixture.remove(OTHER)
                finally:
                    restore()
            finally:
                apps.stopServices(display)
    finally:
        shutil.rmtree(work, ignore_errors=True)
    print("FAILED: %s" % ", ".join(fails) if fails else "PASS  nmode bandshift")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
