#!/usr/bin/env python3
"""File > Open Example Calculations imports the shipped examples (headless).

    tests/apps/examples_test.py                own Xvfb, services, local data

A user's first Organizer imports the outputs under data/client/examples
(ECCE-QM water, ORCA benzene, MOPAC ammonia, NWChem methane) into a new
project Examples/Getting_Started by itself (#302): the `getting-started` set,
five calculations.  Sets are projects below Examples, topics projects below
those: Examples/<Set>/<Topic>/<calculation>.
The ECCE_TEST_ORGANIZER command `examples SET` is the dialog's OK: at the
second start (which imports nothing) `examples computational` makes the
project Examples/Computational with the Trp-cage study under GROMACS,
`examples organic` and `examples spectroscopy` (#447, #485) make
Examples/Organic_Chemistry and Examples/Spectroscopy; asking again (or for
`getting-started`) selects the existing project and duplicates nothing.  Every calculation of the two new sets must
import with its properties and carry a note that says what it shows.
Each calculation must
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
    # SLATERBASIS: the .mgf beside the output reaches its parse on import
    "ammonia": ["MO", "VIBFREQ", "SLATERBASIS"],
    "methane": ["MO", "VIBFREQ", "MULLIKEN"],
}
COMPUTATIONAL = {
    # a GROMACS study: its tasks, each as "task:" and "task:PROP"
    "trpcage": ["prepare:", "optimize:", "equilibrate:", "dynamics:",
                "optimize:TE", "dynamics:TE"],
}
VIBS = ["TE", "VIB", "VIBFREQ", "VIBIR"]
ORCA_RAMAN = ["TE", "MO", "VIB", "VIBFREQ", "VIBIR", "VIBRAM", "VIBRAMDEPOL"]
QM_LOC = VIBS + ["MO", "LOCMO", "LOCMOENG", "LOCMOLABEL", "LOCMOOCC"]
SCAN = ["TE", "SCANCOORD", "SCANENERGY", "SCANGEOM"]
ORGANIC = {
    "ethane": QM_LOC, "ethene": QM_LOC, "ethyne": QM_LOC,
    "ethane_scan": SCAN, "butane_scan": SCAN, "nma_scan": SCAN,
    "nma_scan_rev": SCAN,
    "benzene": ["MO", "VIBFREQ", "ESPFIT", "HIRSHFELD", "MBIS", "NPA"],
    "butadiene": VIBS, "ethylene": VIBS, "cyclohexene": VIBS,
    "diels_alder_ts": VIBS, "diels_alder_irc": ["TE", "IRCCOORD", "IRCENERGY"],
    "chloride": ["TE"], "methyl_bromide": VIBS, "reactant_complex": VIBS,
    "sn2_ts": VIBS, "sn2_irc": ["TE", "IRCCOORD", "IRCENERGY"],
    "product_complex": VIBS, "bromide": ["TE"], "methyl_chloride": VIBS,
}
SPECTROSCOPY = {c: ORCA_RAMAN for c in (
    "formaldehyde", "acetaldehyde", "acetone", "acetic_acid", "acetate_anion",
    "methyl_acetate", "ethyl_acetate", "methyl_propionate",
    "co2_en4_oh_so4_bridged", "sulfate", "co_nh3_5_oso3", "co_en2_cl_oso3", "co_en2_o2so2",
    "n_methylacetamide", "ac_ala_nhme", "alpha_d_glucopyranose",
    "alpha_d_galactopyranose")}
SPECTROSCOPY.update({
    "water": ["TE", "VIBFREQ", "VIBIR", "MO"],
    "ethanol": ["TE", "MO", "VIBFREQ", "VIBIR", "VIBRAM", "VIBRAMDEPOL"]})
GENCHEM = {
    "ar-gas": ["dynamics:", "dynamics:TE", "dynamics:MDMSD", "dynamics:MDSPEED"],
    "he-gas": ["dynamics:", "dynamics:TE", "dynamics:MDMSD", "dynamics:MDSPEED"],
    "ar-liquid": ["dynamics:", "dynamics:TE", "dynamics:MDMSD"],
    "water-spce": ["dynamics:", "dynamics:TE", "dynamics:MDMSD"],
    "decane-opls": ["dynamics:", "dynamics:TE", "dynamics:MDMSD"],
    "nacl-dissolving": ["dynamics:", "dynamics:TE", "dynamics:MDMSD"],
    "ice-to-steam": ["dynamics:", "dynamics:TE"],
}
NEW_SETS = {"Examples/Organic_Chemistry": ORGANIC,
            "Examples/Spectroscopy": SPECTROSCOPY,
            "Examples/General_Chemistry": GENCHEM}
FIRST = "Examples/Getting_Started"
COMP = "Examples/Computational"
# The study has no thumbnail and no theory/run type of its own.
CALCS = list(EXPECT)

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
    """{calculation: set of Props files} below a project directory."""
    found = {}
    for base, dirs, _files in os.walk(project):
        name = os.path.basename(base)
        if "Props" in dirs:
            found[name] = set(os.listdir(os.path.join(base, "Props")))
            dirs[:] = []
        elif name in COMPUTATIONAL or name in GENCHEM:
            found[name] = studyProps(base)
            dirs[:] = []
    return found


def calcPath(project, calc):
    """The folder of a calculation below a set's project (topics nest)."""
    for base, dirs, _files in os.walk(project):
        if os.path.basename(base) == calc:
            return base
    return os.path.join(project, calc)


def complete(found, expect):
    return all(set(names) <= found.get(calc, set())
               for calc, names in expect.items())


def waitImported(root, projects, expect, seconds=240):
    """Wait until each project holds all the example calculations with every
    expected property, and their Props stopped changing for 20 s.  A quiet
    spell alone is not enough: a long run (NaCl, 10000 energy frames) keeps
    its job store busy for minutes after its first properties land."""
    end = time.time() + seconds
    last, stable = None, 0
    while time.time() < end:
        now = {p: propsOf(os.path.join(root, p)) for p in projects}
        if all(complete(v, expect) for v in now.values()) and now == last:
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
        first["props"] = waitImported(root, [FIRST], EXPECT)

    result = apps.run(display, "organizer", windowTimeout=60, settle=60,
                      env={"ECCE_NO_EXAMPLE_IMPORT": ""}, inspect=firstStart)
    for line in re.findall(r"Organizer: examples \w+ imported in .*", result.log or ""):
        print(line)
        # Printed, not asserted tightly: it was 10 s with a 1 s sleep per
        # import in the job monitor hand-over (#426).
        took = float(re.search(r"in ([0-9.]+) s", line).group(1))
        if took > 60:
            fail("examples import time", "%.1f s (limit 60 s)" % took)
    lg = result.log or ""
    shown = lg.find("Organizer: window shown")
    began = lg.find("Organizer: examples import started")
    if shown < 0 or began < 0 or shown > began:
        fail("window before import", "log order: window shown at %d, import "
             "started at %d" % (shown, began), lg)
    else:
        print("ok    the Organizer window is shown before the import starts")
    progress = re.findall(r"Organizer: example progress: (\d+) of (\d+) - (.*)$",
                          lg, re.M)
    if [(a, b) for a, b, _ in progress] != [(str(i), "5") for i in range(1, 6)]:
        fail("import progress", "progress lines %s" % progress, lg)
    else:
        print("ok    progress names each calculation: %s"
              % ", ".join(n for _, _, n in progress))
    got = (first.get("props") or {}).get(FIRST, {})
    if result.crashed or sorted(c.split("_")[0] for c in got) != sorted(EXPECT):
        fail("first start imports exactly the general set",
             "%s holds %s" % (FIRST, sorted(got)), result.log)
    else:
        print("ok    first start imports the getting-started set into " + FIRST)
    if os.path.exists(os.path.join(root, COMP)):
        fail("first start", "also imported the computational set", "")
    # each example says what it is for in its Annotation field
    noted = []
    for calc in sorted(got):
        meta = os.path.join(calcPath(os.path.join(root, FIRST), calc), ".ecce-meta")
        if os.path.exists(meta) and "annotation" in open(meta, errors="replace").read():
            noted.append(calc)
    if sorted(noted) != sorted(got):
        fail("each example has an annotation",
             "annotated: %s of %s" % (noted, sorted(got)), "")
    else:
        print("ok    each example has an annotation")

    with open(script, "w") as handle:
        handle.write("examples computational\nexamples computational\n"
                     "examples getting-started\nexamples organic\n"
                     "examples spectroscopy\nexamples genchem\nexamples organic\n")

    box = {}
    projects = [COMP]

    def inspect(_display):
        box["props"] = waitImported(root, projects, COMPUTATIONAL)
        for project, expect in NEW_SETS.items():
            box[project] = waitImported(
                root, [project], expect,
                seconds=900 if expect is GENCHEM else 240)[project]
        # the tree as drawn once both imports have settled
        with open(script, "a") as handle:
            # The commands run in order and Report a Problem takes half a
            # minute: it goes last, so its archive means all the answers
            # are in the log.
            handle.write("treeflags\ntree\n")
            # what the project table's Theory and Run Type columns show
            for calc in sorted(CALCS):
                handle.write("summary file://%s\n" % calcPath(
                    os.path.join(root, FIRST), calc))
            # Help > Report a Problem... without its dialog
            handle.write("reportproblem\n")
        time.sleep(12)
        # ecce-diagnose takes about half a minute
        end = time.time() + 120
        while time.time() < end and not glob.glob(
                os.path.join(home, "ecce-bug-*.zip")):
            time.sleep(2)
        # the archive is written just before the answer is logged
        time.sleep(3)

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
    answers = re.findall(r"ECCE_TEST_ORGANIZER: examples [^:]*: (.*)$", log, re.M)
    problems = []
    if answers != ["ok Examples/Computational created",
                   "ok Examples/Computational existing",
                   "ok Examples/Getting_Started existing",
                   "ok Examples/Organic_Chemistry created",
                   "ok Examples/Spectroscopy created",
                   "ok Examples/General_Chemistry created",
                   "ok Examples/Organic_Chemistry existing"]:
        problems.append("answers %s" % answers)
    for extra in ("Examples_2", "Examples_3", "Examples_Organic",
                  "Examples_Spectroscopy", "Examples_Computational"):
        if os.path.exists(os.path.join(root, extra)):
            problems.append("%s: an example set was imported twice" % extra)
    if len(propsOf(os.path.join(root, FIRST))) != len(EXPECT):
        problems.append("%s changed: %s"
                        % (FIRST, sorted(propsOf(os.path.join(root, FIRST)))))
    for project in projects:
        got = (box.get("props") or {}).get(project, {})
        for calc, names in COMPUTATIONAL.items():
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
        if not re.search(r"exampleSet[^0-9]*10", text):
            problems.append("%s records no example-set version" % project)
    for project, expect in NEW_SETS.items():
        got = box.get(project) or {}
        if sorted(got) != sorted(expect):
            problems.append("%s holds %s, expected %s"
                            % (project, sorted(got), sorted(expect)))
        for calc, names in expect.items():
            missing = [n for n in names if n not in got.get(calc, ())]
            if missing:
                problems.append("%s/%s lacks %s" % (project, calc, missing))
            meta = os.path.join(calcPath(os.path.join(root, project), calc),
                                ".ecce-meta")
            text = open(meta, errors="replace").read() if os.path.exists(meta) else ""
            if "This calculation shows" not in text:
                problems.append("%s/%s: its note does not say what it shows"
                                % (project, calc))
            for bad in ("Gaussian", "literature"):
                if bad in text:
                    problems.append("%s/%s: note mentions %s"
                                    % (project, calc, bad))
    trees = re.findall(r"ECCE_TEST_ORGANIZER: tree: ok (.*)$", log, re.M)
    if not trees:
        problems.append("no tree answer")
    else:
        nodes = [t.lstrip("[") for t in trees[-1].split(" selected=")[0].split()]
        # Examples > Computational > GROMACS > trpcage, one level deeper each
        want = ["1:Examples", "2:Computational", "3:GROMACS", "4:trpcage"]
        got = [n for n in nodes if n in want]
        if got != want:
            problems.append("tree: %s lacks the nested projects (tree: %s)"
                            % (want, trees[-1]))
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
    if problems:
        fail(what, "; ".join(problems), log)
    else:
        print("ok    %s: %s, %d organic and %d spectroscopy calculations"
              % (what, ", ".join(sorted(COMPUTATIONAL)), len(ORGANIC),
                 len(SPECTROSCOPY)))


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
