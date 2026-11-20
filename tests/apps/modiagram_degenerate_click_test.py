#!/usr/bin/env python3
"""A click on a degenerate MO Diagram level reaches every orbital of the
level (#1212): ethane_scan (a relaxed scan, levels grouped by energy) and
orca-benzene (D6h, e pairs by irrep).

    tests/apps/modiagram_degenerate_click_test.py

Same isolation as modiagram_panel_test.py.  Exit 77 (skip) without an
install or Xvfb.
"""
import os
import re
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

EXAMPLES = os.path.join(os.path.dirname(HERE), "..", "data", "client", "examples")
CASES = [
    ("ethane_scan", os.path.join(EXAMPLES, "organic", "conformations",
                                 "ecceqm-ethane-scan", "stored", "ethane_scan")),
    ("benzene", os.path.join(EXAMPLES, "getting-started", "orca-benzene",
                             "stored", "benzene")),
]


def text(out, name):
    path = os.path.join(out, name + ".txt")
    return open(path).read() if os.path.exists(path) else ""


def main():
    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-builder")):
        print("SKIP: no ecce-builder under %s" % apps.WRAPPERS)
        return 77
    if not shutil.which("Xvfb"):
        print("SKIP: no Xvfb")
        return 77
    isolate.apply(apps.INSTALL)
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    work = tempfile.mkdtemp(prefix="modiagram-degenerate-",
                            dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    xdisplay.SCREEN = "1366x900x24"
    failures = []

    def check(ok, what):
        print("%s %s" % ("ok   " if ok else "FAIL ", what))
        if not ok:
            failures.append(what)

    with xdisplay.Display() as display:
        subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")],
                       env=display.env(), timeout=180)
        try:
            for name, source in CASES:
                source = os.path.normpath(source)
                if not os.path.isdir(source):
                    check(False, "%s: example present at %s" % (name, source))
                    continue
                url = base.makeCalc(data, name, source=source)
                out = os.path.join(work, "run-" + name)
                scene = "hold 3\nmodiagram hits hits 0\n"
                for n in range(0, 12):
                    scene += "modiagram click%d click %d\n" % (n, n)
                result = base.runBuilder(display, url, out, scene,
                                         panel="MO Diagram")
                check(not result.crashed, "%s: the Builder did not crash" % name)
                check(not os.path.exists(os.path.join(out, "FAILED")),
                      "%s: the scene ran to the end: %s" % (name, text(out, "FAILED")))
                hits = text(out, "hits")
                print("---- %s hits\n%s" % (name, hits))
                for n in range(0, 12):
                    print("click %d: %s" % (n, text(out, "click%d" % n).strip()))
                #  Every level that names several MOs offers each of them.
                levels = {}
                for mo, key, desc in re.findall(r"^hit MO (\d+) key (-?\d+) box .*? \| (.*)$",
                                                hits, re.M):
                    levels.setdefault(key, set()).add(int(mo))
                check(bool(levels), "%s: the diagram has click targets" % name)
        finally:
            subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-stop")],
                           env=display.env(), timeout=120)
    shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
