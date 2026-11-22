#!/usr/bin/env python3
"""The first Viewer of a session, opened while the examples import, ends
its start feedback and leaves every window enabled (hand test 11-20: the
"Starting the Viewer" window stayed and the IRC Path panel was greyed).

One process (tab mode).  Each round is a fresh data directory whose first
start imports the examples; the Viewer on f_etcl_sn2_irc is requested at
once, during the import, and once more in a round without the import.

Upgrade rounds (the hand case): the data holds an Examples/<Set> project of
the previous example version for every set and the preferences say that
version, so the first start re-imports all of them; the Viewer is opened
on the imported organic SN2 IRC and on the workflow's sn2_irc.  Clicks are
swallowed while the import runs (InputBlock), so the "start" hook, which
runs at the first timer tick outside an example set's import, reaches
the same moments a real click can.

During-import rounds: the Viewer request is queued at an example step, as
a message arriving then would be, so it runs inside the progress dialog's
Update() while the set (and the calculation it names) is being imported.
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
# ECCE_NO_PREPARSED: the slow first start of a new package version.
PARSE_ROUNDS = int(os.environ.get("ECCE_FIRST_OPEN_PARSE_ROUNDS", "2"))
UPGRADE_ROUNDS = int(os.environ.get("ECCE_FIRST_OPEN_UPGRADE_ROUNDS", "4"))
UPGRADE_PARSE_ROUNDS = int(os.environ.get(
    "ECCE_FIRST_OPEN_UPGRADE_PARSE_ROUNDS", "2"))
EXAMPLES = os.path.join(HERE, "..", "..", "data", "client", "examples")
META = ".\thttp://www.emsl.pnl.gov/ecce:%s\t\t%s\n"
SN2 = {"organic": "Examples/Organic_Chemistry/Substitution_vs_elimination/"
                  "f_etcl_sn2_irc",
       "workflow": "Examples/Workflows/SN2_Cl-_and_CH3Br/sn2_irc"}


def previousVersion():
    import examples_upgrade_test
    return str(int(examples_upgrade_test.exampleSetVersion()) - 1)


def seedUpgrade(data):
    """Every set as the previous version left it, emptied: all re-import."""
    top = os.path.join(data, "users", "local", "Examples")
    os.makedirs(top)
    with open(os.path.join(top, ".ecce-meta"), "w") as f:
        f.write(META % ("contenttype", "ecceProject"))
    old = previousVersion()
    for setDir in sorted(os.listdir(EXAMPLES)):
        txt = os.path.join(EXAMPLES, setDir, "set.txt")
        if not os.path.exists(txt):
            continue
        name = open(txt).readline().strip().replace(" ", "_")
        os.makedirs(os.path.join(top, name))
        with open(os.path.join(top, name, ".ecce-meta"), "w") as f:
            f.write(META % ("contenttype", "ecceProject"))
            f.write(META % ("exampleSet", old))
    return old


def check(ok, what, detail=""):
    print("%s %s%s" % ("ok   " if ok else "FAIL ", what,
                       ("\n      " + detail) if (detail and not ok) else ""))
    if not ok:
        failures.append(what)
    return ok


def run(display, label, importExamples, startDelay, parse=False,
        upgrade=None, during=None):
    data = tempfile.mkdtemp(prefix="ecce-firstopen-data-")
    work = tempfile.mkdtemp(prefix="ecce-firstopen-")
    try:
        old = None
        if upgrade:
            old = seedUpgrade(data)
            calc = os.path.join(data, "users", "local", SN2[upgrade])
        else:
            project = os.path.join(data, "users", "local", "firstopen")
            os.makedirs(project)
            shutil.copy(pb.PROJECT_META, os.path.join(project, ".ecce-meta"))
            calc = os.path.join(project, "f_etcl_sn2_irc")
            shutil.copytree(os.path.join(
                EXAMPLES, "organic", "substitution-vs-elimination",
                "ecceqm-f_etcl_sn2_irc", "stored", "f_etcl_sn2_irc"), calc,
                symlinks=True)
        url = "file://" + calc
        cmds = os.path.join(work, "commands")
        open(cmds, "w").close()

        def append(line, wait):
            with open(cmds, "a") as handle:
                handle.write(line + "\n")
            time.sleep(wait)

        def inspect(_display):
            time.sleep(startDelay)
            # Parsing every example takes minutes; the commands wait for the
            # import, so the start window is checked once it has run.
            wait = ((420 if upgrade else 300) if parse
                    else (120 if upgrade else 40))
            if during:
                time.sleep(wait)
            else:
                append("start CalculationViewer " + url, wait)
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
            # The imported version is kept in the user's preferences, not
            # the data directory: without this only the first round imports.
            prefs = os.path.join(os.environ["ECCE_REALUSERHOME"], ".ECCE",
                                 "Organizer")
            kept = []
            if os.path.exists(prefs):
                kept = [l for l in open(prefs)
                        if not l.startswith("Organizer.ExampleSet:")]
            if old:
                kept.append("Organizer.ExampleSet:\t%s\n" % old)
            os.makedirs(os.path.dirname(prefs), exist_ok=True)
            open(prefs, "w").writelines(kept)
        if parse:
            env["ECCE_NO_PREPARSED"] = "1"
        if during:
            env["ECCE_TEST_START_DURING_IMPORT"] = "%d %s" % (during, url)
        result = apps.run(display, "organizer", windowTimeout=60, settle=5,
                          env=env, inspect=inspect)
        log = result.log or ""
        states = pb.answers(log, "toolstate")
        inproc = pb.answers(log, "inprocess")
        print(label, "toolstate:", states)
        print(label, "inprocess:", inproc)
        print(label, "tabs:", pb.answers(log, "tabs"))
        print(label, "import:", [l for l in log.splitlines()
                                 if "imported in" in l or "example copy" in l
                                 or "examples import" in l][:12])
        print(label, "startApp:", [l for l in log.splitlines()
                                   if "startApp" in l or "ecce_tool_up" in l
                                   or "ecce_invoke " in l
                                   or "during import" in l][:8])
        print(label, "errors:", [l for l in log.splitlines()
                                 if "error parsing" in l or "recursive" in l
                                 or "Error" in l or "assert" in l][:8])
        check(os.path.isdir(calc), label + ": the calculation is there", calc)
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
        for i in range(PARSE_ROUNDS):
            run(display, "parsed import round %d" % (i + 1), True, 0.5 * i,
                parse=True)
        for i in range(UPGRADE_ROUNDS):
            which = "organic" if i % 2 == 0 else "workflow"
            run(display, "upgrade round %d (%s)" % (i + 1, which), True,
                [0, 0, 3, 3, 15, 15][i % 6], upgrade=which)
        for i in range(UPGRADE_PARSE_ROUNDS):
            which = "organic" if i % 2 == 0 else "workflow"
            run(display, "parsed upgrade round %d (%s)" % (i + 1, which),
                True, 0, parse=True, upgrade=which)
        for step in (1, 3, 12):
            run(display, "upgrade round, start at import step %d" % step,
                True, 0, upgrade="organic", during=step)
        run(display, "no import", False, 0)
        if failures:
            results.fail("viewer-first-open", "; ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
