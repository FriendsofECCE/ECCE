#!/usr/bin/env python3
"""
Details dialogs show no internal setting keys as tooltips.

Opens the Gaussian 16 Theory and Runtype dialogs on a private Xvfb, walks
every child window and fails on a tooltip of the form XX.Section.Name (the
saved-settings key, e.g. ES.Theory.UseCartesian).  Every scripts/codereg
dialog shares templates.py, so one code covers the wrappers.  Needs wxPython
and Xvfb; exit 77 (skip) without.  No ECCE install, services or real HOME.
"""

import os
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
CODEREG = os.path.join(os.path.dirname(os.path.dirname(HERE)),
                       "scripts", "codereg")
DIALOGS = ["ged16theory.py", "ged16runtype.py"]

#  Runs inside the dialog's process: stands in for Finalize's MainLoop.
DRIVER = r'''
import sys, os, re
script, report = sys.argv[1], sys.argv[2]
sys.argv = [script, os.devnull, "0", "GUIValues", "Writable", "DebugOff",
            "DFT", "B3LYP", "Energy", "t", "0", "C1", "10", "1", "1", "5",
            "2", "3"]
import wx
import templates
KEY = re.compile(r"^[A-Z]{2}\.[A-Za-z]+\.")

def walk(win):
    yield win
    for c in win.GetChildren():
        yield from walk(c)

def finalize(self):
    self.Show(True)
    for _ in range(10):
        wx.GetApp().Yield(True)
        wx.MilliSleep(30)
    total = tips = 0
    out = open(report, "w")
    for w in walk(self):
        total += 1
        tip = w.GetToolTipText()
        if tip:
            tips += 1
            if KEY.match(tip):
                out.write("KEY %s\n" % tip)
    out.write("windows %d\ntooltips %d\n" % (total, tips))
    out.close()
    os._exit(0)

templates.EcceFrame.Finalize = finalize
exec(compile(open(script).read(), script, "exec"))
'''


def main():
    if not shutil.which("Xvfb") or not os.path.isdir(CODEREG):
        return 77
    try:
        import wx  # noqa: F401
    except ImportError:
        return 77
    scratch = tempfile.mkdtemp(prefix="details-tooltip-")
    driver = os.path.join(scratch, "driver.py")
    open(driver, "w").write(DRIVER)
    xv = subprocess.Popen(["Xvfb", ":96", "-screen", "0", "1280x1024x24",
                           "-nolisten", "tcp"], stderr=subprocess.DEVNULL)
    failed = []
    try:
        time.sleep(1.5)
        env = dict(os.environ, DISPLAY=":96", GDK_BACKEND="x11",
                   PYTHONPATH=CODEREG, PYTHONDONTWRITEBYTECODE="1",
                   HOME=scratch)
        env.pop("WAYLAND_DISPLAY", None)
        for name in DIALOGS:
            report = os.path.join(scratch, name + ".txt")
            subprocess.run([sys.executable, driver,
                            os.path.join(CODEREG, name), report],
                           env=env, cwd=CODEREG, timeout=90,
                           stdout=subprocess.DEVNULL,
                           stderr=subprocess.DEVNULL)
            try:
                lines = open(report).read().splitlines()
            except OSError:
                failed.append("%s: dialog did not report" % name)
                continue
            failed += ["%s: tooltip %s" % (name, l[4:])
                       for l in lines if l.startswith("KEY ")]
            print("%s: %s" % (name, ", ".join(
                l for l in lines if not l.startswith("KEY "))))
    finally:
        xv.terminate()
        shutil.rmtree(scratch, ignore_errors=True)
    for f in failed:
        print("FAIL " + f)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
