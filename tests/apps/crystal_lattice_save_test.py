#!/usr/bin/env python3
"""
A lattice made in the Builder reaches a crystal code's calculation.

New CP2K (and Quantum ESPRESSO) calculation, no cell: the Calculation Editor
says "Not ready: this code needs a periodic unit cell".  In the Builder the
Periodic Builder's Create lattice must make Save available (File > Save,
Ctrl+S), saving must store the cell, the editor must then be ready on that
point, and the Builder must show the cell again on reopening.  Also: making
the editor window taller must give the extra height to the log pane.

    tests/apps/crystal_lattice_save_test.py [--case CP2K|QuantumESPRESSO]

Local data mode, scripted through ECCE_TEST_ORGANIZER, ECCE_BUILDER_SCRIPT
and ECCE_CALCED_SCRIPT (no synthetic input).  Same installed tree, isolation
and services as run_tests.py.
"""

import os
import re
import shutil
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402
import xdisplay   # noqa: E402
import qe_walkthrough_test as q   # noqa: E402

CODES = ["CP2K", "QuantumESPRESSO"]
NOT_READY = "this code needs a periodic unit cell"
failures = []


def check(cond, what, detail=""):
    print("  %s %s" % ("ok  " if cond else "FAIL", what), flush=True)
    if not cond:
        failures.append(what + ("\n" + detail if detail else ""))


def run(display, wrapper, var, script, url, authArgs, tmp):
    path = os.path.join(tmp, "%s.script" % wrapper)
    with open(path, "w") as handle:
        handle.write("\n".join(script) + "\n")
    result = apps.run(display, wrapper.replace("ecce-", ""),
                      args=authArgs + ("-context", url), windowTimeout=60,
                      settle=120, env={var: path})
    return result.log or ""


def walk(display, tmp, org, code):
    project, name = "latt-" + code.lower(), "w"
    url = "%s/%s/%s" % (q.userUrl(), project, name)
    print(org.send("newproject " + project))
    print(org.send("newcalc %s %s %s" % (project, name, code)))
    a = q.auth(tmp)

    log = run(display, "ecce-calced", "ECCE_CALCED_SCRIPT",
              ["wait 3000", "ready", "info", "quit"], url, a, tmp)
    check("CALCED: notready=Not ready" in log,
          code + ": a new calculation is not ready", run_tests._tail(log, 15))

    log = run(display, "ecce-builder", "ECCE_BUILDER_SCRIPT",
              ["wait 4000", "add O Bent 0 0 0", "wait 1000", "cmd addh",
               "wait 1500", "info", "savestate", "pbc create", "wait 1000",
               "savestate", "savemenu", "wait 3000", "savestate", "info",
               "quit"], url, a, tmp)
    states = re.findall(r"BUILDER: dirty=(\d) save_enabled=(\d) "
                        r"toolbar_save_enabled=(\d)", log)
    check(len(states) == 3 and states[1] == ("1", "1", "1"),
          code + ": Save is enabled after Create lattice (%s)" % states,
          run_tests._tail(log, 30))
    check(len(states) == 3 and states[2][0] == "0",
          code + ": File > Save saved the structure (%s)" % states)
    check("lattice=yes" in log.split("savestate")[-1] or
          log.count("lattice=yes") >= 1,
          code + ": the Builder has the lattice")

    log = run(display, "ecce-calced", "ECCE_CALCED_SCRIPT",
              ["wait 3000", "ready", "info", "quit"], url, a, tmp)
    check(NOT_READY not in log, code + ": the editor accepts the cell",
          run_tests._tail(log, 15))

    log = run(display, "ecce-builder", "ECCE_BUILDER_SCRIPT",
              ["wait 4000", "info", "quit"], url, a, tmp)
    check("lattice=yes" in log, code + ": reopened, the Builder has the cell",
          run_tests._tail(log, 15))

    if code == "CP2K":
        log = run(display, "ecce-calced", "ECCE_CALCED_SCRIPT",
                  ["wait 3000", "rects", "grow 300", "wait 1500", "rects",
                   "quit"], url, a, tmp)
        logs = [int(v) for v in re.findall(r"frame=\S+ log=(-?\d+)", log)]
        check(len(logs) == 2 and logs[1] - logs[0] >= 250,
              "taller editor window: the log grows (%s)" % logs,
              run_tests._tail(log, 15))


def main():
    options = [a for a in sys.argv[1:] if a != "--case"]
    codes = CODES
    if "--case" in sys.argv:
        codes = [sys.argv[sys.argv.index("--case") + 1]]
        sys.argv = [x for x in sys.argv if x not in ("--case", codes[0])]
    q.LOCAL.append(tempfile.mkdtemp(prefix="latt-localdata-"))
    os.environ["ECCE_LOCAL_DATA"] = q.LOCAL[0]

    def checkApp(display, name, results, verbose=False):
        tmp = tempfile.mkdtemp(prefix="latt-")
        config = os.path.join(fixture.stateHome(), ".ECCE", "CONFIG.localhost")
        shutil.copy(os.path.join(apps.INSTALL, "siteconfig", "CONFIG-Examples",
                                 "CONFIG.localhost"), config)
        org = q.Organizer(display, tmp, q.auth(tmp))
        try:
            time.sleep(25)
            for code in codes:
                walk(display, tmp, org, code)
        finally:
            org.stop()
        if failures:
            results.fail("crystal-lattice-save", "; ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
