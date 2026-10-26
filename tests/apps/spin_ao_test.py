#!/usr/bin/env python3
"""
MOs panel: spin density, the stored virtual-orbital note, and an atom's
orbitals named by shell.  Headless Builder scene commands, no input events.

  * O2 triplet (restricted open shell): "Spin Density" is offered, its grid
    has both signs; closed-shell Ne offers none and says why in a tooltip;
  * Ne with improved virtual orbitals: the panel reports "Virtual orbitals:
    improved (IVO)", the table names 1s 2s 2p and "3s (virtual)";
  * Ne MO Diagram levels carry the same names.

    ECCE_SPINAO_PNG=<dir>   also copy the window pictures there

Same installed tree and isolation as orca_mo_test.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
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

REPO = os.path.dirname(os.path.dirname(HERE))
RD = os.path.join(REPO, "tests", "builder", "radialdist-fixtures")
O2 = os.path.join(REPO, "tests", "ecceqm", "fixtures", "o2-roks-loc")


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
    pngdir = os.environ.get("ECCE_SPINAO_PNG")
    work = tempfile.mkdtemp(prefix="spinao-", dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    xdisplay.SCREEN = "1700x1000x24"
    print("work dir", work)
    failures = []

    def check(ok, what):
        print("%s %s" % ("ok   " if ok else "FAIL ", what))
        if not ok:
            failures.append(what)

    def run(display, url, name, scene, shot=False, panel="MOs", env=None):
        out = os.path.join(work, name)
        png = os.path.join(out, name + ".png") if shot else None
        res = base.runBuilder(display, url, out, scene, shot=png, panel=panel,
                              extraEnv=env)
        check(not res.crashed, "%s: the Builder did not crash" % name)
        failed = os.path.join(out, "FAILED")
        check(not os.path.exists(failed), "%s: the scene ran to the end %s" % (
            name, open(failed).read() if os.path.exists(failed) else ""))
        if png and pngdir and os.path.exists(png):
            os.makedirs(pngdir, exist_ok=True)
            shutil.copy(png, pngdir)
        return out

    with xdisplay.Display() as display:
        subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")],
                       env=display.env(), timeout=180)
        try:
            o2 = base.makeCalc(data, "o2", source=O2)
            ne = base.makeCalc(data, "ne", source=os.path.join(RD, "ne"))
            ivo = base.makeCalc(data, "neivo", source=os.path.join(RD, "ne-ivo"))

            out = run(display, o2, "o2",
                      "hold 2\nmotable t\nmofield Spin Density\nbackground 1 1 1\n"
                      "mopanel s\nmogrid g\nrotate 90\nviewall\nhold 2\n", shot=True)
            t = text(out, "t")
            check(re.search(r"^fieldTypes .*Spin Density", t, re.M) is not None,
                  "O2 triplet offers Spin Density: %s" % t[:300])
            g = text(out, "g")
            m = re.search(r"fieldMin (\S+)\nfieldMax (\S+)", g)
            check(m is not None and float(m.group(2)) > 0.05 and float(m.group(1)) > -1e-6,
                  "the O2 ROKS spin density is positive (pi* electrons only): %s" % g.strip())

            out = run(display, ne, "ne", "hold 2\nmotable t\nhold 1\n", shot=True)
            t = text(out, "t")
            check("Spin Density" not in t.split("\n", 12)[-1] and
                  re.search(r"^fieldTypes .*Spin", t, re.M) is None,
                  "closed-shell Ne offers no Spin Density")
            check(re.search(r"^fieldTypeTip .*open-shell", t, re.M) is not None,
                  "the field-type choice says why")
            check(re.search(r"^virtNote *$", t, re.M) is not None,
                  "no virtual-orbital note for a canonical run")
            check(re.search(r"^row .*\| 2p \|", t, re.M) is not None and
                  re.search(r"^row .*\| 1s \|", t, re.M) is not None,
                  "the Ne table names the occupied orbitals by shell")

            # an open shell splits the energies of the 2p set, not its name
            f = base.makeCalc(data, "f", source=os.path.join(
                REPO, "data", "client", "examples", "genchem", "atoms", "ecceqm-F", "stored", "F"))
            out = run(display, f, "f", "hold 2\nmotable t\nhold 1\n")
            t = text(out, "t")
            check(re.search(r"^row .*\| 2p \|", t, re.M) is not None and
                  re.search(r"^row .*\| 3p \|", t, re.M) is None,
                  "open-shell F: its occupied p orbitals are all 2p")
            dump = os.path.join(work, "df.dump")
            run(display, f, "df", "hold 2\n", panel="MO Diagram",
                env={"ECCE_MODIAGRAM_DUMP": dump})
            d = open(dump).read() if os.path.exists(dump) else ""
            check("label=2p" in d and "label=3p" not in d,
                  "open-shell F: the diagram has no 3p level")

            out = run(display, ivo, "neivo", "hold 2\nmotable t\nhold 1\n", shot=True)
            t = text(out, "t")
            check("virtNote Virtual orbitals: improved (IVO)" in t,
                  "IVO run reports its virtual orbitals")
            check(re.search(r"^row .*\| 3s \(virtual\) \|", t, re.M) is not None and
                  re.search(r"^row .*\| 3p \(virtual\) \|", t, re.M) is not None,
                  "the IVO table names 3s and 3p virtuals")

            for name, url in (("dne", ne), ("divo", ivo)):
                dump = os.path.join(work, name + ".dump")
                run(display, url, name, "hold 2\n", panel="MO Diagram",
                    shot=True, env={"ECCE_MODIAGRAM_DUMP": dump})
                d = open(dump).read() if os.path.exists(dump) else ""
                check("label=2p" in d, "%s: the diagram names its 2p level" % name)
                if name == "divo":
                    check("3s (virtual)" in d, "divo: diagram names 3s (virtual)")
        finally:
            subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-stop")],
                           env=display.env(), timeout=120)
    keep = os.environ.get("SPINAO_KEEP")      # a directory to copy the outputs to
    if keep:
        shutil.copytree(work, keep, dirs_exist_ok=True,
                        ignore=shutil.ignore_patterns("data", "*.png"))
    shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
