#!/usr/bin/env python3
"""
Dipole Moment panel (#578): a plain arrow from the centre of negative to the
centre of positive partial charge, "Dipole moment x.xx D (ESP charges)" in the viewer corner, no Scale slider;
mu = 0 gives no arrow and the label "Dipole moment \u03bc = 0 D".  Headless Builder scene
commands on the stored example calculations; the poles are recomputed here
from the stored charges (CHELPG, else Merz-Kollman, else Mulliken) and the
atom coordinates the Builder reports.

    ECCE_DIPOLE_PNG=<dir>   also copy the window pictures there

Same installed tree and isolation as spin_ao_test.py.  Exit 77 (skip)
without an install or Xvfb.
"""
import math
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps                   # noqa: E402
import isolate                # noqa: E402
import orca_mo_test as base   # noqa: E402
import xdisplay               # noqa: E402

REPO = os.path.dirname(os.path.dirname(HERE))
EX = os.path.join(REPO, "data", "client", "examples")
CASES = [
    ("so2", "genchem/polar-bonds-and-dipole-moments/ecceqm-SO2/stored/SO2"),
    ("hf", "genchem/polar-bonds-and-dipole-moments/ecceqm-HF_dipole/stored/HF_dipole"),
    ("co2", "genchem/polar-bonds-and-dipole-moments/ecceqm-CO2/stored/CO2"),
    # CO: the CHELPG charges put O negative, mu puts C negative (#653).
    ("co", "genchem/molecular-orbitals-of-diatomics/ecceqm-CO/stored/CO"),
    ("hcl", "genchem/polar-bonds-and-dipole-moments/ecceqm-HCl/stored/HCl"),
    ("h2o", "getting-started/ecceqm-water/stored/water"),
    ("nh3", "getting-started/mopac-ammonia/stored/ammonia"),
    ("acala", "spectroscopy/amides-and-peptides/orca-ac_ala_nhme/stored/ac_ala_nhme"),
]


def vec(s):
    return [float(x) for x in re.split(r"[,\s]+", s.strip())]


def stored(calc, key):
    path = os.path.join(calc, "Props", key)
    if not os.path.exists(path):
        return None
    txt = open(path).read()
    return [float(x) for x in re.search(r"<vector[^>]*>([^<]*)<", txt).group(1).split()]


def poles(calc, xyz):
    for key in ("ESPFIT", "MKFIT", "MULLIKEN"):
        q = stored(calc, key)
        if q:
            break
    out = []
    for sign in (-1, 1):
        sel = [(qi, c) for qi, c in zip(q, xyz) if qi * sign > 0]
        tot = sum(qi for qi, _ in sel)
        out.append([sum(qi * c[k] for qi, c in sel) / tot for k in range(3)])
    return key, out[0], out[1]


def runBuilder(display, url, out, png, turn):
    """ECCE_BUILDER_SCRIPT run with the Dipole Moment panel open; the
    window picture is taken at the script's "shot" and the log returned."""
    os.makedirs(out, exist_ok=True)
    lines = ["wait 3000", "background 1 1 1", "viewall"]
    if turn:
        lines.append("rotate %d" % turn)
    lines += ["wait 1500", "coords", "vector DIPOLE", "wait 500", "shot s", "quit"]
    with open(os.path.join(out, "script"), "w") as handle:
        handle.write("\n".join(lines) + "\n")
    log = os.path.join(out, "builder.log")
    env = display.env()
    env.update(ECCE_BUILDER_SCRIPT=os.path.join(out, "script"),
               ECCE_OPEN_PANEL="Dipole Moment",
               ECCE_TRANSPARENCY_FALLBACK_MS="0")
    proc = subprocess.Popen([os.path.join(apps.WRAPPERS, "ecce-builder"),
                             "-context", url], env=env,
                            stdout=open(log, "w"), stderr=subprocess.STDOUT,
                            start_new_session=True)
    ready = os.path.join(out, "s.ready")
    deadline = time.time() + 240
    try:
        while time.time() < deadline and proc.poll() is None:
            if os.path.exists(ready):
                time.sleep(1.5)
                argv = ["import", "-display", display.name, "-window", "root"]
                best = None
                for wid, _t in display.windows():
                    info = subprocess.run(["xwininfo", "-display", display.name,
                                           "-id", wid], capture_output=True,
                                          text=True).stdout
                    g = dict(re.findall(r"(Absolute upper-left [XY]|Width|"
                                        r"Height):\s+(-?\d+)", info))
                    if len(g) == 4 and (best is None or int(g["Width"]) *
                                        int(g["Height"]) > int(best["Width"]) *
                                        int(best["Height"])):
                        best = g
                if best:
                    argv += ["-crop", "%sx%s+%s+%s" % (
                        best["Width"], best["Height"],
                        best["Absolute upper-left X"],
                        best["Absolute upper-left Y"]), "+repage"]
                subprocess.run(argv + [png], check=False)
                os.unlink(ready)
                open(os.path.join(out, "s.go"), "w").close()
            time.sleep(0.3)
        proc.wait(timeout=30)
    except subprocess.TimeoutExpired:
        pass
    finally:
        if proc.poll() is None:
            os.killpg(proc.pid, 9)
    return open(log, errors="replace").read()


def main():
    if "ECCE_TEST_WRAPPERS" not in os.environ:
        print("SKIP: set ECCE_TEST_HOME and ECCE_TEST_WRAPPERS to a private install")
        return 77
    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-builder")):
        print("SKIP: no ecce-builder under %s" % apps.WRAPPERS)
        return 77
    if not shutil.which("Xvfb"):
        print("SKIP: no Xvfb")
        return 77
    isolate.apply(apps.INSTALL)
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    pngdir = os.environ.get("ECCE_DIPOLE_PNG")
    work = tempfile.mkdtemp(prefix="dipole-", dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    xdisplay.SCREEN = "1700x1000x24"
    failures = []

    def check(ok, what):
        print("%s %s" % ("ok   " if ok else "FAIL ", what))
        if not ok:
            failures.append(what)

    # The molecule is turned so the arrow lies across the view.
    turn = {"so2": 90, "h2o": 90, "hcl": 90, "hf": 90, "nh3": 70, "co": 90}
    with xdisplay.Display() as display:
        subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")],
                       env=display.env(), timeout=180)
        try:
            for name, rel in CASES:
                src = os.path.join(EX, rel)
                url = base.makeCalc(data, name, source=src)
                calc = url[len("file://"):].rstrip("/")
                out = os.path.join(work, name)
                png = os.path.join(out, name + ".png")
                log = runBuilder(display, url, out, png,
                                 turn.get(name, 0))
                if pngdir and os.path.exists(png):
                    os.makedirs(pngdir, exist_ok=True)
                    shutil.copy(png, os.path.join(pngdir, name + ".png"))
                if "label=" not in log and "arrows=0" not in log:
                    print("---- %s log tail\n%s\n----" % (name, log[-1500:]))
                xyz = [[float(x) for x in m.groups()] for m in re.finditer(
                    r"BUILDER: atom \d+ \S+ (\S+) (\S+) (\S+)", log)]
                mu = stored(calc, "DIPOLE")
                mag = math.sqrt(sum(x * x for x in mu))
                label = re.search(r"vector DIPOLE label=(.*)", log)
                label = label.group(1).strip() if label else None
                arrows = re.search(r"vector DIPOLE arrows=(\d+)", log)
                check(arrows is not None, "%s: the scene reported the vector" % name)
                if not arrows:
                    continue
                n = int(arrows.group(1))
                sl = re.search(r"vector DIPOLE slider=(\d+)", log)
                check(sl is not None and sl.group(1) == "0",
                      "%s: no Scale slider on the panel" % name)
                if mag < 0.005:
                    check(n == 0, "%s: mu = 0, no arrow" % name)
                    check(label == "Dipole moment \u03bc = 0 D", "%s: label is 'Dipole moment \u03bc = 0 D' (%r)" % (name, label))
                    continue
                key, neg, pos = poles(calc, xyz)
                # Charges pointing against mu: the arrow runs from their
                # positive pole to their negative one, along mu.
                oppose = sum((pos[k] - neg[k]) * mu[k] for k in range(3)) < 0
                if oppose:
                    neg, pos = pos, neg
                short = {"ESPFIT": "ESP", "MKFIT": "MK", "MULLIKEN": "Mulliken"}[key]
                short += " charges point the other way" if oppose else " charges"
                check(n == 1, "%s: one arrow" % name)
                check(oppose == (name == "co"), "%s: charges against mu only for CO" % name)
                check(label == "Dipole moment \u03bc = %.2f D (%s)" % (mag, short),
                      "%s: corner text 'Dipole moment \u03bc = %.2f D' (%r)" % (name, mag, label))
                m = re.search(r"vector DIPOLE 0 start=(\S+) dir=(\S+) "
                              r"length=(\S+)", log)
                if not m:
                    check(False, "%s: arrow geometry reported" % name)
                    continue
                start, d, length = vec(m.group(1)), vec(m.group(2)), float(m.group(3))
                tip = [start[k] + d[k] * length for k in range(3)]
                dist = lambda a, b: math.sqrt(sum((a[k] - b[k]) ** 2 for k in range(3)))
                uh = [x / mag for x in mu]
                along = lambda a, b: sum((a[k] - b[k]) * uh[k] for k in range(3))
                off = dist(tip, pos)
                print("     %s charges, mu %.3f D, poles %.3f A apart, tail-neg %.3f "
                      "tip-pos %.3f (along mu %.3f)" % (key, mag, dist(neg, pos),
                      dist(start, neg), off, along(tip, pos)))
                check(dist(start, neg) < 0.05, "%s: tail at the negative pole "
                      "(the positive one where the charges oppose mu)" % name)
                # Where the charges reproduce mu the head is at the positive
                # pole; where the pole line is skew to mu it is level with it
                # along mu, and the pole is that far off the arrow's line.
                check(abs(along(tip, pos)) < 0.05,
                      "%s: head level with the positive pole along mu" % name)
                check(off < 0.05 or name == "acala",
                      "%s: head at the positive pole (%.3f A)" % (name, off))
                dot = sum(d[k] * mu[k] for k in range(3)) / mag
                check(dot > 0.99, "%s: the arrow points along mu (cos %.3f)" % (name, dot))
        finally:
            pass
    if not failures:
        shutil.rmtree(work, ignore_errors=True)
    else:
        print("work dir kept:", work)
    print("FAILED: %d" % len(failures) if failures else "all ok")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
