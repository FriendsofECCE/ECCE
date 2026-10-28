#!/usr/bin/env python3
"""
Theory/Runtype Details dialogs stay usable on a short screen.

Every scripts/codereg dialog is built on EcceFrame/EccePanel, so this opens
the Gaussian 16 Theory and Runtype dialogs on a private Xvfb of 1024x500 and
checks that the frame is resizable, opens inside the screen, and that after
being resized to 800x400 and to its minimum the Close and Reset buttons still
lie inside the frame.  Needs wxPython and Xvfb; exit 77 (skip) without.
Self-contained: no ECCE install, no services, no HOME access.

    details_resize_test.py
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
SCREEN = (1024, 500)
DIALOGS = ["ged16theory.py", "ged16runtype.py"]

#  Runs inside the dialog's process: stands in for Finalize's MainLoop.
DRIVER = r'''
import sys, os
script, report = sys.argv[1], sys.argv[2]
sys.argv = [script, os.devnull, "0", "GUIValues", "Writable", "DebugOff",
            "DFT", "B3LYP", "Energy", "t", "0", "C1", "10", "1", "1", "5",
            "2", "3"]
import wx
import templates
out = open(report, "w")

def inside(frame, win):
    r = win.GetScreenRect()
    c = frame.GetClientRect()
    o = frame.ClientToScreen(c.GetTopLeft())
    return (r.x >= o.x and r.y >= o.y and r.x + r.width <= o.x + c.width
            and r.y + r.height <= o.y + c.height)

def settle():
    for _ in range(20):
        wx.GetApp().Yield(True)
        wx.MilliSleep(30)

def finalize(self):
    self.Fit() if not hasattr(self, "TargetSize") else \
        self.SetClientSize(self.TargetSize)
    self.Show(True)
    settle()
    panel = [c for c in self.GetChildren()
             if isinstance(c, templates.EccePanel)][0]
    area = wx.Display(0).GetClientArea()
    out.write("resizable %d\n" % bool(self.GetWindowStyleFlag()
                                      & wx.RESIZE_BORDER))
    out.write("fits %d\n" % (self.GetSize().height <= area.height
                             and self.GetSize().width <= area.width))
    out.write("buttons-initial %d\n" % (inside(self, panel.closeButton)
                                        and inside(self, panel.resetButton)))
    self.SetSize(800, 400)
    settle()
    out.write("buttons-800x400 %d\n" % (inside(self, panel.closeButton)
                                        and inside(self, panel.resetButton)))
    self.SetSize(self.GetMinSize())
    settle()
    out.write("buttons-min %d\n" % (inside(self, panel.closeButton)
                                    and inside(self, panel.resetButton)))
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
    scratch = tempfile.mkdtemp(prefix="details-resize-")
    driver = os.path.join(scratch, "driver.py")
    open(driver, "w").write(DRIVER)
    xv = subprocess.Popen(["Xvfb", ":97", "-screen", "0",
                           "%dx%dx24" % SCREEN, "-nolisten", "tcp"],
                          stderr=subprocess.DEVNULL)
    failed = []
    try:
        time.sleep(1.5)
        env = dict(os.environ, DISPLAY=":97", GDK_BACKEND="x11",
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
                got = dict(l.split() for l in open(report))
            except OSError:
                failed.append("%s: dialog did not report" % name)
                continue
            for key in ("resizable", "fits", "buttons-initial",
                        "buttons-800x400", "buttons-min"):
                if got.get(key) != "1":
                    failed.append("%s: %s failed" % (name, key))
            print("%s: %s" % (name, " ".join("%s=%s" % kv
                                             for kv in got.items())))
    finally:
        xv.terminate()
        shutil.rmtree(scratch, ignore_errors=True)
    for f in failed:
        print("FAIL " + f)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
