#!/usr/bin/env python3
"""The MO diagram for a MOPAC result (#911): levels with the code's symmetry
labels and occupations, and a note that there is no composition.

    tests/apps/modiagram_mopac_test.py [--shot FILE.png]

Uses the stored getting-started example (mopac-ammonia).  Same isolation as
modiagram_panel_test.py.  Exit 77 (skip) without an install or Xvfb.
"""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps                   # noqa: E402
import isolate                # noqa: E402
import orca_mo_test as base   # noqa: E402
import xdisplay               # noqa: E402

AMMONIA = os.path.join(os.path.dirname(HERE), "..", "data", "client", "examples",
                       "getting-started", "mopac-ammonia", "stored", "ammonia")


def text(out, name):
    path = os.path.join(out, name + ".txt")
    return open(path).read() if os.path.exists(path) else ""


def main():
    shot = None
    if "--shot" in sys.argv:
        shot = os.path.abspath(sys.argv[sys.argv.index("--shot") + 1])
    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-builder")):
        print("SKIP: no ecce-builder under %s" % apps.WRAPPERS)
        return 77
    if not shutil.which("Xvfb"):
        print("SKIP: no Xvfb")
        return 77
    isolate.apply(apps.INSTALL)
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    work = tempfile.mkdtemp(prefix="modiagram-mopac-",
                            dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    xdisplay.SCREEN = "1366x1000x24" if shot else "1366x768x24"
    failures = []

    def check(ok, what):
        print("%s %s" % ("ok   " if ok else "FAIL ", what))
        if not ok:
            failures.append(what)

    with xdisplay.Display() as display:
        subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")],
                       env=display.env(), timeout=180)
        try:
            url = base.makeCalc(data, "ammonia", source=os.path.normpath(AMMONIA))
            out = os.path.join(work, "run")
            scene = "hold 3\nmodiagram st status 0\n"
            for n in range(0, 8):
                scene += "modiagram tip%d tip %d\n" % (n, n)
            if shot:
                scene += "shot mopac\n"
            result = base.runBuilder(display, url, out, scene, panel="MO Diagram",
                                     shot=shot)
            check(not result.crashed, "the Builder did not crash")
            check(not os.path.exists(os.path.join(out, "FAILED")),
                  "the scene ran to the end: %s" % text(out, "FAILED"))
            st = text(out, "st")
            check("semi-empirical" in st and "composition" in st,
                  "the panel's note says composition is not available (%r)" % st.strip())
            tips = [text(out, "tip%d" % n).strip() for n in range(0, 8)]
            shown = [t for t in tips if t]
            print("\n".join(shown))
            check(len(shown) == 5, "five levels, the e pairs grouped (%d)" % len(shown))
            check(all("composition" not in t for t in shown),
                  "no level tooltip claims a composition")
            check(any("occupancy 2" in t for t in shown) and
                  any("occupancy 0" in t for t in shown),
                  "levels carry ORBOCC occupations")
            check(any("a1" in t.lower() for t in shown) and
                  any("e" in t.lower().split("irrep")[-1] for t in shown),
                  "levels carry the a1 and e labels")
        finally:
            subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-stop")],
                           env=display.env(), timeout=120)
    shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
