#!/usr/bin/env python3
"""The MD Optimize editor for a GROMACS and an NWChem MD study, and the
Organizer's editor button for each.

Local data, private X server, the tree's own binaries.  A GROMACS MD study
and an NWChem MD study are made through the classes behind the New menu
(launchjob gromacsstudy / nwchemmdstudy); each Optimize task is opened in
mdoptimize through its test hook (ECCE_TEST_MDED) and must stay up with the
right title and pages; the Organizer's summary of each (ECCE_TEST_ORGANIZER
"summary") must name the editor of the task's own code on a button that
holds the whole label.

    tests/launch/md_editors_test.py [--build build-cmake] [--keep] [--png DIR]

Exit status 77 (CTest SKIP) without Xvfb or the built binaries.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "apps"))

import harness  # noqa: E402
from harness import say  # noqa: E402
from gromacs_test import Editor, stop, check, failures  # noqa: E402

GUI = ("mdoptimize", "organizer")


def summary(s, display, genv, wrappers, urls, png=None):
    """The Organizer's "summary" answer for each url (and its window in png)."""
    cmd = os.path.join(s.state, "organizer.cmd")
    open(cmd, "w").close()
    logpath = os.path.join(s.state, "organizer.log")
    org = subprocess.Popen(
        [os.path.join(wrappers, "ecce-organizer")],
        env=dict(genv, ECCE_TEST_ORGANIZER=cmd), cwd=os.path.join(s.home, "bin"),
        stdout=open(logpath, "w"), stderr=subprocess.STDOUT,
        start_new_session=True)
    answers = {}
    try:
        deadline = time.time() + 120
        while time.time() < deadline and not any(
                "Organizer" in (w[1] or "") for w in display.windows()):
            time.sleep(0.5)
        time.sleep(3)
        with open(cmd, "a") as h:
            for i, u in enumerate(urls):
                h.write("summary %s\n" % u)
                if png:
                    h.write("snap %s\n" % os.path.join(png, "summary-%d.png" % i))
        deadline = time.time() + 90
        while time.time() < deadline and len(answers) < len(urls):
            for u in urls:
                m = re.search(r"ECCE_TEST_ORGANIZER: summary %s: (.*)" % re.escape(u),
                              open(logpath, errors="replace").read())
                if m:
                    answers[u] = m.group(1)
            time.sleep(0.5)
    finally:
        stop(org)
    return answers


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--png", help="save the Organizer's summaries here")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    if not shutil.which("Xvfb"):
        harness.skip("Xvfb is not installed")
    harness.prerequisites(build, ())
    for exe in GUI:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    s = harness.Session(build, "mdeditors", {}, local=True, keep=args.keep)
    for exe in GUI:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    gw = os.path.join(REPO, "packaging", "gateway")
    for name in os.listdir(gw):
        if name.startswith("ecce-"):
            harness.link(os.path.join(gw, name), os.path.join(s.home, "bin", name))
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
        display = xdisplay.Display().__enter__()
        cmddir = os.path.join(s.state, "mded")
        os.makedirs(cmddir)
        genv = s.env({"DISPLAY": display.name, "ECCE_TEST_MDED": cmddir,
                      "PATH": wrappers + os.pathsep + s.env()["PATH"]})

        opts = {}
        for code, mode in (("GROMACS", "gromacsstudy"), ("NWChem", "nwchemmdstudy")):
            rc, out = s.driver(mode, s.userUrl(), code.lower())
            urls = [l for l in out.splitlines() if l.startswith("file://")]
            opt = [u for u in urls if u.rstrip("/").endswith("/optimize")]
            if not check(rc == 0 and len(opt) == 1, "a %s MD study was made: %s"
                         % (code, out.strip().replace("\n", " | ")[:200])):
                return 1
            opts[code] = opt[0].rstrip("/") + "/"

        for code, url in opts.items():
            ed = Editor(s, display, genv, wrappers, cmddir, "mdoptimize", "MDOptimize")
            try:
                if not ed.up():
                    continue
                check(ed.cmd("open " + url) == "ok", "%s Optimize opens" % code)
                title = ed.cmd("title")
                check(title == "ECCE %s MD Optimize" % code, "the title: %s" % title)
                tabs = ed.cmd("tabs").split("|")
                check(("Control" in tabs) == (code == "NWChem") and "Optimize" in tabs,
                      "%s pages: %s" % (code, tabs))
                check(ed.proc.poll() is None, "mdoptimize is still up after the %s task" % code)
            finally:
                ed.close()

        answers = summary(s, display, genv, wrappers, list(opts.values()), args.png)
        for code, url in opts.items():
            out = answers.get(url, "")
            tools = re.findall(r"tool=(.*?)(?= tool=|$)", out)
            check(("%s MD Optimize" % code) in tools,
                  "the %s Optimize summary names its editor: %s" % (code, tools))
            check(tools and not any(t.endswith(":clipped") for t in tools),
                  "no %s summary button clips its label" % code)
    finally:
        if display is not None:
            display.__exit__(None, None, None)
        s.services(False)
    say("FAIL (%d): %s" % (len(failures), "; ".join(failures)) if failures else "PASS")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
