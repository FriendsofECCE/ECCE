#!/usr/bin/env python3
"""Drive the Builder's element chooser headless: own Xvfb, isolated HOME.

    chooser_drive.py BUILDER_BINARY OUTDIR [dark]

Prints the BUILDER lines the chooser hooks write (pick, search, Enter) and
leaves the popup screenshots (initial, search, final) as PNGs in OUTDIR.
Exit status 1 when a pick, a search or the Enter key does not do its job.
"""
import os
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "tests", "apps"))
import xdisplay  # noqa: E402

SCRIPT = ["wait 3000", "chooser shot initial",
          "chooser search iron", "chooser shot search",
          "chooser enter iron", "chooser pick O", "chooser pick Cl",
          "chooser shot final", "chooser close", "quit"]


def main():
    binary, out = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2])
    dark = len(sys.argv) > 3 and sys.argv[3] == "dark"
    os.makedirs(out, exist_ok=True)
    home = os.path.join(out, "home")
    os.makedirs(home, exist_ok=True)
    script = os.path.join(out, "chooser.script")
    with open(script, "w") as f:
        f.write("\n".join(SCRIPT) + "\n")
    import apps
    import fixture
    import isolate
    for var in ("ECCE_DATASERVER_PORT", "ECCE_BROKER_PORT"):
        os.environ.pop(var, None)
    os.environ["ECCE_REALUSER"] = fixture.USER
    settings = isolate.apply(apps.INSTALL)
    isolate.killLeftovers(settings["ECCE_REALUSERHOME"])
    os.environ["ECCE_NO_REAP"] = "1"
    xdisplay.SCREEN = "1400x1000x24"
    log = ""
    with xdisplay.Display() as display:
        svc = []
        apps.startServices(display, svc)
        try:
            fixture.ensureRealUserAccount()
            env = display.env()
            env.update(ECCE_BUILDER_SCRIPT=script, ECCE_FEEDBACK_STDERR="1",
                       ECCE_TEST_XYZ_UNITS="angstrom")
            if dark:
                env["GTK_THEME"] = "Adwaita:dark"
            structure = os.path.join(ROOT, "tests", "fragreaders", "data",
                                     "benzene.car")
            #  The wrapper starts $ECCE_HOME/bin/builder: a copy of the
            #  isolated home whose builder is the binary under test.
            farm = os.path.join(out, "ecce-home")
            real = os.path.realpath(env["ECCE_HOME"])
            shutil.rmtree(farm, ignore_errors=True)
            os.makedirs(os.path.join(farm, "bin"))
            for name in os.listdir(env["ECCE_HOME"]):
                if name != "bin":
                    os.symlink(os.path.join(env["ECCE_HOME"], name),
                               os.path.join(farm, name))
            for name in os.listdir(os.path.join(env["ECCE_HOME"], "bin")):
                if name != "builder":
                    os.symlink(os.path.join(env["ECCE_HOME"], "bin", name),
                               os.path.join(farm, "bin", name))
            os.symlink(binary, os.path.join(farm, "bin", "builder"))
            env["ECCE_HOME"] = farm
            print("binary:", binary)
            try:
                proc = subprocess.run(["/usr/bin/ecce-builder", structure], env=env,
                                      timeout=120, stdout=subprocess.PIPE,
                                      stderr=subprocess.STDOUT)
                log = proc.stdout.decode("utf-8", "replace")
            except subprocess.TimeoutExpired as e:
                log = (e.stdout or b"").decode("utf-8", "replace")
                log += "\nTIMEOUT"
        finally:
            apps.stopServices(display)
    with open(os.path.join(out, "builder.log"), "w") as f:
        f.write(log)
    lines = [l for l in log.splitlines() if "chooser" in l]
    print("\n".join(lines))
    want = ["chooser matches=1 element=", "chooser pick O -> 1",
            "chooser element=Cl"]
    bad = [w for w in want if not any(w in l for l in lines)]
    # Enter on "iron" must have picked Fe: the matches line after it says Fe.
    if not any("matches=1 element=Fe" in l for l in lines):
        bad.append("Enter on 'iron' set element=Fe")
    for b in bad:
        print("FAIL:", b)
    if bad:
        print(log[-3000:])
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
