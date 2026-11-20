#!/usr/bin/env python3
"""The first Viewer of a session, opened while the examples import, ends
its start feedback and leaves every window enabled (hand test 11-20: the
"Starting the Viewer" window stayed and the IRC Path panel was greyed).

One process (tab mode).  Each round is a fresh data directory whose first
start imports the examples; the Viewer on f_etcl_sn2_irc is requested at
once, during the import, and once more in a round without the import.
"""
import os
import shutil
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import run_tests  # noqa: E402
import progress_box_test as pb  # noqa: E402

failures = []
ROUNDS = int(os.environ.get("ECCE_FIRST_OPEN_ROUNDS", "3"))


def check(ok, what, detail=""):
    print("%s %s%s" % ("ok   " if ok else "FAIL ", what,
                       ("\n      " + detail) if (detail and not ok) else ""))
    if not ok:
        failures.append(what)
    return ok


def run(display, label, importExamples, startDelay):
    data = tempfile.mkdtemp(prefix="ecce-firstopen-data-")
    work = tempfile.mkdtemp(prefix="ecce-firstopen-")
    try:
        project = os.path.join(data, "users", "local", "firstopen")
        os.makedirs(project)
        shutil.copy(pb.PROJECT_META, os.path.join(project, ".ecce-meta"))
        calc = os.path.join(project, "f_etcl_sn2_irc")
        shutil.copytree(os.path.join(
            HERE, "..", "..", "data", "client", "examples", "organic",
            "substitution-vs-elimination", "ecceqm-f_etcl_sn2_irc", "stored",
            "f_etcl_sn2_irc"), calc, symlinks=True)
        url = "file://" + calc
        cmds = os.path.join(work, "commands")
        open(cmds, "w").close()

        def append(line, wait):
            with open(cmds, "a") as handle:
                handle.write(line + "\n")
            time.sleep(wait)

        def inspect(_display):
            time.sleep(startDelay)
            append("start CalculationViewer " + url, 40)
            append("toolstate", 2)
            append("inprocess", 2)
            append("tabs", 2)
            append("toolstate", 30)
            append("inprocess", 2)
            append("menu exit", 3)
            append("answer ok", 20)

        env = {"ECCE_ONE_PROCESS": "1", "ECCE_TEST_ORGANIZER": cmds,
               "ECCE_LOCAL_DATA": data, "ECCE_TEST_MESSAGES": "1"}
        if importExamples:
            env["ECCE_NO_EXAMPLE_IMPORT"] = ""
        result = apps.run(display, "organizer", windowTimeout=60, settle=5,
                          env=env, inspect=inspect)
        log = result.log or ""
        states = pb.answers(log, "toolstate")
        inproc = pb.answers(log, "inprocess")
        print(label, "toolstate:", states)
        print(label, "inprocess:", inproc)
        print(label, "tabs:", pb.answers(log, "tabs"))
        check(not result.crashed, label + ": no crash",
              result.signalName or log[-1500:])
        check(len(states) == 2 and all("window=none" in s and "busy=0" in s
                                       for s in states),
              label + ": the start window closed and the busy cursor ended",
              str(states))
        check(len(inproc) == 2 and not any("disabled" in i for i in inproc),
              label + ": no window is left disabled", str(inproc))
        if failures:
            print("\n".join(l for l in log.splitlines()
                            if "Organizer:" in l or "rogress" in l)[-6000:])
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        for i in range(ROUNDS):
            run(display, "import round %d" % (i + 1), True, 0.5 * i)
        run(display, "no import", False, 0)
        if failures:
            results.fail("viewer-first-open", "; ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
