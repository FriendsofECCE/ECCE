#!/usr/bin/env python3
"""File > Open Example Calculations imports the shipped examples (headless).

    tests/apps/examples_test.py                own Xvfb, services, local data

A user's first Organizer imports the outputs under data/client/examples
(ECCE-QM water, ORCA benzene, MOPAC ammonia, NWChem methane) into a new
project Examples by itself (#302).  The ECCE_TEST_ORGANIZER command
`examples` is the menu item: at the second start (which imports nothing)
it makes Examples_2, and a second call Examples_3, without touching the
first.  Each calculation must
carry the properties its output holds: the files under Props/ are what the
Viewer's Properties menu is built from.  The project records the version of
the example set.

Same installed tree and isolation as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""
import glob
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import isolate    # noqa: E402
import xdisplay   # noqa: E402

SKIP = 77

# calculation directory (named by the importer) -> Props the example must
# carry.  The names are the property files ECCE stores (Props/<NAME>).
EXPECT = {
    "water": ["TE", "VIBFREQ", "VIBIR", "MULLIKEN", "LOEWDIN", "ESPFIT",
              "MKFIT", "MO"],
    "benzene": ["MO", "VIBFREQ", "ESPFIT", "HIRSHFELD", "MBIS", "NPA"],
    "ethanol": ["TE", "MO", "VIBFREQ", "VIBIR", "VIBRAM", "VIBRAMDEPOL"],
    "ammonia": ["MO", "VIBFREQ"],
    "methane": ["MO", "VIBFREQ", "MULLIKEN"],
    # a GROMACS study: its tasks, each as "task:" and "task:PROP"
    "trpcage": ["prepare:", "optimize:", "equilibrate:", "dynamics:",
                "optimize:TE", "dynamics:TE"],
}
# The study has no thumbnail and no theory/run type of its own.
CALCS = [c for c in EXPECT if c != "trpcage"]

failures = []


def fail(what, why, log=""):
    tail = "\n".join("      | " + l for l in (log or "").splitlines()[-15:])
    failures.append(what)
    print("FAIL  %s: %s\n%s" % (what, why, tail))


def studyProps(study):
    """The tasks of a study as "task:" and "task:PROP" for each Props file."""
    found = set()
    for task in os.listdir(study):
        tdir = os.path.join(study, task)
        if task.startswith(".") or not os.path.isdir(tdir):
            continue
        found.add(task + ":")
        props = os.path.join(tdir, "Props")
        if os.path.isdir(props):
            found.update(task + ":" + p for p in os.listdir(props))
    return found


def propsOf(project):
    """{calculation: set of Props files} under a project directory."""
    found = {}
    for calc in sorted(os.listdir(project)) if os.path.isdir(project) else []:
        props = os.path.join(project, calc, "Props")
        if os.path.isdir(props):
            found[calc] = set(os.listdir(props))
        elif os.path.isdir(os.path.join(project, calc)):
            found[calc] = studyProps(os.path.join(project, calc))
    return found


def waitImported(root, projects, seconds=240):
    """Wait until each project holds all the example calculations whose Props stopped
    changing for 20 s."""
    end = time.time() + seconds
    last, stable = None, 0
    while time.time() < end:
        now = {p: propsOf(os.path.join(root, p)) for p in projects}
        if all(len(v) == len(EXPECT) for v in now.values()) and now == last:
            stable += 1
            if stable >= 10:
                return now
        else:
            stable = 0
        last = now
        time.sleep(2)
    return last


def reportProblemProblems(log):
    """The archive Help > Report a Problem... makes: a zip with the version
    and the ecce-diagnose text."""
    answers = re.findall(r"ECCE_TEST_ORGANIZER: reportproblem: (.*)$", log, re.M)
    if len(answers) != 1 or not answers[0].startswith("archive "):
        return ["Report a Problem answered %s" % answers]
    path = answers[0][len("archive "):]
    if not os.path.exists(path):
        return ["Report a Problem: no archive %s" % path]
    names = zipfile.ZipFile(path).namelist()
    found = [n for n in names if "ecce-diagnostics-" in n]
    if not found or not any(n.endswith("version.txt") for n in names):
        return ["Report a Problem: archive holds %s" % names]
    return []


def thumbnails(project):
    """Calculations of a project directory that hold a Thumbnail file."""
    found = set()
    for calc in os.listdir(project) if os.path.isdir(project) else []:
        for base, _dirs, files in os.walk(os.path.join(project, calc)):
            if any(f.lower().startswith("thumbnail") for f in files):
                found.add(calc)
    return found


def run(display, data, work):
    root = os.path.join(data, "users", "local")
    home = os.environ["ECCE_REALUSERHOME"]
    script = os.path.join(work, "commands")
    # Another project's calculation is the selection, as after a session
    # that ended on one.
    alpha = os.path.join(root, "alpha")
    os.makedirs(alpha)
    shutil.copy(os.path.join(ROOT, "tools", "screenshots", "data",
                             "project.ecce-meta"),
                os.path.join(alpha, ".ecce-meta"))
    calc = os.path.join(alpha, "water")
    shutil.copytree(os.path.join(HERE, "fixtures", "calc-water-opt"), calc,
                    symlinks=True)
    first = {}

    def firstStart(_display):
        first["props"] = waitImported(root, ["Examples"])

    result = apps.run(display, "organizer", windowTimeout=60, settle=20,
                      env={"ECCE_NO_EXAMPLE_IMPORT": ""}, inspect=firstStart)
    for line in re.findall(r"Organizer: examples imported in .*", result.log or ""):
        print(line)
    lg = result.log or ""
    shown = lg.find("Organizer: window shown")
    began = lg.find("Organizer: examples import started")
    if shown < 0 or began < 0 or shown > began:
        fail("window before import", "log order: window shown at %d, import "
             "started at %d" % (shown, began), lg)
    else:
        print("ok    the Organizer window is shown before the import starts")
    got = (first.get("props") or {}).get("Examples", {})
    if result.crashed or sorted(c.split("_")[0] for c in got) != sorted(EXPECT):
        fail("first start imports the examples",
             "Examples holds %s" % sorted(got), result.log)
    else:
        print("ok    first start imports the examples into Examples")
    # each example says what it is for in its Annotation field
    noted = []
    for calc in sorted(got):
        meta = os.path.join(root, "Examples", calc, ".ecce-meta")
        if os.path.exists(meta) and "annotation" in open(meta, errors="replace").read():
            noted.append(calc)
    if sorted(noted) != sorted(got):
        fail("each example has an annotation",
             "annotated: %s of %s" % (noted, sorted(got)), "")
    else:
        print("ok    each example has an annotation")

    with open(script, "w") as handle:
        handle.write("examplesmenu\nexamples\n")

    box = {}
    projects = ["Examples", "Examples_2", "Examples_3"]

    def inspect(_display):
        box["props"] = waitImported(root, projects)
        # the tree as drawn once both imports have settled
        with open(script, "a") as handle:
            # Help > Report a Problem... without its dialog
            handle.write("reportproblem\ntreeflags\ntree\n")
            # what the project table's Theory and Run Type columns show
            for calc in sorted(CALCS):
                handle.write("summary file://%s\n" % os.path.join(
                    root, "Examples_2", calc))
        time.sleep(12)
        # VizThumbnail renders them in its own process
        end = time.time() + 90
        while time.time() < end and len(
                thumbnails(os.path.join(root, "Examples_2"))) < len(CALCS):
            time.sleep(2)
        box["thumbs"] = thumbnails(os.path.join(root, "Examples_2"))
        # ecce-diagnose takes about half a minute
        end = time.time() + 120
        while time.time() < end and not glob.glob(
                os.path.join(home, "ecce-bug-*.zip")):
            time.sleep(2)

    # Through the gateway: it starts VizThumbnail for the thumbnail requests.
    result = apps.run(display, "gateway", windowTimeout=60, settle=20,
                      env={"ECCE_TEST_ORGANIZER": script,
                           "ECCE_NO_EXAMPLE_IMPORT": "",
                           "ECCE_ORGANIZER_OPEN": "file://" + calc},
                      inspect=inspect)
    log = result.log or ""
    what = "Open Example Calculations"
    if result.crashed:
        return fail(what, "CRASHED (%s)" % result.signalName, log)
    answers = re.findall(r"ECCE_TEST_ORGANIZER: examples: (.*)$", log, re.M)
    problems = []
    if answers != ["ok Examples_3"]:
        problems.append("answers %s" % answers)
    if os.path.exists(os.path.join(root, "Examples_4")):
        problems.append("the second start imported the examples again")
    for project in projects:
        got = (box.get("props") or {}).get(project, {})
        for calc, names in EXPECT.items():
            held = [c for c in got if c.startswith(calc)]
            if not held:
                problems.append("%s: no calculation %s (have %s)"
                                % (project, calc, sorted(got)))
                continue
            missing = [n for n in names if n not in got[held[0]]]
            if missing:
                problems.append("%s/%s lacks %s (has %s)"
                                % (project, held[0], missing,
                                   sorted(got[held[0]])))
        meta = os.path.join(root, project, ".ecce-meta")
        text = open(meta).read() if os.path.exists(meta) else ""
        if not re.search(r"exampleSet[^0-9]*3", text):
            problems.append("%s records no example-set version" % project)
    trees = re.findall(r"ECCE_TEST_ORGANIZER: tree: ok (.*)$", log, re.M)
    if not trees:
        problems.append("no tree answer")
    else:
        nodes = [t.lstrip("[") for t in trees[-1].split(" selected=")[0].split()]
        # Examples (from the first start) is drawn collapsed here.
        for project in projects[1:]:
            below = []
            seen = False
            for n in nodes:
                depth, _, label = n.partition(":")
                if seen and depth == "2":
                    below.append(label)
                elif seen:
                    break
                seen = label == project if depth == "1" else seen
            if sorted(below) != sorted(EXPECT):
                problems.append("tree: %s holds %s, expected %s (tree: %s)"
                                % (project, below, sorted(EXPECT), trees[-1]))
    # Theory and Run Type as the calculation ran (the importer's .param).
    want = {"water": "GeoVib", "benzene": "GeoVib", "ammonia": "GeoVib",
            "methane": "GeoVib"}
    sums = re.findall(r"ECCE_TEST_ORGANIZER: summary file://\S+/(\w+): (.*)$",
                      log, re.M)
    got = {c: s for c, s in sums}
    for calc, runtype in want.items():
        s = got.get(calc, "")
        theory = re.search(r" Theory=(\S*)", s)
        rt = re.search(r" Runtype=(\S*)", s)
        if not theory or theory.group(1) in ("", "<none>"):
            problems.append("%s: no theory in its summary (%s)" % (calc, s))
        if not rt or runtype not in rt.group(1):
            problems.append("%s: run type not %s (%s)" % (calc, runtype, s))
    problems += reportProblemProblems(log)
    requested = re.findall(r"thumbnails requested for (\d+) example", log)
    if requested[:2] != [str(len(CALCS))] * 2:
        problems.append("thumbnail requests: %s" % requested)
    if len(box.get("thumbs", ())) != len(CALCS):
        problems.append("thumbnails stored for %s" % sorted(box.get("thumbs", ())))
    if problems:
        fail(what, "; ".join(problems), log)
    else:
        print("ok    %s: %s" % (what, ", ".join(sorted(EXPECT))))


def main():
    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-organizer")):
        print("SKIP: no ecce-organizer under %s" % apps.WRAPPERS)
        return SKIP
    if not shutil.which("Xvfb"):
        print("SKIP: no Xvfb")
        return SKIP
    try:
        settings = isolate.apply(apps.INSTALL)
    except isolate.IsolationError as exc:
        print("refusing to run: %s" % exc)
        return 1
    print(isolate.describe(settings))
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    work = tempfile.mkdtemp(prefix="examples-",
                            dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data
    with xdisplay.Display() as display:
        gateway = os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")
        subprocess.run([gateway], env=display.env(), timeout=180)
        try:
            run(display, data, work)
        finally:
            stop = os.path.join(apps.INSTALL, "bin", "ecce-gateway-stop")
            subprocess.run([stop], env=display.env(), timeout=120)
    if os.environ.get("ECCE_TEST_KEEP"):
        print("kept " + work)
    else:
        shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
