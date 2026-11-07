#!/usr/bin/env python3
"""MANUAL, needs ORCA 6 under /opt/orca (not run by ctest): the Fragment
analysis recipe's metal field step end to end (#201).

Cr(CO)6, ORCA RHF/def2-SVP (tests/fragorb_h5/inputs/crco6.inp, run here and
imported as a finished calculation).  The recipe is applied after it with
Cr | (CO)6; complex, fragment-1-field, fragment-1 and fragment-2 are launched
in order on a local Shell machine (ORCA at most 4 cores, nice 19) and the
decks, the .pc file, the MORead guess, the occupied d set of the Cr fragment
and the Fragment orbitals box are reported.

    ECCE_TEST_HOME=<tree>/.inst/ecce ECCE_TEST_WRAPPERS=<tree>/.inst/bin \
        tests/apps/fragorb_e2e_manual.py

Isolation as fragorb_gui_test.py: private HOME state, ECCE_LOCAL_DATA temp dir.
"""
import glob
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import isolate    # noqa: E402
import xdisplay   # noqa: E402

PROJECT_META = os.path.join(ROOT, "tools", "screenshots", "data", "project.ecce-meta")
INPUT = os.path.join(ROOT, "tests", "fragorb_h5", "inputs", "crco6.inp")
STEPS = ("complex", "fragment-1-field", "fragment-1", "fragment-2")
failures = []


def say(text):
    print(text, flush=True)


def check(what, cond, why=""):
    say(("ok    " if cond else "FAIL  ") + what + ("" if cond else "\n      " + str(why)[:1500]))
    if not cond:
        failures.append(what)
    return cond


def findOrca():
    for path in sorted(glob.glob("/opt/orca/orca_6*/orca")):
        if os.access(path, os.X_OK):
            return path
    return ""


def organizer(display, work, tag, commands, env=None, settle=60):
    script = os.path.join(work, "commands-" + tag)
    with open(script, "w") as f:
        f.write("\n".join(commands) + "\n")
    environment = {"ECCE_TEST_ORGANIZER": script}
    environment.update(env or {})
    result = apps.run(display, "organizer", windowTimeout=60, settle=settle, env=environment)
    log = result.log or ""
    with open(os.path.join(work, tag + ".log"), "w") as f:
        f.write(log)
    check("the Organizer ran (%s)" % tag, not result.crashed, log[-1500:])
    return log


def answers(log, prefix):
    return re.findall(r"ECCE_TEST_ORGANIZER: %s.*?: (.*)$" % re.escape(prefix), log, re.M)


def calced(display, work, wf, step):
    """Open the step in the Calculation Editor and Save: writes the deck."""
    commands = os.path.join(work, "calced-" + step)
    with open(commands, "w") as f:
        f.write("state\nsave\n")
    result = apps.run(display, "calced", args=("-context", wf + "/" + step),
                      windowTimeout=60, settle=25,
                      env={"ECCE_TEST_CALCED": commands, "ECCE_TMPDIR": work})
    text = result.log or ""
    with open(os.path.join(work, "calced-%s.log" % step), "w") as f:
        f.write(text)
    check("%s: the editor saved the input" % step,
          not result.crashed and "ECCE_TEST_CALCED: save: kept " in text, text[-1500:])


def register(orca):
    prefs = os.path.join(os.environ["ECCE_REALUSERHOME"], ".ECCE")
    os.makedirs(prefs, exist_ok=True)
    with open(os.path.join(prefs, "MyMachines"), "w") as h:
        h.write("localhost\tlocalhost\tGeneric\tqueue test machine\tUnspecified\t"
                "1:4\tssh\t:ORCA\tMN:RD:SD:Q:TL:MM:SS:AA\n")
    with open(os.path.join(prefs, "Queues"), "w") as h:
        h.write("Queues: localhost\nlocalhost|queueMgrName:   Shell\n"
                "localhost|prefFile:       localhost.Q\n\n")
    with open(os.path.join(prefs, "localhost.Q"), "w") as h:
        h.write("Queues: normal\nnormal|minProcessors: 1\nnormal|maxProcessors: 4\n"
                "normal|runLimit: 1440\nnormal|memLimit: 0\nnormal|memUnits: MB\n")
    with open(os.path.join(prefs, "CONFIG.localhost"), "w") as h:
        h.write("ORCA: %s\nperlPath: /usr/bin\n" % orca)


def stateOf(display, work, wf, tag):
    log = organizer(display, work, tag, ["workflowinfo %s" % wf], settle=20)
    info = answers(log, "workflowinfo")
    states = {}
    for entry in (info[-1] if info else "").split(" links:")[0].replace("ok ", "", 1).split("|"):
        f = entry.split()
        if len(f) >= 3:
            states[f[0]] = f[2]
    return states


def run(display, work, data, orca):
    root = os.path.join(data, "users", "local")
    # 1 ORCA on the complex, imported as a finished calculation
    ref = os.path.join(work, "crco6")
    os.makedirs(ref)
    shutil.copy(INPUT, ref)
    with open(os.path.join(ref, "crco6.out"), "w") as out:
        subprocess.run(["nice", "-n", "19", orca, "crco6.inp"], cwd=ref, stdout=out,
                       stderr=subprocess.STDOUT)
    check("ORCA ran on the complex", "ORCA TERMINATED NORMALLY" in open(os.path.join(ref, "crco6.out")).read())
    log = organizer(display, work, "import", [], settle=90,
                    env={"ECCE_TEST_CALCIMPORT": os.path.join(ref, "crco6.out")})
    m = re.search(r"ECCE_TEST_CALCIMPORT: imported.*", log)
    if not check("the output was imported", m, log[-1500:]):
        return
    proj = "file://" + os.path.join(root, "calcimport-test")
    names = [n for n in os.listdir(os.path.join(root, "calcimport-test")) if not n.startswith(".")]
    wf = proj + "/Workflow"
    calc = names[0]
    log = organizer(display, work, "recipe", [
        "new %s workflow" % proj,
        "workflowcopy %s %s/%s co_complex" % (wf, proj, calc),
        "workflowrecipe %s fragment-analysis orca %s/co_complex fragments=1:0:1/2-13:0:1 method=HF solvent=none" % (wf, wf),
        "workflowinfo %s" % wf], settle=40)
    say("recipe: " + " ".join(answers(log, "workflowrecipe")) + "\n" + "\n".join(answers(log, "workflowinfo")))
    folder = os.path.join(root, "calcimport-test", "Workflow")
    rundirs = {}
    for step in STEPS:
        rundirs[step] = os.path.join(work, "run-" + step)
        os.makedirs(rundirs[step])
    log = organizer(display, work, "setlaunch", [
        "workflowsetlaunch %s %s/%s localhost %s" % (wf, wf, s, rundirs[s]) for s in STEPS], settle=40)
    pc = os.path.join(folder, "fragment-1-field", "Inputs", "pointcharges.pc")
    pcl = open(pc).read().split("\n") if os.path.exists(pc) else []
    check("the field step stores 6 point charges of -0.1 e",
          pcl[:1] == ["6"] and sum(1 for l in pcl[1:] if l.startswith("-0.1 ")) == 6, pcl[:8])
    for step in STEPS:
        if step == "fragment-1":
            # attaches the field step's orbitals before the deck is written
            organizer(display, work, "refresh", ["workflowready %s" % wf,
                                                 "start CalculationEditor %s/%s" % (wf, step)],
                      env={"ECCE_TEST_ORGANIZER_NOLAUNCH": "1"}, settle=30)
        calced(display, work, wf, step)
        log = organizer(display, work, "launch-" + step, ["workflowlaunchstep %s %s/%s" % (wf, wf, step)], settle=40)
        say("launch %s: %s" % (step, " ".join(answers(log, "workflowlaunchstep"))))
        for _ in range(60):
            st = stateOf(display, work, wf, "poll-" + step).get(step, "?")
            if st.lower() in ("complete", "completed", "failed", "killed", "unsuccessful"):
                break
            time.sleep(5)
        check("%s finished" % step, st.lower().startswith("complete"), st)
        say("--- %s run dir: %s" % (step, sorted(os.path.relpath(os.path.join(d, f), rundirs[step]) for d, _, fs in os.walk(rundirs[step]) for f in fs)))
        for deck in glob.glob(os.path.join(rundirs[step], "**", "*.orcain"), recursive=True) + glob.glob(os.path.join(rundirs[step], "**", "*.pc"), recursive=True):
            say("--- %s deck %s\n%s" % (step, os.path.basename(deck), open(deck).read()[:1500]))
        if not st.lower().startswith("complete"):
            return
        text = "".join(open(d).read() for d in glob.glob(
            os.path.join(rundirs[step], "**", "orca.orcain"), recursive=True))
        if step == "fragment-1-field":
            check("field deck has %pointcharges", '%pointcharges "pointcharges.pc"' in text, text)
        if step == "fragment-1":
            check("fragment-1 deck has MORead and %moinp", "MORead" in text and "%moinp" in text, text)
    log = organizer(display, work, "box", ["workflowfragorb %s" % wf], settle=60)
    box = "\n".join(answers(log, "workflowfragorb"))
    check("the box shows no field-check warning", "do not match" not in box, box)
    say("BOX: " + "\n".join(answers(log, "workflowfragorb")).replace("|", "\n"))
    r = subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-fragorb"), "analyse"] +
                       [os.path.join(folder, s) for s in ("complex", "fragment-1", "fragment-2")],
                       env=dict(os.environ, ECCE_HOME=apps.INSTALL), capture_output=True, text=True)
    say("ecce-fragorb analyse:\n" + r.stdout[-4000:] + r.stderr[-500:])


def main():
    orca = findOrca()
    if not orca or not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-organizer")) or not shutil.which("Xvfb"):
        say("SKIP: needs ORCA 6, an install and Xvfb")
        return 77
    try:
        settings = isolate.apply(apps.INSTALL)
    except isolate.IsolationError as exc:
        say("refusing to run: %s" % exc)
        return 1
    say(isolate.describe(settings))
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    work = tempfile.mkdtemp(prefix="fragorb-e2e-", dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    register(orca)
    with xdisplay.Display() as display:
        subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")], env=display.env(), timeout=180)
        try:
            run(display, work, data, orca)
        finally:
            subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-stop")], env=display.env(), timeout=120)
    say("kept " + work)
    say("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
