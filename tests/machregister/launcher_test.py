#!/usr/bin/env python3
"""
The Launcher's "Machine settings..." button, driven through its real window
under Xvfb by the Launcher's test hook (ECCE_LAUNCHER_SCRIPT, see
src/apps/launcher/WxLauncherScript.H).

    launcher_test.py --build <build dir> [--pngs <dir>]

Checks that the button asks for Register Machines on the machine chosen in the
Launcher (the hook records the request instead of sending it to the gateway),
and that after a real Register Machines run saves queue changes the Launcher,
told the way the "ecce_machreg_changed" message tells it, shows them.
SKIPs (77) without Xvfb or the binaries.
"""

import argparse
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import gui_test as g   # noqa: E402  (Env, check, write, read ...)


def run_launcher(display, build, env, script, timeout=120):
    g.write(os.path.join(env.root, "lscript"), script)
    e = env.env(display)
    e.pop("ECCE_MACHREG_SCRIPT", None)
    e["ECCE_REALUSER"] = "eccetest"
    e["ECCE_LAUNCHER_SCRIPT"] =os.path.join(env.root, "lscript")
    return subprocess.run([os.path.join(build, "launcher")], env=e,
                          cwd=env.root, stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT, text=True, timeout=timeout)


def clean(p, what):
    fails = [l for l in p.stdout.splitlines() if "FAIL" in l]
    done = "script done failures=0" in p.stdout
    if fails or not done:
        print(p.stdout[-3000:])
    g.check(not fails and done, what + (": " + "; ".join(fails[:3]) if fails
            else "" if done else ": did not finish\n" + p.stdout[-1200:]))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--build", required=True)
    ap.add_argument("--pngs")
    a = ap.parse_args()
    build = os.path.abspath(a.build)
    for b in ("launcher", "machregister"):
        if not os.access(os.path.join(build, b), os.X_OK):
            print("SKIP  no %s in %s" % (b, build))
            return 77
    try:
        from xdisplay import Display
        disp = Display()
        disp.__enter__()
    except Exception as exc:
        print("SKIP  " + str(exc))
        return 77
    tmp = tempfile.mkdtemp(prefix="ecce-launcher-")
    try:
        e = g.Env(tmp, "launcher")
        # What Register Machines does when it saves: a real run of the real
        # window, with its own hook.
        g.write(os.path.join(e.root, "mscript"), """
select cluster
tab queues
set q-name gpu
set q-maxprocs 8
queue-apply
set queue short
set q-defprocs 4
queue-apply
save
expect dirty 0
quit
""")
        save = ("ECCE_MACHREG_SCRIPT=%s %s" % (os.path.join(e.root, "mscript"),
                os.path.join(build, "machregister")))
        pngs = a.pngs
        if pngs:
            os.makedirs(pngs, exist_ok=True)
        shot1 = ("shot %s/launcher-machine-settings.png" % pngs) if pngs else ""
        shot2 = ("shot %s/launcher-after-save.png" % pngs) if pngs else ""
        p = run_launcher(disp, build, e, """
expect button 1
machine cluster
expect machine cluster
click machine-settings
expect request 'appname=MachineRegister initmachine=cluster'
machine mine
click machine-settings
expect request 'appname=MachineRegister initmachine=mine'
machine cluster
%(shot1)s
expect queues long,short
queue long
expect queue long
expect procsmax 256
exec "%(save)s"
reload
expect machine cluster
expect queues gpu,long,short
queue gpu
expect procsmax 8
%(shot2)s
quit
""" % {"save": save, "shot1": shot1, "shot2": shot2})
        clean(p, "the button names the chosen machine; the Launcher reloads "
                 "queues and defaults after a save")
        g.check("binary=%s" % os.path.join(build, "launcher") in p.stdout,
                "the hook reports the binary under test")
        q = g.read(os.path.join(e.ue, "cluster.Q"))
        g.check("gpu" in q and "short|defProcessors:" in q,
                "the save really went through Register Machines")
    finally:
        disp.__exit__(None, None, None)
        shutil.rmtree(tmp, ignore_errors=True)
    print("%d passed, %d failed" % (g.passed, g.failed))
    return 1 if g.failed else 0


if __name__ == "__main__":
    sys.exit(main())
