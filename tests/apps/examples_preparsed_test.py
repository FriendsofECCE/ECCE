#!/usr/bin/env python3
"""The pre-parsed example calculations agree with a parse of the outputs (#448).

    tests/apps/examples_preparsed_test.py

Per file-output example set (getting-started, organic, spectroscopy), two imports in a
fresh state each: one with
ECCE_NO_PREPARSED=1 (the output parsers run), one as shipped (the stored
copies under data/client/examples/<set>/[<topic>/]<example>/stored are copied).  Every
file of every example calculation must be the same both ways, and every stored
property too, except dates, the user, the machine and job directories.  The
copying arm must really have copied all five, so a stale copy that quietly
falls back to the parse fails here, as does one that no longer matches.
Prints the time of both arms.  Exit 77 (skip) without an install or Xvfb.
"""
import difflib
import os
import re
import shutil
import subprocess
import sys
import tempfile

import apps

HERE = os.path.dirname(os.path.abspath(__file__))
SKIP = 77
# set folder -> (project it is imported into, number of calculations)
SETS = {"getting-started": ("Examples/Getting_Started", 6),
        "computational": ("Examples/Computational", 4),
        "organic": ("Examples/Organic_Chemistry", 35),
        "spectroscopy": ("Examples/Spectroscopy", 19),
        "genchem": ("Examples/General_Chemistry", 137),
        "workflows": ("Examples/Workflows", 9),
        "test": ("Examples/Test", 22),
        "docking": ("Examples/Docking", 3)}

# Written per import: who ran it, when, and where (StoredCalcCopy's kSkip).
VOLATILE = {"owner", "creationdate", "createdWith", "currentVersion",
            "startdate", "completiondate", "job_path", "job_jobid",
            "job_clienthost", "job_clientid", "launch_user", "launch_machine",
            "launch_remoteShell", "launch_rundir", "launch_account",
            "launch_profile", "launch_allocleft", "submitdate", "version",
            "taskIndex"}
# The progress record carries the clock of the run it came from.
# seq counts the monitor's messages, which depends on timing, not content;
# eta is an IRC's or scan's time-to-go estimate, from the parse's own clock.
CLOCK = re.compile(r"(seq|sent|at|eta)=[0-9.]+")
IGNORED_FILES = re.compile(r"(^|/)(Thumbnail[^/]*|[^/]*\.lock)$")
DATE = re.compile(r"\b(Mon|Tue|Wed|Thu|Fri|Sat|Sun), \d\d \w{3} \d{4} "
                  r"\d\d:\d\d:\d\d GMT")
DATE_B = re.compile(DATE.pattern.encode())


def runArm(out, noPreparsed, setId):
    env = dict(os.environ)
    env.pop("ECCE_NO_PREPARSED", None)
    if noPreparsed:
        env["ECCE_NO_PREPARSED"] = "1"
    project, count = SETS[setId]
    proc = subprocess.run([sys.executable, os.path.join(HERE, "examples_arm.py"),
                           out, setId, project, str(count)], env=env, capture_output=True, text=True)
    sys.stdout.write(proc.stdout[-1500:] if proc.returncode else "")
    m = re.search(r"^ARM (\S+) (\S+) (\d+) (\d+)$", proc.stdout, re.M)
    return proc.returncode, m, proc.stdout


def records(path):
    """{(resource, property): value} of a sidecar, minus the volatile ones."""
    found = {}
    with open(path, errors="replace") as handle:
        for line in handle:
            parts = line.rstrip("\n").split("\t", 3)
            if len(parts) != 4:
                continue
            name = parts[1].rsplit(":", 1)[-1]
            if name in VOLATILE:
                continue
            found[(parts[0], parts[1])] = CLOCK.sub(r"\1=<t>",
                                                    DATE.sub("<date>", parts[3]))
    return found


def files(root):
    found = set()
    for base, _dirs, names in os.walk(root):
        for n in names:
            rel = os.path.relpath(os.path.join(base, n), root)
            if not IGNORED_FILES.search(rel):
                found.add(rel)
    return found


def h5content(path):
    """What results.h5 holds, as ecce-results prints it: the summary and the
    full geometry, basis and orbitals of a Molden export."""
    tool = os.path.join(os.environ.get("ECCE_TEST_WRAPPERS", "/usr/bin"),
                        "ecce-results")
    env = dict(os.environ)
    home = tempfile.mkdtemp(prefix="h5cmp-")
    env["ECCE_REALUSERHOME"] = home
    try:
        show = subprocess.run([tool, "show", path], env=env,
                              capture_output=True, text=True).stdout
        molden = os.path.join(home, "x.molden")
        subprocess.run([tool, "export", "molden", path, molden], env=env,
                       capture_output=True)
        text = open(molden).read() if os.path.exists(molden) else ""
        return show + text
    finally:
        shutil.rmtree(home, ignore_errors=True)


def compare(parsed, copied, skipFiles=None, skipProps=()):
    problems = []
    a, b = files(parsed), files(copied)
    if skipFiles:
        a = {r for r in a if not skipFiles.search(r)}
        b = {r for r in b if not skipFiles.search(r)}
    for rel in sorted(a - b):
        problems.append("missing in the copy: " + rel)
    for rel in sorted(b - a):
        problems.append("only in the copy: " + rel)
    for rel in sorted(a & b):
        pa, pb = os.path.join(parsed, rel), os.path.join(copied, rel)
        if os.path.basename(rel) == ".ecce-meta":
            ra, rb = records(pa), records(pb)
            for key in sorted(set(ra) | set(rb)):
                if rel == ".ecce-meta" and key[1].rsplit(":", 1)[-1] in skipProps:
                    continue
                if ra.get(key) != rb.get(key):
                    problems.append("%s %s: parse %r, copy %r" % (
                        rel, key[1].rsplit(":", 1)[-1],
                        ra.get(key, "-")[-160:], rb.get(key, "-")[-160:]))
        elif os.path.basename(rel) == "eccejobstorelog.ecce_run_log":
            continue
        elif rel.endswith("/MO") and "MO_table_check" in parsed + copied:
            continue    # shipped short on purpose (#719); a parse is whole    # a log of this run, with its own times
        elif rel.endswith(".h5"):
            # HDF5 does not lay out the same content byte for byte twice
            ha, hb = h5content(pa), h5content(pb)
            if ha != hb:
                # Name the first differing lines, so a failure says which
                # dataset moved without rerunning by hand.
                diff = [l.rstrip("\n") for l in difflib.unified_diff(
                    ha.splitlines(1), hb.splitlines(1), "parse", "copy", n=0)
                    if not l.startswith(("---", "+++"))]
                problems.append("%s: content differs\n        %s" % (
                    rel, "\n        ".join(diff[:8])))
        else:
            with open(pa, "rb") as x, open(pb, "rb") as y:
                da, db = x.read(), y.read()
            if da != db and DATE_B.sub(b"", da) != DATE_B.sub(b"", db):
                problems.append("%s differs (%d and %d bytes)"
                                % (rel, len(da), len(db)))
    return problems


# A docked example cannot be identical in both arms.  The copy arm is a RUN
# (what Launch makes: the Vina inputs, the structure before docking); the
# parse arm is an IMPORT of the finished run, which has no inputs and, because
# a Vina output holds only docked poses, the structure of pose 1.  So each of
# those differences is asserted to its expected value, and everything else
# (POSESCORE, GEOMTRACE, the receptor atoms, every other record) is compared
# exactly as for any set.
DOCK_FILES = re.compile(r"^(Inputs/|Outputs/[^/]*\.(out|vinaout)$|Parameters/chemsys\.mvm$)")
DOCK_PROPS = ("state", "hasinputs", "launch_maxwall", "launch_nodes",
              "launch_totalprocs")
COORD_TOL = 2e-3     # the files carry 3 decimals; .mvm 4 significant digits


def meta(calc, name):
    for (res, key), value in records(os.path.join(calc, ".ecce-meta")).items():
        if res == "." and key.rsplit(":", 1)[-1] == name:
            return value
    return None


def mvmAtoms(path):
    """[(element, x, y, z)] of an ECCE .mvm/.frag atom_list."""
    lines = open(path, errors="replace").read().split("\n")
    start = [i for i, l in enumerate(lines) if l.startswith("atom_list")][0]
    atoms = []
    for line in lines[start + 1:]:
        t = line.split()
        if len(t) != 4:
            break
        atoms.append((t[0], float(t[1]), float(t[2]), float(t[3])))
    return atoms


def traceFrame1(path):
    """The coordinates of step 1 of a GEOMTRACE, as [(x, y, z)]."""
    m = re.search(r'<step number="1">(.*?)</step>', open(path).read(), re.S)
    v = [float(x) for x in m.group(1).split()]
    return [tuple(v[i:i + 3]) for i in range(0, len(v), 3)]


def maxDev(a, b):
    return max(abs(p - q) for x, y in zip(a, b) for p, q in zip(x, y))


def compareDocking(parsed, copied):
    problems = compare(parsed, copied, DOCK_FILES, DOCK_PROPS)

    def expect(what, got, want):
        if got != want:
            problems.append("%s: %r, expected %r" % (what, got, want))

    def isdir(calc, name):
        return os.path.isdir(os.path.join(calc, name))

    expect("copy arm state", meta(copied, "state"), "Complete")
    expect("parse arm state", meta(parsed, "state"), "Loaded")
    expect("copy arm Inputs", sorted(os.listdir(os.path.join(copied, "Inputs")))
           if isdir(copied, "Inputs") else None,
           ["vina.vinafrag", "vina.vinain"])
    expect("parse arm Inputs files",    # the empty folder itself is made anyway
           sorted(os.listdir(os.path.join(parsed, "Inputs")))
           if isdir(parsed, "Inputs") else [], [])
    expect("parse arm hasinputs", meta(parsed, "hasinputs"), None)
    if "vina.vinain" not in (meta(copied, "hasinputs") or ""):
        problems.append("copy arm hasinputs does not list vina.vinain")
    expect("parse arm launch_totalprocs", meta(parsed, "launch_totalprocs"), "0")
    if meta(copied, "launch_totalprocs") in (None, "", "0"):
        problems.append("copy arm records no processors for its run")
    # The stored Vina output is the imported file.
    outs = [r for r in files(parsed) if r.startswith("Outputs/") and r.endswith(".out")]
    expect("parse arm Outputs", len(outs), 1)
    if outs and open(os.path.join(parsed, outs[0]), "rb").read() != \
            open(os.path.join(copied, "Outputs", "vina.vinaout"), "rb").read():
        problems.append("Outputs/vina.vinaout is not the imported output")

    # The ligand: same atoms in both arms, only its coordinates differ.
    try:
        rng = re.search(r"LIGAND_ATOMS=(\d+)-(\d+)", open(os.path.join(
            copied, "Inputs", "vina.vinain")).read())
        lo, hi = int(rng.group(1)) - 1, int(rng.group(2))
        pm = mvmAtoms(os.path.join(parsed, "Parameters", "chemsys.mvm"))
        cm = mvmAtoms(os.path.join(copied, "Parameters", "chemsys.mvm"))
        frag = mvmAtoms(os.path.join(copied, "Inputs", "vina.vinafrag"))
        trace = traceFrame1(os.path.join(parsed, "Props", "GEOMTRACE"))
    except (AttributeError, IndexError, OSError):
        problems.append("ligand check: cannot read vinain/vinafrag/mvm/GEOMTRACE")
        return problems
    expect("atom count, parse and copy", len(pm), len(cm))
    expect("element order, parse and copy", [a[0] for a in pm], [a[0] for a in cm])
    if not 0 <= lo < hi <= len(cm) or len(frag) != len(cm) or len(trace) != len(cm):
        problems.append("atom counts disagree: ligand %d-%d, mvm %d/%d, vinafrag "
                        "%d, GEOMTRACE %d" % (lo + 1, hi, len(pm), len(cm),
                                              len(frag), len(trace)))
        return problems
    xyz = lambda atoms: [a[1:] for a in atoms]
    expect("receptor atoms, parse and copy", pm[:lo], cm[:lo])
    d = maxDev(xyz(pm)[lo:hi], trace[lo:hi])
    if d > COORD_TOL:
        problems.append("parse arm ligand is not frame 1 of GEOMTRACE (%.4f)" % d)
    d = maxDev(xyz(cm)[lo:hi], xyz(frag)[lo:hi])
    if d > COORD_TOL:
        problems.append("copy arm ligand is not the ligand of vina.vinafrag (%.4f)" % d)
    if maxDev(xyz(pm)[lo:hi], xyz(cm)[lo:hi]) <= COORD_TOL:
        problems.append("the ligand is the same in both arms; the copy should "
                        "hold it before docking")
    return problems


def calcDirs(project):
    """{path within the project: path} of the calculations below a set's
    project (topics nest; every MD study has its own "dynamics" task)."""
    return {os.path.relpath(b, project): b for b, d, _f in os.walk(project)
            if "Props" in d}


def storedNames(setId):
    """Names of the calculations of a set that ship a stored copy.  Only these
    differ between the arms; the others (GROMACS studies) are parsed in both,
    in the background, so comparing them would compare timing."""
    root = os.path.join(HERE, "..", "..", "data", "client", "examples", setId)
    return {n for b, d, _f in os.walk(root) if os.path.basename(b) == "stored"
            for n in d}


def checkSet(work, setId, failures):
    project, count = SETS[setId]
    arms = {}
    for label, no in (("parse", True), ("copy", False)):
        rc, m, out = runArm(os.path.join(work, setId, label), no, setId)
        if rc == SKIP:
            print(out.strip())
            return SKIP
        if rc != 0 or not m:
            print("FAIL  %s %s arm: exit %d\n%s" % (setId, label, rc, out[-1500:]))
            failures.append("%s %s arm" % (setId, label))
            return 1
        arms[label] = m
        print("%-12s %-5s logged %s s, last property file after %s s; "
              "%s copied, %s parsed" % (setId, label, m.group(1), m.group(2),
                                        m.group(3), m.group(4)))
    if arms["copy"].group(3) != str(count) or arms["parse"].group(3) != "0":
        failures.append("%s copied/parsed counts: copy arm %s/%s, parse arm "
                        "%s/%s" % (setId, arms["copy"].group(3),
                                   arms["copy"].group(4),
                                   arms["parse"].group(3),
                                   arms["parse"].group(4)))
    pdir = os.path.join(work, setId, "parse", project)
    cdir = os.path.join(work, setId, "copy", project)
    pcalcs, ccalcs = calcDirs(pdir), calcDirs(cdir)
    if sorted(pcalcs) != sorted(ccalcs):
        failures.append("%s calculations differ: %s and %s"
                        % (setId, sorted(pcalcs), sorted(ccalcs)))
        return 0
    stored = storedNames(setId)
    for calc in sorted(pcalcs):
        if os.path.basename(calc) not in stored:
            continue
        problems = (compareDocking if setId == "docking" else compare)(
            pcalcs[calc], ccalcs[calc])
        if problems:
            failures.append("%s/%s: %d differences" % (setId, calc, len(problems)))
            print("FAIL  %s/%s\n      %s" % (setId, calc, "\n      ".join(
                problems[:25])))
        else:
            print("ok    %s/%s: copy and parse agree (%d files)"
                  % (setId, calc, len(files(pcalcs[calc]))))
    # An imported CREST run whose conformers the topology check all removed
    # carries the job store's warning, as a launched one does (#831).
    for label, calcs in (("parse", pcalcs), ("copy", ccalcs)):
        for calc, path in sorted(calcs.items()):
            if os.path.basename(calc) != "zinc_aqua_topology":
                continue
            reason = [v for (r, k), v in records(os.path.join(path, ".ecce-meta")).items()
                      if r == "." and k.endswith(":runStatusReason")]
            if not (reason and reason[0].startswith("All conformers were removed")):
                failures.append("%s/%s %s arm: no topology-check warning (%r)"
                                % (setId, calc, label, reason))
            else:
                print("ok    %s/%s %s arm: topology-check warning" % (setId, calc, label))
    return 0


def main():
    msg = apps.versionMismatch()
    if msg:
        print(msg)
        return SKIP
    # A fresh test home (offload hosts) has no ~/.cache yet.
    os.makedirs(os.path.expanduser("~/.cache"), exist_ok=True)
    work = tempfile.mkdtemp(prefix="examples-preparsed-",
                            dir=os.path.expanduser("~/.cache"))
    failures = []
    try:
        for setId in (sys.argv[1:] or SETS):
            rc = checkSet(work, setId, failures)
            if rc == SKIP:
                return SKIP
            if rc:
                break
    finally:
        if os.environ.get("ECCE_TEST_KEEP"):
            print("kept " + work)
        else:
            shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
