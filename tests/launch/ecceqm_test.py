#!/usr/bin/env python3
"""ECCE-QM, the bundled engine, end to end in local-data mode.

For water (B3LYP/6-31G*, closed shell) and O2 (triplet, restricted open-shell
B3LYP/6-31G*, ROKS) it does what a user's Calculation Editor and Launcher do,
without the windows:

  1. writes the calculation's molecule, basis and theory as ECCE stores them
     and generates the input deck with the real ai.ecceqm / std2ECCEQM;
  2. runs the bundled ecce-qm once on that deck and imports the output with
     the real ECCE-QM.expt (the import path), which gives the calculation its
     molecule, basis and theory;
  3. creates the calculation in a local data folder (ECCE_LOCAL_DATA, no data
     server), launches it on `localhost` through Launch, gensub and
     eccejobmonitor, and waits for it to complete;
  4. checks the stored total energy against the ORCA energy in
     tests/qm/oracle.json, and the properties the Builder needs;
  5. checks the orbitals as ECCE reads them back: ecce-mocomp rebuilds the
     overlap matrix from the stored basis and the code's MOOrdering, and every
     orbital must have c.S.c = 1, so trace(P S) is the electron count;
  6. checks that the MO correlation diagram is not offered for the code
     (ECCE-QM.edml SupportsMODiagram) and still is for another code.

    tests/launch/ecceqm_test.py [--build build-cmake] [--keep] [-v]

Exit status 77 (CTest SKIP) when a prerequisite is missing.
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import time
import types

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(REPO, "tests", "apps"))
sys.path.insert(0, os.path.join(REPO, "tests", "qm"))
sys.path.insert(0, HERE)

import isolate        # noqa: E402
import run_tests as launch   # noqa: E402  (tests/launch/run_tests.py)
from molecules import MOLECULES   # noqa: E402

CODE = "ECCE-QM"
RESOURCE = "ecceqm_es"
BASIS = "6-31G*"
FUNCTIONAL = "B3LYP"
ORACLE = os.path.join(REPO, "tests", "qm", "oracle.json")
BASIS_DIR = os.path.join(REPO, "data", "admin", "basissets")
PARSERS = os.path.join(REPO, "scripts", "parsers")

#  (name, molecule, oracle case, multiplicity, tolerance in hartree).  ORCA's
#  DefGrid3 and ours are different grids: the oracle tests allow 1e-5.
CASES = [
    ("water", "h2o", "h2o_6-31gs_b3lyp_r", 1, 1e-5),
    ("o2-triplet-roks", "o2", "o2_6-31gs_b3lyp_ro", 3, 1e-5),
]

say = launch.say
failures = []


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


# --- the calculation's files, as ECCE stores them -----------------------------

def libraryShells(basis):
    """{element: [(shell letters, [(exponent, [coefficients])])]} from the
    library file(s) a name such as 6-31G* stands for."""
    files = [basis.rstrip("*")]
    if basis.endswith("*"):
        files.append(basis.rstrip("*") + "S" * (len(basis) - len(basis.rstrip("*"))))
    table = {}
    for name in files:
        path = os.path.join(BASIS_DIR, name + ".BAS")
        element = None
        with open(path) as handle:
            lines = handle.read().splitlines()
        i = 0
        while i < len(lines):
            line = lines[i]
            m = re.match(r"atom=(\w+)", line)
            if m:
                element = m.group(1)
                table.setdefault(element, [])
            m = re.match(r"contraction shell=(\w+) num_primitives=(\d+)", line)
            if m and element:
                rows = []
                for k in range(int(m.group(2))):
                    i += 1
                    rows.append(lines[i].split())
                table[element].append((m.group(1), rows))
            i += 1
    return table


def writeGbs(path, elements, basis):
    """The .gbs in the form TGBSConfig::dump() writes: names, then numbers."""
    table = libraryShells(basis)
    with open(path, "w") as out:
        out.write('NameBasis\nbasis "ao basis" spherical print\n')
        for el in elements:
            out.write('%s library "%s"\n' % (el, basis))
        out.write("END\nEndNameBasis\n\nNumericalBasis\n"
                  'basis "ao basis" spherical print\n')
        for el in elements:
            for shell, rows in table[el]:
                out.write("%s %s\n" % (el, shell))
                for row in rows:
                    out.write("  " + "  ".join(row) + "\n")
        out.write("END\nEndNumericalBasis\n")


def writeCalcFiles(work, name, mol, mult):
    charge, _mult, atoms = MOLECULES[mol]
    with open(os.path.join(work, name + ".frag"), "w") as out:
        out.write("fragment\ntitle: %s\nnum_atoms: %d\natom_info: symbol\n"
                  "atom_list:\n" % (name, len(atoms)))
        for a in atoms:
            out.write("%s %.10f %.10f %.10f\n" % a)
    with open(os.path.join(work, name + ".param"), "w") as out:
        out.write("Category: DFT\nTheory: DFT\nRunType: Energy\n"
                  "Charge: %d\nChemSys.Multiplicity: %d\n"
                  "ES.Theory.DFT.XCFunctionals: %s\n" % (charge, mult, FUNCTIONAL))
    elements = sorted({a[0] for a in atoms})
    writeGbs(os.path.join(work, name + ".gbs"), elements, BASIS)


def perl(env, script, *args, stdin=None):
    with open(stdin) if stdin else open(os.devnull) as handle:
        result = subprocess.run(["perl", os.path.join(PARSERS, script)] + list(args),
                                stdin=handle, env=env, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT)
    return result.returncode, result.stdout.decode("utf-8", "replace")


def generateDeck(env, work, name):
    """The input deck as the Calculation Editor makes it."""
    rc, out = perl(env, "std2ECCEQM", stdin=os.path.join(work, name + ".gbs"))
    with open(os.path.join(work, name + ".basis"), "w") as handle:
        handle.write(out)
    check(rc == 0 and out.startswith("basis "), "std2ECCEQM wrote the basis (rc=%d)" % rc)
    shutil.copy(os.path.join(PARSERS, "ecceqm.tpl"), os.path.join(work, name + ".tpl"))
    rc, out = subprocess.run(
        ["perl", os.path.join(PARSERS, "ai.ecceqm"), "-p", "-f", "-b", "-n", name,
         "-t", name + ".tpl"], cwd=work, env=env, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT).returncode, ""
    check(rc == 0, "ai.ecceqm wrote the deck (rc=%d)" % rc)
    deck = os.path.join(work, name + ".tpl")
    with open(deck) as handle:
        text = handle.read()
    check("method b3lyp\n" in text and "shell O S\n" in text and "geometry\n" in text,
          "the deck names the method, carries the basis and the geometry")
    final = os.path.join(work, "ecceqm.qmin")
    shutil.move(deck, final)
    return final


def importFromOutput(env, work, name, qm, deck):
    """Run ecce-qm once on the deck and import its output, as Import
    Calculation would: ECCE-QM.expt writes molecule, theory and basis."""
    out = os.path.join(work, name + ".qmout")
    with open(out, "w") as handle:
        rc = subprocess.run([qm, deck], stdout=handle, stderr=subprocess.STDOUT,
                            env=env).returncode
    check(rc == 0, "ecce-qm ran on the deck (rc=%d)" % rc)
    rc, text = perl(env, "ECCE-QM.expt", out)
    check(rc == 0, "ECCE-QM.expt imported the output (rc=%d) %s" % (rc, text.strip()))
    return out


# --- reading the results back -----------------------------------------------

def propertyValue(calc, key):
    path = os.path.join(calc, "Props", key)
    with open(path, errors="replace") as handle:
        text = handle.read()
    m = re.search(r"<value[^>]*>\s*([-+0-9.eE]+)\s*</value>", text) or \
        re.search(r">\s*([-+]?\d+\.\d+(?:[eE][-+]?\d+)?)\s*<", text)
    return float(m.group(1)) if m else None


def mocomp(env, build, calc, *args):
    exe = os.path.join(build, "mocomp")
    result = subprocess.run([exe, calc] + list(args), env=env,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    return result.returncode, result.stdout.decode("utf-8", "replace")


def checkOrbitals(env, build, calc, nelectrons):
    rc, out = mocomp(env, build, calc, "--list")
    rows = [l.split("\t") for l in out.splitlines() if l and l[0].isdigit()]
    if not check(rc == 0 and rows, "ecce-mocomp lists the orbitals (%d)" % len(rows)):
        say(out)
        return
    occupations = [float(r[2]) for r in rows]
    check(abs(sum(occupations) - nelectrons) < 1e-6,
          "the occupations add up to %d electrons (%.6f)" % (nelectrons, sum(occupations)))
    norms = []
    for n in range(1, len(rows) + 1):
        rc, out = mocomp(env, build, calc, "--mo", str(n), "--by", "atom")
        m = re.search(r"c\.S\.c = ([-0-9.]+)", out)
        norms.append(float(m.group(1)) if m else float("nan"))
    worst = max(abs(x - 1.0) for x in norms)
    check(worst < 2e-3, "c.S.c = 1 for every one of %d orbitals, worst %.5f"
          % (len(norms), worst))
    trace = sum(o * x for o, x in zip(occupations, norms))
    check(abs(trace - nelectrons) < 5e-3,
          "trace(P S) = %.4f for %d electrons" % (trace, nelectrons))


def checkDiagramNotOffered(build):
    """The panel's isRelevant() asks JCode::supportsMODiagram(); the .edml
    carries the flag, per code."""
    with open(os.path.join(REPO, "data", "client", "cap", "ECCE-QM.edml")) as handle:
        mine = handle.read()
    with open(os.path.join(REPO, "data", "client", "cap", "ORCA.edml")) as handle:
        other = handle.read()
    check(re.search(r"<SupportsMODiagram>\s*false\s*</SupportsMODiagram>", mine)
          is not None, "ECCE-QM.edml turns the MO diagram off")
    check("SupportsMODiagram" not in other,
          "ORCA.edml does not (the flag defaults to on)")


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    parser.add_argument("--keep", action="store_true")
    parser.add_argument("--export", metavar="DIR",
                        help="copy each finished calculation folder to DIR "
                        "(how tests/ecceqm/fixtures was made)")
    parser.add_argument("-v", "--verbose", action="store_true")
    opts = parser.parse_args()
    build = os.path.abspath(opts.build)

    qm = os.path.join(build, "src", "qm", "ecce-qm")
    for exe in ("launchjob", "eccejobstore", "eccejobmaster", "ecmd", "mocomp"):
        if not os.access(os.path.join(build, exe), os.X_OK):
            launch.skip("%s is not built in %s" % (exe, build))
    if not os.access(qm, os.X_OK):
        launch.skip("ecce-qm is not built (cmake -DECCE_BUILD_QM=ON)")
    for tool in ("mosquitto", "perl"):
        if not shutil.which(tool):
            launch.skip("%s is not installed" % tool)
    if not os.path.exists(os.path.join(build, "siteconfig-local", "DataServers")):
        launch.skip("%s/siteconfig-local is missing (run cmake)" % build)
    with open(ORACLE) as handle:
        oracle = json.load(handle)

    state = isolate.resolveStateDir(isolate.runState("ecceqm", keep=opts.keep))
    os.makedirs(state, exist_ok=True)
    launch.sweep(state)
    for sub in ("jobs", "tmp", "folder", "localdata", "work"):
        shutil.rmtree(os.path.join(state, sub), ignore_errors=True)
    os.makedirs(os.path.join(state, "tmp"))
    os.makedirs(os.path.join(state, "work"))
    os.environ["ECCE_TMPDIR"] = os.path.join(state, "tmp")
    install = launch.treeInstall(state, build)
    #  The bundled engine sits next to the other programs, as in a package.
    launch.link(qm, os.path.join(install, "bin", "ecce-qm"))
    launch.link(os.path.join(build, "mocomp"), os.path.join(install, "bin", "ecce-mocomp"))
    settings = isolate.apply(install, state)
    home = settings["ECCE_HOME"]
    os.unlink(os.path.join(home, "siteconfig", "DataServers"))
    os.environ["ECCE_TEST_HOME"] = install
    say(isolate.describe(settings))

    #  No CONFIG.localhost and no path for the code: the engine is found where
    #  ECCE put it.
    args = types.SimpleNamespace(
        local=True, folder=False, machine="localhost", cwd=None,
        legacy_transport=None, job_comms=None, shared_connection=False,
        hold=False, drop=False, remote_user="", expect_keepalive=False)
    suite = launch.Suite(args, build, state, home)
    for name in ("eccejobmaster", "eccejobstore", "launchjob"):
        path = os.path.realpath(os.path.join(home, "bin", name))
        check(path == os.path.join(build, name), "%s -> %s" % (name, path))
    check(os.path.realpath(os.path.join(home, "bin", "ecce-qm")) == os.path.realpath(qm),
          "ecce-qm is the one in the build tree")

    env = suite.env()
    env["ECCE_HOME"] = REPO
    try:
        if not suite.services(True):
            check(False, "services started")
        else:
            rc, out = suite.driver("machines", CODE)
            check("localhost" in out.split(),
                  "the Launcher offers localhost for %s with nothing registered (%s)"
                  % (CODE, " ".join(out.split())))
            for name, mol, key, mult, tol in CASES:
                runCase(suite, env, build, state, oracle, name, mol, key, mult, tol, qm,
                        opts.export)
            checkDiagramNotOffered(build)
    finally:
        if not opts.keep:
            suite.services(False)
            note, n = launch.sweep(state)
            left = launch.procsUnder(state)
            say("cleanup: %d extra process(es) stopped, %d left" % (n, len(left)))
            if left:
                failures.append("processes left running: %r" % left)

    say("")
    if failures:
        say("FAILED (%d): %s" % (len(failures), "; ".join(failures)))
        return 1
    say("PASSED")
    return 0


def runCase(suite, env, build, state, oracle, name, mol, key, mult, tol, qm,
                        opts.export):
    say("--- %s" % name)
    work = os.path.join(state, "work", name)
    os.makedirs(work)
    writeCalcFiles(work, name, mol, mult)
    deck = generateDeck(env, work, name)
    output = importFromOutput(env, work, name, qm, deck)

    rundir = os.path.join(state, "jobs")
    os.makedirs(rundir, exist_ok=True)
    calcName = "ecceqm-%s-%d" % (name, int(time.time()))
    rc, out = suite.driver("create", suite.userUrl(), calcName, RESOURCE, deck,
                           "ecceqm.qmin", "localhost", rundir, suite.user())
    if not check(rc == 0, "calculation created"):
        say(out)
        return
    url = out.strip().splitlines()[-1]
    #  The import reads <dir>/<root>.frag/.param/.gbs, written by ECCE-QM.expt.
    rc, out = suite.driver("setup", url, work, os.path.basename(output))
    if not check(rc == 0, "molecule, basis and theory stored (setup)"):
        say(out)
        return
    rc, out = suite.driver("launch", url)
    say("\n".join("  | " + l for l in out.strip().splitlines()[-8:]))
    if not check(rc == 0, "Launch ran to the end"):
        return
    state_ = suite.waitState(url)
    check(state_ == "completed", "the run reached completed (last: %s)" % state_)
    if state_ != "completed":
        suite.diagnose(url)
        return
    time.sleep(2)
    props = suite.props(url)
    say("  properties: " + " ".join(props))
    for prop in ("TE", "MO", "ORBENG", "ORBOCC", "MULLIKEN", "DIPOLE", "NNREPUL",
                 "ONEELEC", "COULOMB", "EXCORR"):
        check(prop in props, "%s present in Props/" % prop)

    calc = url[len("file://"):] if url.startswith("file://") else url
    calc = calc.rstrip("/")
    outputs = os.listdir(os.path.join(calc, "Outputs"))
    check("ecceqm.qmout" in outputs, "the output file was stored in Outputs/")
    energy = propertyValue(calc, "TE")
    want = oracle[key]["orca"]["energy"]
    check(energy is not None and abs(energy - want) < tol,
          "total energy %.8f against ORCA %.8f (%s, tolerance %g)"
          % (energy if energy is not None else float("nan"), want,
             oracle[key]["orca"]["keywords"].split()[0], tol))
    if export:
        target = os.path.join(export, name)
        shutil.rmtree(target, ignore_errors=True)
        shutil.copytree(calc, target, symlinks=True)
    checkOrbitals(suite.env(), build, calc,
                  sum(Z for Z in [{"O": 8, "H": 1}[a[0]] for a in MOLECULES[mol][2]]))


if __name__ == "__main__":
    sys.exit(main())
