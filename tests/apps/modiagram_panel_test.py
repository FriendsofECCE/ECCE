#!/usr/bin/env python3
"""The MO diagram in the Builder, headless: clicks, tooltips, composition.

    tests/apps/modiagram_panel_test.py

Water (calc-orca-water-ccpvdz), with only the MO Diagram panel opened:

  * a click on a level draws that orbital although the MOs panel was never
    opened (its first-time initialisation is done by the click);
  * a level's tooltip text names the MO, its energy, irrep and occupancy and
    carries the MOs panel's composition line ("MO n, composition (Mulliken
    population): ..."); a degenerate set names every MO and says the
    composition is their mean.

Same installed tree and isolation as orca_mo_test.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
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


def text(out, name):
    path = os.path.join(out, name + ".txt")
    return open(path).read() if os.path.exists(path) else None


BENZENE = os.path.join(os.path.dirname(HERE), "mocomp", "fixtures", "benzene")


def makeBenzene(data):
    """Benzene (D6h, doubly degenerate e levels): the water calculation's
    shell with the benzene geometry, basis and orbitals put in."""
    url = base.makeCalc(data, "benzene")
    calc = url[len("file://"):].rstrip("/")
    for name in ("chemsys.mvm", "BasisSet.ecce_basisset"):
        shutil.copy(os.path.join(BENZENE, "Parameters", name),
                    os.path.join(calc, "Parameters", name))
    for name in os.listdir(os.path.join(calc, "Props")):
        if not name.startswith("."):
            os.unlink(os.path.join(calc, "Props", name))
    for name in os.listdir(os.path.join(BENZENE, "Props")):
        if not name.startswith("."):
            shutil.copy(os.path.join(BENZENE, "Props", name),
                        os.path.join(calc, "Props", name))
    return url


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
    work = tempfile.mkdtemp(prefix="modiagram-",
                            dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    xdisplay.SCREEN = "1366x768x24"
    failures = []

    def check(ok, what):
        print("%s %s" % ("ok   " if ok else "FAIL ", what))
        if not ok:
            failures.append(what)

    with xdisplay.Display() as display:
        subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")],
                       env=display.env(), timeout=180)
        try:
            url = base.makeCalc(data, "water")
            out = os.path.join(work, "run")
            scene = "hold 3\n"
            for n in range(0, 8):
                scene += "modiagram tip%d tip %d\n" % (n, n)
                scene += "modiagram ctip%d ctip %d\n" % (n, n)
            scene += "modiagram click click 4\nmogrid grid\n"
            result = base.runBuilder(display, url, out, scene,
                                     panel="MO Diagram")
            log = result.log or ""
            failed = os.path.join(out, "FAILED")
            check(not result.crashed, "the Builder did not crash")
            check(not os.path.exists(failed), "the scene ran to the end: %s" % (
                open(failed).read() if os.path.exists(failed) else ""))
            click = text(out, "click") or ""
            check("grid yes" in click,
                  "a click draws the orbital without the MOs panel opened "
                  "first (%s)" % click.strip())
            check("cannot be computed" not in log,
                  "no 'cannot be computed' message")
            tips = [text(out, "ctip%d" % n) or "" for n in range(0, 8)]
            mtips = [text(out, "tip%d" % n) or "" for n in range(0, 8)]
            check(any("Made of:" in t and "Matches your MO" in t for t in mtips),
                  "a model level's hover gives its fragment composition and its match")
            check(any("matches model" in t for t in tips),
                  "a calculation level's hover names its model partner")
            shown = [t for t in tips if t.strip()]
            check(len(shown) >= 3, "level tooltips exist (%d)" % len(shown))
            heads = [t.splitlines()[0] for t in shown]
            check(all(("MO " in h or "MOs " in h) and "E = " in h and
                      "occupancy" in h for h in heads),
                  "every tooltip head names MO, energy, occupancy")
            check(all("composition (Mulliken population):" in t for t in shown),
                  "every tooltip carries the MOs panel's composition line")
            check(any("O1" in t for t in shown), "the composition names O1")
            degenerate = [t for t in shown if t.startswith("MOs ")]
            print("tooltips:\n" + "\n".join(shown[:3]))
            if degenerate:
                check("mean over its orbitals" in degenerate[0],
                      "a degenerate set says its composition is a mean")
            url = makeBenzene(data)
            out = os.path.join(work, "benzene")
            scene = "hold 3\n"
            for n in range(0, 30):
                scene += "modiagram tip%d ctip %d\n" % (n, n)
            res = base.runBuilder(display, url, out, scene, panel="MO Diagram")
            tips = [text(out, "tip%d" % n) or "" for n in range(0, 30)]
            if not any(t.strip() for t in tips):
                print("no benzene tooltips; %s\n%s" % (
                    text(out, "FAILED"), "\n".join(
                        (res.log or "").splitlines()[-15:])))
            print("benzene tooltips:\n" + "\n".join(
                t.splitlines()[0] for t in tips if t.strip()))
            pairs = [t for t in tips if t.startswith("MOs ")]
            print("benzene, degenerate sets:\n" + "\n".join(pairs[:2]))
            check(len(pairs) >= 1, "benzene has a degenerate level in the diagram "
                  "(%d)" % len(pairs))
            check(all("mean over its orbitals" in t and
                      "composition (Mulliken population):" in t
                      for t in pairs),
                  "a degenerate set names its MOs and gives the mean "
                  "composition")
        finally:
            subprocess.run([os.path.join(apps.INSTALL, "bin",
                                         "ecce-gateway-stop")],
                           env=display.env(), timeout=120)
    shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
