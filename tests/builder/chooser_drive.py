#!/usr/bin/env python3
"""Drive the Builder's element chooser headless: own Xvfb, isolated HOME.

    chooser_drive.py BUILDER_BINARY OUTDIR

Three Builder starts on one fixture calculation, services and HOME:
  1. ECCE_NEW_PERTABLE=1: search "iron", Enter (must make Fe the Builder's
     element), picks, the Build panel's table button, screenshots.
  2. ECCE_NEW_PERTABLE=1 again: the recent row must have survived the restart.
  3. variable unset: the Builder must still open the old PerTabPanel popup.
BINARY is put into a copy of the isolated ECCE_HOME so the run cannot fall
back to /opt/ecce; the test prints the binary it ran.  Exit 77: no install.
"""
import os
import shutil
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tests", "apps"))

import apps         # noqa: E402
import fixture      # noqa: E402
import isolate      # noqa: E402
import pbc_ux_test  # noqa: E402
import xdisplay     # noqa: E402

STRUCTURE = os.path.join(ROOT, "tests", "fragreaders", "data", "benzene.car")
NEW_RUN = ["chooser kind", "chooser shot initial",
           "chooser search iron", "chooser shot search",
           "chooser enter iron", "chooser pick O", "chooser pick Cl",
           "chooser screen window", "chooser shot final", "chooser close",
           "pertab O"]
RECENT_RUN = ["chooser kind", "chooser close"]
OLD_RUN = ["chooser kind", "chooser pick N", "chooser close", "pertab O"]


def lines(log, key):
    return [l.split("BUILDER: ", 1)[1].strip() for l in log.splitlines()
            if "BUILDER: " + key in l]


def farm(out, binary):
    """An ECCE_HOME like the isolated one whose builder is BINARY."""
    home = os.environ["ECCE_HOME"]
    path = os.path.join(out, "ecce-home")
    shutil.rmtree(path, ignore_errors=True)
    os.makedirs(os.path.join(path, "bin"))
    for name in os.listdir(home):
        if name != "bin":
            os.symlink(os.path.join(home, name), os.path.join(path, name))
    for name in os.listdir(os.path.join(home, "bin")):
        if name != "builder":
            os.symlink(os.path.join(home, "bin", name),
                       os.path.join(path, "bin", name))
    os.symlink(binary, os.path.join(path, "bin", "builder"))
    return path


def start(display, out, name, url, steps, env):
    d = os.path.join(out, name)
    shutil.rmtree(d, ignore_errors=True)
    os.makedirs(d)
    script = os.path.join(d, "chooser.script")
    with open(script, "w") as f:
        f.write("\n".join(["wait 3000"] + steps + ["quit"]) + "\n")
    auth = fixture.authFile(os.path.join(fixture.stateHome(), ".ECCE",
                                         "auth.pipe"), user=fixture.USER)
    environment = {"ECCE_TEST_XYZ_UNITS": "angstrom",
                   "ECCE_BUILDER_SCRIPT": script, "ECCE_FEEDBACK_STDERR": "1",
                   "ECCE_REALUSER": fixture.USER}
    environment.update(env)
    result = apps.run(display, "builder", args=(STRUCTURE,),
                      windowTimeout=120, settle=90, env=environment)
    log = result.log or ""
    with open(os.path.join(d, "builder.log"), "w") as f:
        f.write(log)
    problems = []
    if result.crashed:
        problems.append("%s: builder CRASHED (%s)" % (name, result.signalName))
    if "ECCE_BUILDER_SCRIPT: quit" not in log:
        problems.append("%s: script did not reach quit" % name)
    problems += ["%s: %s" % (name, l) for l in log.splitlines()
                 if "ECCE_BUILDER_SCRIPT" in l and ": FAIL" in l]
    return log, problems


def main():
    binary, out = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2])
    if not apps.installed("builder"):
        print("SKIP  no installed builder")
        return 77
    os.makedirs(out, exist_ok=True)
    for var in ("ECCE_DATASERVER_PORT", "ECCE_BROKER_PORT", "ECCE_NEW_PERTABLE"):
        os.environ.pop(var, None)
    os.environ["ECCE_REALUSER"] = fixture.USER
    settings = isolate.apply(apps.INSTALL)
    isolate.killLeftovers(settings["ECCE_REALUSERHOME"])
    os.environ["ECCE_NO_REAP"] = "1"
    xdisplay.SCREEN = "1400x1000x24"
    problems = []
    with xdisplay.Display() as display:
        svc = []
        apps.startServices(display, svc)
        try:
            if not all(apps.serviceState(display).values()):
                print("FAIL  services did not come up: %s" % svc)
                return 1
            fixture.ensureRealUserAccount()
            url, error = pbc_ux_test.install("chooser-unrun", pbc_ux_test.notRun)
            if error:
                print("FAIL  cannot install fixture: %s" % error)
                return 1
            restore = fixture.settleUpgradeNotices()
            home = farm(out, binary)
            print("binary under test:", binary, "(%d bytes)" % os.path.getsize(binary))
            new = {"ECCE_HOME": home, "ECCE_NEW_PERTABLE": "1"}

            log, p = start(display, out, "new", url, NEW_RUN, new)
            problems += p
            got = lines(log, "chooser")
            print("\n".join(got + lines(log, "pertab")))
            elements = [l for l in got if "element=" in l and "kind" not in l]
            if not any("kind=new" in l for l in got):
                problems.append("new: the chooser is not the new one")
            if "chooser matches=1 element=Fe" not in got and \
               not any(l.startswith("chooser matches=1 element=Fe") for l in got):
                problems.append("new: Enter on 'iron' did not make Fe current")
            for want in ("chooser pick O -> 1", "chooser pick Cl -> 1"):
                if want not in got:
                    problems.append("new: missing %r" % want)
            if "chooser element=Cl" not in got:
                problems.append("new: current element after pick Cl is not Cl")
            if "pertab element=O" not in lines(log, "pertab"):
                problems.append("new: the Build panel's table did not pick O")
            for png in ("initial", "search", "final", "window"):
                if not os.path.exists(os.path.join(out, "new", png + ".png")):
                    problems.append("new: no %s.png" % png)

            log, p = start(display, out, "recent", url, RECENT_RUN, new)
            problems += p
            kind = [l for l in lines(log, "chooser kind")]
            print("\n".join(kind))
            if not kind or not all(e in kind[0] for e in ("Fe", "O", "Cl")):
                problems.append("recent: Fe, O, Cl not read back after restart")

            log, p = start(display, out, "old", url, OLD_RUN, {"ECCE_HOME": home})
            problems += p
            got = lines(log, "chooser") + lines(log, "pertab")
            print("\n".join(got))
            if not any("kind=old" in l for l in got):
                problems.append("old: default is not the old popup")
            if "pertab element=O" not in got:
                problems.append("old: the Build panel's table did not pick O")
        finally:
            try:
                restore()
            except NameError:
                pass
            apps.stopServices(display)
    for p in problems:
        print("FAIL  " + p)
    if not problems:
        print("PASS  element chooser")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
