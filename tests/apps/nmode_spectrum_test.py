#!/usr/bin/env python3
"""Vibrations panel spectrum extras (#707): the exported curves equal ecce-spectrum.

    tests/apps/nmode_spectrum_test.py

A Builder opens the Gaussian 16 water frequency calculation; a copy with
frequencies scaled by 0.98 is in the same folder.  The scene command
"vibspectrum" compares with the copy, switches the difference curve on,
imports a transmittance CSV as the experimental spectrum, zooms, and writes
the export CSV.  Every column of its curve block (computed, other, difference,
experimental) must equal `ecce-spectrum` on the stored calculations and the
same file.
"""
import math
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
NAME = "nmode-spec-water"
OTHER = NAME + "-scaled"
CLI = os.path.join(ROOT, "scripts", "ecce_spectrum.py")
MARKERS = ("ended by SIG", "Segmentation fault", "double free or corruption",
           "terminate called", "Unhandled standard exception")
fails = []


def check(name, ok, detail=""):
    print(("PASS  " if ok else "FAIL  ") + name + ("" if ok else "  " + str(detail)))
    if not ok:
        fails.append(name)


def writeExp(path):
    with open(path, "w") as f:
        f.write("# transmittance, percent\nwavenumber,T\n")
        for x in range(500, 3951, 3):
            a = 0.8 * math.exp(-((x - 1612) / 25.0) ** 2) + 0.2 * math.exp(-((x - 3790) / 30.0) ** 2) + 0.02
            f.write("%d,%.5f\n" % (x, 100 * 10 ** -a))


def run(display, args, work, exp, csv):
    script = os.path.join(work, "scene")
    with open(script, "w") as h:
        h.write("\n".join(["vibfocus", "vibspectrum %s %s 1 %s 1000,2000" % (csv, OTHER, exp), ""]))
    result = apps.run(display, "builder", args=args, windowTimeout=120, settle=120,
                      env={"ECCE_VIEWER_SCENE": script, "ECCE_VIEWER_SCENE_OUT": work,
                           "ECCE_REALUSER": fixture.USER})
    log = result.log or ""
    if result.crashed or os.path.exists(os.path.join(work, "FAILED")) or \
            not result.exitedAfterWindow or any(m in log for m in MARKERS):
        check("builder ran the spectrum script", False, "\n".join(log.splitlines()[-20:]))
        return None
    return log


def verify(log, a, b, exp, csv):
    check("spectrum script succeeded", "VIBSPECTRUM: ok=1" in log, log[-600:])
    text = open(csv, encoding="utf-8").read().splitlines()
    joined = "\n".join(text)
    fwhm = float(re.search(r"# broadening: \w+, FWHM (\S+) cm-1", joined).group(1))
    scale = float(re.search(r"# frequency scaling factor: (\S+)", joined).group(1))
    at = [i for i, l in enumerate(text) if l.startswith("wavenumber_cm-1")][0]
    head = text[at].split(",")
    rows = [l.split(",") for l in text[at + 1:] if l]
    xs = [float(r[0]) for r in rows]
    check("curve block has the four curves", len(head) == 5 and head[3].startswith("difference_")
          and head[4] == "experimental", head)
    out = subprocess.run([sys.executable, CLI, a, b, "--exp", exp, "--fwhm", repr(fwhm),
                          "--scale", repr(scale), "--lo", repr(xs[0]), "--hi", repr(xs[-1]),
                          "--step", "1", "--csv"], capture_output=True, text=True)
    check("ecce-spectrum ran", out.returncode == 0, out.stderr)
    want = [l.split(",") for l in out.stdout.splitlines()[1:]]
    check("same number of points", len(want) == len(rows), (len(want), len(rows)))
    top = max(float(r[1]) for r in want)
    for c, name in enumerate(["x", "computed", "other", "difference", "experimental"]):
        err = 0.0
        for r, w in zip(rows, want):
            if (r[c] == "") != (w[c] == ""):
                err = 9
                break
            if r[c]:
                err = max(err, abs(float(r[c]) - float(w[c])))
        check(name + " column equals ecce-spectrum", err < 1e-5 * max(top, 1), err)
    check("experimental column has data", any(r[4] for r in rows))


def main():
    import isolate
    import xdisplay
    for var in ("ECCE_DATASERVER_PORT", "ECCE_BROKER_PORT"):
        os.environ.pop(var, None)
    os.environ["ECCE_REALUSER"] = fixture.USER
    settings = isolate.apply(apps.INSTALL)
    isolate.killLeftovers(settings["ECCE_REALUSERHOME"])
    os.environ["ECCE_NO_REAP"] = "1"
    work = tempfile.mkdtemp(prefix="ecce-nmode-spectrum-")
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
                        exp = os.path.join(work, "exp.csv")
                        csv = os.path.join(work, "spectrum.csv")
                        writeExp(exp)
                        out = run(display, args, work, exp, csv)
                        if out is not None:
                            verify(out, src, scl, exp, csv)
                    finally:
                        fixture.remove(NAME)
                        fixture.remove(OTHER)
                finally:
                    restore()
            finally:
                apps.stopServices(display)
    finally:
        shutil.rmtree(work, ignore_errors=True)
    print("FAILED: %s" % ", ".join(fails) if fails else "PASS  nmode spectrum")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
