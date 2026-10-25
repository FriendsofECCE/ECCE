#!/usr/bin/env python3
"""
Exploring orbitals: the MO Diagram docked on its own (outside the right-hand
column), Density Map or Density Profile chosen in the Properties list, and a
click on a level showing that MO in the viewer and in the plot.  Headless:
Builder scene commands (dockpane, list, modiagram, layout), no input events.

  * the MO Diagram stays when Density Map / Density Profile is selected;
  * a click on level k shows MO k in the viewer and in the plot, and a click
    on level m moves both;
  * the layout depends on which panels are open, not on the order they were
    opened, closed, floated or docked in;
  * in the right-hand detail slot the MO Diagram is replaced (allowed).

    ECCE_EXPLORE_PNG=<dir>   also copy the window pictures there

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


def text(out, name):
    path = os.path.join(out, name + ".txt")
    return open(path).read() if os.path.exists(path) else ""


def panes(out, name):
    return [l for l in text(out, name).splitlines() if l.strip()]


def titles(log):
    """The Density Map title reported by each dmstate."""
    return re.findall(r"DENSITYMAP: dmstate\n(?:.*\n)*?title (.*)", log)


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
    pngdir = os.environ.get("ECCE_EXPLORE_PNG")
    work = tempfile.mkdtemp(prefix="explore-",
                            dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    xdisplay.SCREEN = "1700x1000x24"
    failures = []

    def check(ok, what):
        print("%s %s" % ("ok   " if ok else "FAIL ", what))
        if not ok:
            failures.append(what)

    def run(display, url, name, scene, shot=False):
        out = os.path.join(work, name)
        png = os.path.join(out, name + ".png") if shot else None
        res = base.runBuilder(display, url, out, scene, shot=png,
                              panel="MO Diagram")
        failed = os.path.join(out, "FAILED")
        check(not res.crashed, "%s: the Builder did not crash" % name)
        check(not os.path.exists(failed), "%s: the scene ran to the end %s" % (
            name, open(failed).read() if os.path.exists(failed) else ""))
        if png and pngdir and os.path.exists(png):
            os.makedirs(pngdir, exist_ok=True)
            shutil.copy(png, pngdir)
        return out, res.log or ""

    top = "dockpane MO Diagram top\n"
    with xdisplay.Display() as display:
        subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")],
                       env=display.env(), timeout=180)
        try:
            url = base.makeCalc(data, "water")

            #  (a) MO Diagram on top, then Density Map, then clicks.
            scene = ("hold 2\n" + top + "list Density Map\nhold 2\n"
                     "layout map\n"
                     "modiagram c1 click 3\nmogrid g1\ndmstate\n"
                     "modiagram c2 click 4\nmogrid g2\ndmstate\n"
                     "list Density Profile\nhold 2\nlayout prof\n"
                     "modiagram c3 click 3\ndpstate\n"
                     "modiagram c4 click 4\ndpstate\nhold 2\n")
            out, log = run(display, url, "a", scene, shot=True)
            lay = text(out, "map")
            check("pane MO Diagram" in lay and "pane Density Map" in lay,
                  "MO Diagram and Density Map are both shown: %s" % lay)
            check(re.search(r"pane MO Diagram dir=1 ", lay) is not None,
                  "the MO Diagram is docked on top")
            lay = text(out, "prof")
            check("pane MO Diagram" in lay and "pane Density Profile" in lay,
                  "MO Diagram and Density Profile are both shown: %s" % lay)
            g1, g2 = text(out, "g1"), text(out, "g2")
            check("fieldMax" in g1 and "fieldMax" in g2 and g1 != g2,
                  "the viewer shows MO 4, then MO 5 (%r, %r)" % (g1, g2))
            t = titles(log)
            check(len(t) >= 2 and t[0] != t[1],
                  "the Density Map follows the click: %s" % t)
            curves = re.findall(r"^curve '(.*?)'", log, re.M)
            check(len(set(curves)) >= 2,
                  "the Density Profile follows the click: %s" % curves)

            #  (b) one layout whatever the order of opening and docking.
            orders = {
                "o1": "list Density Map\n" + top,
                "o2": top + "list Density Map\n",
                "o3": "dockpane MO Diagram float\nlist Density Map\n" + top,
                "o4": top + "list Density Map\nproperty MO Diagram\n"
                      "property MO Diagram\n" + top,
            }
            layouts = {}
            for name, steps in orders.items():
                out, _ = run(display, url, name,
                             "hold 2\n" + steps + "hold 2\nlayout end\n")
                layouts[name] = panes(out, "end")
            ref = layouts["o2"]
            for name, got in sorted(layouts.items()):
                check(got == ref, "%s ends in the same layout as o2%s" % (
                    name, "" if got == ref else ":\n%s\nvs\n%s" % (
                        "\n".join(got), "\n".join(ref))))

            #  (c) in the right-hand slot the MO Diagram is replaced.
            out, _ = run(display, url, "c",
                         "hold 2\ndockpane MO Diagram right\nlist MO Diagram\nhold 1\n"
                         "list Density Map\n"
                         "hold 2\nlayout end\n")
            lay = text(out, "end")
            check("pane Density Map" in lay and "pane MO Diagram" not in lay,
                  "in the detail slot the MO Diagram is replaced: %s" % lay)

            #  The tab buttons stay one row with the detail pane closed.
            out, _ = run(display, url, "d",
                         "hold 2\nlist Density Map\nhold 2\n"
                         "property Density Map\nhold 2\nlayout closed\n",
                         shot=True)
            tabs = re.search(r"pane Panel Tabs .* h=(\d+)",
                             text(out, "closed"))
            check(tabs is not None and int(tabs.group(1)) <= 40,
                  "tab buttons stay one row, detail closed: %s" %
                  (tabs.group(1) if tabs else "?"))
            index = re.search(r"pane Property Index .* h=(\d+)",
                              text(out, "closed"))
            check(index is not None and int(index.group(1)) > 150,
                  "the list takes the room, detail closed")
        finally:
            subprocess.run([os.path.join(apps.INSTALL, "bin",
                                         "ecce-gateway-stop")],
                           env=display.env(), timeout=120)
    shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
