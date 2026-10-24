#!/usr/bin/env python3
"""
The Machine Browser's "Machine settings..." button (#234).

    browser_test.py --build <build dir> [--png <file>]

The browser's ECCE_MACHBROWSER_SHOT hook selects the first machine, clicks the
button, saves the window to a PNG and exits; with the hook set the browser
neither subscribes nor publishes, and the button prints the request it would
send to the gateway.  Checks that the button is enabled with a machine
selected and asks for Register Machines on that machine.
SKIPs (77) without Xvfb or the binary.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import gui_test as g   # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--build", required=True)
    ap.add_argument("--png")
    a = ap.parse_args()
    build = os.path.abspath(a.build)
    if not os.access(os.path.join(build, "machbrowser"), os.X_OK):
        print("SKIP  no machbrowser in " + build)
        return 77
    try:
        from xdisplay import Display
        disp = Display()
        disp.__enter__()
    except Exception as exc:
        print("SKIP  " + str(exc))
        return 77
    tmp = tempfile.mkdtemp(prefix="ecce-machbrowser-")
    try:
        e = g.Env(tmp, "browser")
        png = os.path.abspath(a.png) if a.png else os.path.join(tmp, "b.png")
        env = e.env(disp)
        env.pop("ECCE_MACHREG_SCRIPT", None)
        env["ECCE_REALUSER"] = "eccetest"
        env["ECCE_MACHBROWSER_SHOT"] = png
        p = subprocess.run([os.path.join(build, "machbrowser")], env=env,
                           cwd=e.root, stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT, text=True, timeout=120)
        m = re.search(r"\[MACHBROWSER\] request: appname=MachineRegister "
                      r"initmachine=(\S+)", p.stdout)
        g.check(m is not None,
                "the button asks for Register Machines on the selected "
                "machine" + ("" if m else "\n" + p.stdout[-1500:]))
        g.check("FAIL" not in p.stdout, "the button is enabled")
        g.check(os.path.isfile(png) and os.path.getsize(png) > 2000,
                "the window was photographed")
        #  The Allocation query (#494): the queue manager's command, none for
        #  a machine without one, and the machine's own CONFIG key over both.
        def alloc(machine):
            env["ECCE_MACHBROWSER_MACHINE"] = machine
            q = subprocess.run([os.path.join(build, "machbrowser")], env=env,
                               cwd=e.root, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT, text=True, timeout=120)
            m = re.search(r"\[MACHBROWSER\] allocation: enabled=(\d) "
                          r"command=(.*)", q.stdout)
            return (m.group(1), m.group(2)) if m else (None, q.stdout[-800:])
        g.check(alloc("cluster")[1] == "sshare -l -U -u $USER",
                "a Slurm machine defaults to sshare: %r" % (alloc("cluster"),))
        g.check(alloc("mine")[1] == "" and alloc("mine")[0] == "0",
                "a machine without a command has the mode greyed out: %r"
                % (alloc("mine"),))
        with open(os.path.join(e.ue, "CONFIG.cluster"), "w") as f:
            f.write("allocationCommand: projinfo\n")
        g.check(alloc("cluster")[1] == "projinfo",
                "the machine's allocationCommand replaces the default: %r"
                % (alloc("cluster"),))
    finally:
        disp.__exit__(None, None, None)
        shutil.rmtree(tmp, ignore_errors=True)
    print("%d passed, %d failed" % (g.passed, g.failed))
    return 1 if g.failed else 0


if __name__ == "__main__":
    sys.exit(main())
