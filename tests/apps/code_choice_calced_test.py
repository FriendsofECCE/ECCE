#!/usr/bin/env python3
"""The Calculation Editor's code bar ends with "+N hidden" (#1049).

The hook ECCE_TEST_CALCED unticks codes as Preferences > Codes does and lists
the bar.  Same isolation as run_tests.py.  Exit 77 (skip) without Xvfb.
"""
import os
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import isolate    # noqa: E402
import xdisplay   # noqa: E402

failures = []


def check(ok, what, detail=""):
    print("%s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
        print("      | " + detail[-400:])


def main():
    if not shutil.which("Xvfb") or not os.path.exists(
            os.path.join(apps.WRAPPERS, "ecce-calced")):
        print("SKIP: no Xvfb or ecce-calced")
        return 77
    try:
        isolate.apply(apps.INSTALL)
    except isolate.IsolationError as exc:
        print("refusing to run: %s" % exc)
        return 1
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    os.environ.pop("DISPLAY", None)
    work = tempfile.mkdtemp(prefix="codechoice-calced-", dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    project = os.path.join(data, "users", "local", "qm")
    os.makedirs(project)
    shutil.copy(os.path.join(REPO, "tools", "screenshots", "data", "project.ecce-meta"),
                project + "/.ecce-meta")
    calc = os.path.join(project, "o2")
    shutil.copytree(os.path.join(REPO, "tests", "ecceqm", "fixtures", "o2-roks"),
                    calc, symlinks=True)
    meta = os.path.join(calc, ".ecce-meta")
    text = open(meta).read().replace("ecce:state\t\tComplete", "ecce:state\t\tReady")
    open(meta, "w").write(text)
    os.environ["ECCE_LOCAL_DATA"] = data
    steps = ["codeshown ORCA on", "codeshown MOPAC on", "codeshown NWChem on",
             "codeshown CREST on", "codebar",
             "codeshown MOPAC off", "codeshown NWChem off", "codeshown CREST off",
             "codebar",
             "codeshown MOPAC on", "codebar"]
    cmds = os.path.join(work, "commands")
    open(cmds, "w").write("")
    with xdisplay.Display() as display:
        subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")],
                       env=display.env(), timeout=180)

        def drive(_d):
            time.sleep(15)
            open(cmds, "a").write("".join(s + "\n" for s in steps))
            time.sleep(15)

        try:
            result = apps.run(display, "calced", args=("-context", "file://" + calc + "/"),
                              windowTimeout=60, settle=5, inspect=drive,
                              env={"ECCE_TEST_CALCED": cmds})
        finally:
            subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-stop")],
                           env=display.env(), timeout=120)
    log = result.log or ""
    got = [l.split("ECCE_TEST_CALCED: ", 1)[1] for l in log.splitlines()
           if "ECCE_TEST_CALCED: codebar: " in l]
    check(len(got) == 3, "three code bar reports", log[-600:])
    if len(got) == 3:
        n = lambda s: s.split("more=")[-1].strip() if "more=" in s else "none"
        check(n(got[0]) in ("none",) or n(got[0]).startswith("+"),
              "all four ticked: bar report", got[0])
        check("code=NWChem" not in got[1] and "code=MOPAC" not in got[1] and
              "code=CREST" not in got[1], "unticked codes have no button", got[1])
        check(n(got[1]).endswith("hidden") and n(got[1]).startswith("+"),
              "bar ends with '+N hidden'", got[1])
        check(int(n(got[2]).split()[0][1:]) == int(n(got[1]).split()[0][1:]) - 1,
              "ticking one code lowers the count by one without a restart",
              got[1] + " || " + got[2])
        check("code=MOPAC" in got[2], "the ticked code has its button", got[2])
    shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
