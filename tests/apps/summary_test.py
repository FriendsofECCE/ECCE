#!/usr/bin/env python3
"""
The Organizer's calculation summary shows the molecule and the basis set.

A calculation keeps its formula, atom and electron counts, symmetry and
basis-set figures on its Parameters documents (the molecule and the basis
set), not on itself; the summary panel reads them from the calculation, so
the data layer has to gather them from below it.  The Organizer's
ECCE_TEST_ORGANIZER hook selects a calculation and prints the panel's
fields ("summary <url>") and saves the window ("snap <png>"), for:

- a data-server calculation: fixtures/calc-water-vib (Gaussian-16,
  def2-SVP) in the user's account;
- a local-data calculation (ECCE_LOCAL_DATA): fixtures/calc-water-opt
  (NWChem, 6-31G*).

ECCE_SUMMARY_PNGS=<dir> keeps the two window captures there.

Same installed tree, isolation and services as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""

import os
import re
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402
import run_tests  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(HERE))
PROJECT_META = os.path.join(ROOT, "tools", "screenshots", "data",
                            "project.ecce-meta")

#  The panel's values, from the fixtures' stored metadata.
EXPECT = {
    "server": {"Formula": "H2O", "Atoms": "3", "Electrons": "10",
               "Symmetry": "C2V", "Basis": "def2-svp",
               "Functions": "24", "Primitives": "38",
               "Theory": "DFT/RDFT", "Runtype": "Vibration"},
    "local": {"Formula": "H2O", "Atoms": "3", "Electrons": "10",
              "Symmetry": "C2v", "Basis": "6-31G*", "Polarization": "Cartesian",
              "Functions": "19", "Primitives": "36",
              "Theory": "SCF/RHF", "Runtype": "Geometry"},
}
FIELDS = ("Formula", "Atoms", "Electrons", "Symmetry", "Basis",
          "Polarization", "Functions", "Primitives", "Theory", "Runtype",
          "SpinMult", "OpenShells", "CDFitting")

failures = []


def parse(outcome):
    """'ok Formula=H2O Atoms=3 ...' -> dict; values hold no spaces here."""
    values = {}
    for name in FIELDS:
        m = re.search(r"\b%s=(\S*)" % name, outcome)
        if m:
            values[name] = m.group(1)
    return values


def organizer(display, url, png, env, args=()):
    work = tempfile.mkdtemp(prefix="ecce-summary-")
    cmds = os.path.join(work, "commands")
    with open(cmds, "w") as handle:
        handle.write("summary %s\nscrollsummary\nsnap %s\n" % (url, png))
    try:
        result = apps.run(display, "organizer", args=args, windowTimeout=60,
                          settle=40,
                          env=dict(env, ECCE_TEST_ORGANIZER=cmds,
                                   ECCE_ORGANIZER_OPEN=url))
    finally:
        shutil.rmtree(work, ignore_errors=True)
    return result


def launchProblem(text, queued=False):
    """The Launch Info block (#648): two columns, rows with nothing to show
    are not made. The fixtures ran on localhost with no queue: a stored
    process id is not a Job ID, and there is no allocation."""
    m = re.search(r"launch=(.*?;) (?:valuex|formula-gap|tool)=", text)
    rows = {}
    for item in (m.group(1) if m else "").split(";"):
        if "@" in item:
            name, pos = item.split("@")
            rows[name] = tuple(int(v) for v in pos.split(","))
    for name in ("Machine:", "Processors:", "Start Date:", "Used:"):
        if name not in rows:
            return "Launch Info has no %s row (got %s)" % (name, sorted(rows))
    for name in (("Allocation:",) if queued else
                 ("Job ID:", "Allocation:")):
        if name in rows:
            return "Launch Info shows %s with nothing to show" % name
    if ("Queue:" in rows) != queued:
        return "Queue: row is %s" % ("missing" if queued else "shown with no queue")
    if ("Queue state:" in rows) != queued:
        return "Queue state: row is %s" % ("missing" if queued else "shown")
    if queued:
        state = rows["Queue state:"]
        below = max(rows[n][1] for n in rows if n != "Queue state:")
        if state[1] <= below:
            return "Queue state is not below both columns (%s)" % (rows,)
        if state[0] > rows["Machine:"][0]:
            return "Queue state does not start at the left column"
        if rows["Start Date:"][0] - rows["Machine:"][0] > 450:
            return "right column is far from the left (%s)" % (rows,)
    m = re.search(r"valuex=(.*?;) (?:formula-gap|tool)=", text)
    xs = dict(i.split("@") for i in (m.group(1) if m else "").split(";") if "@" in i)
    if len(set(xs.values())) != 1 or len(xs) < 4:
        return "values of the blocks do not share one x (%s)" % xs
    if rows["Start Date:"][0] <= rows["Machine:"][0]:
        return "Start Date is not in a column right of Machine"
    if rows["Processors:"][1] <= rows["Machine:"][1]:
        return "Processors is not below Machine"
    #  The Launcher button's top is the Machine row's top, whatever rows
    #  follow (#753).
    m = re.search(r"toolpos=(.*?;)(?: |$)", text)
    pos = {}
    for item in (m.group(1) if m else "").split(";"):
        if "@" in item:
            name, xy = item.split("@")
            pos[name] = int(xy.split(",")[1])
    if "Launcher" not in pos:
        return "no Launcher button position (got %s)" % sorted(pos)
    if pos["Launcher"] != rows["Machine:"][1]:
        return "Launcher button top %d is not the Machine row top %d (%s)" % (
            pos["Launcher"], rows["Machine:"][1], rows)
    return None


def check(label, result, expect, queued=False):
    log = result.log or ""
    done = re.search(r"ECCE_TEST_ORGANIZER: summary \S+: (.*)", log)
    scrolled = re.search(r"ECCE_TEST_ORGANIZER: scrollsummary: scrolled to ([1-9]\d*) and back",
                         log)
    problem = None
    if result.crashed:
        problem = "CRASHED (%s)" % result.signalName
    elif not done:
        problem = "the summary command never answered"
    elif not scrolled:
        problem = "the summary panel did not finish scrolling"
    elif "gtk_widget_set_allocation" in log:
        problem = "GTK tried to allocate a hidden summary widget while scrolling"
    else:
        got = parse(done.group(1))
        wrong = ["%s=%r (want %r)" % (k, got.get(k), v)
                 for k, v in sorted(expect.items()) if got.get(k) != v]
        if wrong:
            problem = "summary fields wrong: " + ", ".join(wrong)
        elif launchProblem(done.group(1), queued):
            problem = launchProblem(done.group(1), queued)
        #  Both codes here have basis sets: the Basis Set button stays.
        elif "tool=Basis" not in done.group(1):
            problem = "no Basis Set button in the summary"
    if problem is None:
        print("ok    %s: %s" % (label, done.group(1)))
        return
    failures.append(label)
    print("FAIL  %s: %s\n%s" % (label, problem, run_tests._tail(log, 25)))


def serverCase(display, pngs):
    user = fixture.realUser()
    userRoot = os.path.join(fixture.stateDir(), "htdocs", "Ecce", "users",
                            user)
    project = os.path.join(userRoot, "summary-test")
    shutil.rmtree(project, ignore_errors=True)
    os.makedirs(project)
    shutil.copytree(os.path.join(fixture.FIXTURES, "calc-water-vib"),
                    os.path.join(project, "calc-water-vib"), symlinks=True)
    url = "http://localhost:%d/Ecce/users/%s/summary-test/calc-water-vib" % (
        fixture.dataserverPort(), user)
    authPath = fixture.authFile(
        os.path.join(fixture.stateHome(), ".ECCE", "auth.pipe"), user=user)
    try:
        result = organizer(display, url, os.path.join(pngs, "server.png"), {},
                           args=("-pipe", authPath))
        check("data-server calculation (Gaussian-16)", result,
              EXPECT["server"])
    finally:
        shutil.rmtree(project, ignore_errors=True)


def localCase(display, pngs):
    data = tempfile.mkdtemp(prefix="ecce-summary-data-")
    try:
        project = os.path.join(data, "users", "local", "summary-test")
        os.makedirs(project)
        shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
        calc = os.path.join(project, "water-opt")
        shutil.copytree(os.path.join(fixture.FIXTURES, "calc-water-opt"),
                        calc, symlinks=True)
        result = organizer(display, "file://" + calc,
                           os.path.join(pngs, "local.png"),
                           {"ECCE_LOCAL_DATA": data})
        check("local-data calculation (NWChem)", result, EXPECT["local"])
    finally:
        shutil.rmtree(data, ignore_errors=True)


def queuedCase(display, pngs):
    """A Submitted job with a long queue state (#657): its own row under both
    columns, and one value column for every block."""
    data = tempfile.mkdtemp(prefix="ecce-summary-data-")
    try:
        project = os.path.join(data, "users", "local", "summary-test")
        os.makedirs(project)
        shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
        calc = os.path.join(project, "water-opt")
        shutil.copytree(os.path.join(fixture.FIXTURES, "calc-water-opt"),
                        calc, symlinks=True)
        meta = os.path.join(calc, ".ecce-meta")
        ns = "http://www.emsl.pnl.gov/ecce:"
        lines = []
        for line in open(meta).read().splitlines():
            if line.startswith(".\t" + ns + "state\t"):
                line = ".\t" + ns + "state\t\tSubmitted"
            elif line.startswith(".\t" + ns + "launch_queue\t"):
                line = ".\t" + ns + "launch_queue\t\tbatch"
            lines.append(line)
        lines.append(".\t" + ns + "queueWait\t\twaiting: no node of the requested "
                     "type is free right now (BadConstraints); estimated start "
                     "Thu 29 Oct 04:11")
        open(meta, "w").write("\n".join(lines) + "\n")
        result = organizer(display, "file://" + calc,
                           os.path.join(pngs, "queued.png"),
                           {"ECCE_LOCAL_DATA": data})
        check("queued local calculation", result, EXPECT["local"], queued=True)
    finally:
        shutil.rmtree(data, ignore_errors=True)


def checkFields(label, result, expect):
    """Like check(), for the fields alone; values may hold spaces."""
    log = result.log or ""
    done = re.search(r"ECCE_TEST_ORGANIZER: summary \S+: (.*)", log)
    if result.crashed or not done:
        problem = "crashed or the summary command never answered"
    else:
        wrong = []
        for key, want in sorted(expect.items()):
            m = re.search(r"\b%s=(.*?)(?= [A-Za-z]+=|$)" % key, done.group(1))
            got = m.group(1) if m else None
            if got != want:
                wrong.append("%s=%r (want %r)" % (key, got, want))
        problem = "summary fields wrong: " + ", ".join(wrong) if wrong else None
    if problem is None:
        print("ok    %s" % label)
        return
    failures.append(label)
    print("FAIL  %s: %s\n%s" % (label, problem, run_tests._tail(log, 25)))


ORCA_RI_OUTPUT = os.path.join(HERE, "..", "parsers", "fixtures", "orca",
                              "h2o_pbe0_ri.out")


def orcaCalculations(display):
    """Stored editor settings, old settings, and actual output/import paths."""
    ns = "http://www.emsl.pnl.gov/ecce:"
    cases = [
        ("explicit-singlet", "Singlet", {"RIMode": "RIJCOSX",
         "RIJCOSXAuxBasis": "def2/J"}, "RIJCOSX", "def2/J", False),
        ("explicit-triplet", "Triplet", {"RIMode": "RIJCOSX",
         "RIJCOSXAuxBasis": "def2/J"}, "RIJCOSX", "def2/J", False),
        ("legacy-checkbox", "Singlet", {"UseRIJCOSX": "1",
         "RIJCOSXAuxBasis": "def2/J"}, "RIJCOSX", "def2/J", False),
        ("default-auxiliary", "Singlet", {"RIMode": "RIJCOSX"},
         "RIJCOSX", "def2/J", False),
        ("rijonx", "Singlet", {"RIMode": "RIJONX",
         "RIJCOSXAuxBasis": "SARC/J"}, "RIJONX", "SARC/J", False),
        ("explicit-none", "Singlet", {"RIMode": "None", "UseRIJCOSX": "1",
         "RIJCOSXAuxBasis": "def2/J"}, "", "", False),
        ("no-saved-details", "Singlet", {}, "", "", False),
        ("output-without-editor-ri", "Singlet", {}, "RIJCOSX", "def2/J", True),
        ("implicit-output-without-editor-ri", "Singlet", {}, "RIJCOSX", "def2/J", "implicit"),
    ]
    data = tempfile.mkdtemp(prefix="ecce-summary-data-")
    try:
        project = os.path.join(data, "users", "local", "summary-test")
        os.makedirs(project)
        shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
        for name, mult, settings, mode, aux, output in cases:
            calc = os.path.join(project, name)
            shutil.copytree(os.path.join(fixture.FIXTURES,
                                         "calc-orca-water-ccpvdz"),
                            calc, symlinks=True)
            functional = "B3LYP" if output == "implicit" else "PBE0 D3BJ"
            setup = "ES.Theory.DFT.XCFunctionals|%s||1|1|menu_input\n" % functional
            setup += "".join("ES.Theory.SCF.%s|%s||1|1|menu_input\n" % item
                             for item in settings.items())
            with open(os.path.join(calc, "Parameters", "SetupParams"), "w") as f:
                f.write(setup)
            meta = os.path.join(calc, ".ecce-meta")
            lines = []
            for line in open(meta).read().splitlines():
                if line.startswith(".\t" + ns + "spinmultiplicity\t"):
                    line = ".\t" + ns + "spinmultiplicity\t\t" + mult
                elif "hasConfigurations" in line:
                    line = line.replace("</rdf:Bag>",
                        '<rdf:li><rdf:href xlink:href="Parameters/SetupParams" '
                        'xlink:type="simple"></rdf:href></rdf:li></rdf:Bag>')
                lines.append(line)
            open(meta, "w").write("\n".join(lines) + "\n")
            if output:
                os.makedirs(os.path.join(calc, "Outputs"))
                source = (os.path.join(os.path.dirname(ORCA_RI_OUTPUT), "h2o_smd.out")
                          if output == "implicit" else ORCA_RI_OUTPUT)
                shutil.copy(source, os.path.join(calc, "Outputs", "orca.orcaout"))
            theory = "DFT/RDFT \u00b7 " + functional
            if mode:
                theory += " \u00b7 " + mode
            yield name, data, "file://" + calc, {
                "Theory": theory, "OpenShells": "2" if mult == "Triplet" else "0",
                "CDFitting": aux}

        # Use File > Import's real importer and job store, with no editor setup.
        result = apps.run(display, "organizer", windowTimeout=60, settle=110,
                          env={"ECCE_LOCAL_DATA": data,
                               "ECCE_TEST_CALCIMPORT": os.path.abspath(ORCA_RI_OUTPUT),
                               "ECCE_TEST_IMPORTREPORT": "1"})
        log = result.log or ""
        selected = re.search(r"ECCE_TEST_CALCIMPORT: selected (\S+)", log)
        if result.crashed or not selected or "ECCE_TEST_CALCIMPORT: imported, code ORCA:" not in log:
            failures.append("ORCA actual import")
            print("FAIL  ORCA actual import\n" + run_tests._tail(log, 40))
        else:
            yield "imported-output", data, selected.group(1), {
                "Theory": "DFT/RDFT \u00b7 PBE0 \u00b7 RIJCOSX",
                "OpenShells": "0", "CDFitting": "def2/J"}
    finally:
        shutil.rmtree(data, ignore_errors=True)


def orcaCase(display, pngs):
    for name, data, url, expect in orcaCalculations(display):
        result = organizer(display, url, os.path.join(pngs, "orca-%s.png" % name),
                           {"ECCE_LOCAL_DATA": data})
        checkFields("ORCA " + name, result, expect)


def main():
    def checkApp(display, name, results, verbose=False):
        keep = os.environ.get("ECCE_SUMMARY_PNGS")
        pngs = keep or tempfile.mkdtemp(prefix="ecce-summary-png-")
        os.makedirs(pngs, exist_ok=True)
        try:
            serverCase(display, pngs)
            localCase(display, pngs)
            queuedCase(display, pngs)
            orcaCase(display, pngs)
        finally:
            if not keep:
                shutil.rmtree(pngs, ignore_errors=True)
        if failures:
            results.fail("summary", "summary panel wrong: %s"
                         % ", ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
