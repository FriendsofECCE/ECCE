#!/usr/bin/env python3
"""ECCE-QM end to end on Windows, from an installed tree (#133).

For water (B3LYP/6-31G*) and O2 (triplet, ROKS B3LYP/6-31G*), as
tests/launch/ecceqm_test.py does on Linux, with the tree's own perl, python
and programs and the session broker ecce.cmd starts:
  1. the calculation's molecule, basis and theory as ECCE stores them, and
     the input deck from the real ai.ecceqm / std2ECCEQM;
  2. ecce-qm once on the deck and ECCE-QM.expt on its output (the files the
     Calculation Editor stores);
  3. launchjob create/setup/launch on localhost (the Launcher's Launch code),
     waiting for completed;
  4. the total energy against tests/qm/oracle.json (ORCA), the properties,
     and every orbital normalised (mocomp: c.S.c = 1, trace(P S) = N);
  5. the Builder opened on the calculation draws an orbital (scene hook
     "mopanel compute", then "mogrid": a grid with a field range).
Run it on a desktop session (the Builder needs one):

    python ecceqm_win.py <install tree> <state dir>
"""
import json
import os
import re
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(REPO, "tests", "qm"))
from molecules import MOLECULES   # noqa: E402

ORACLE = os.path.join(REPO, "tests", "qm", "oracle.json")
CASES = [("water", "h2o", "h2o_6-31gs_b3lyp_r", 1, 1e-5),
         ("o2-triplet-roks", "o2", "o2_6-31gs_b3lyp_ro", 3, 1e-5)]
Z = {"H": 1, "C": 6, "N": 7, "O": 8}
failures = []


def say(t):
    print(t, flush=True)


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


tree, state = (os.path.abspath(a).replace("\\", "/") for a in sys.argv[1:3])
shutil.rmtree(state, ignore_errors=True)
for d in (".ECCE", "tmp", "jobs", "work"):
    os.makedirs(os.path.join(state, d))
BIN = tree + "/bin"
PARSERS = tree + "/scripts/parsers"
sysdir = os.environ.get("SystemRoot", r"C:\Windows")
env = {k: v for k, v in os.environ.items() if not k.startswith("ECCE_")}
env.update({
    "ECCE_HOME": tree, "ECCE_REALUSERHOME": state,
    "ECCE_REALUSER": os.environ.get("USERNAME", "user"),
    "HOST": os.environ.get("COMPUTERNAME", "localhost"),
    "ECCE_TMPDIR": state + "/tmp", "ECCE_LOCAL_DATA": state + "/localdata",
    "ECCE_NO_DATASERVER": "1", "ECCE_SESSION_LIVENESS": "lease",
    "ECCE_SESSION_ID": os.urandom(8).hex(),
    "PATH": os.pathsep.join([BIN, tree + "/scripts", PARSERS, tree + "/usr/bin",
                             tree + "/python", tree + "/strawberry/perl/site/bin",
                             tree + "/strawberry/perl/bin", tree + "/strawberry/c/bin",
                             sysdir + r"\System32", sysdir]),
})
PERL = tree + "/strawberry/perl/bin/perl.exe"
BASH = tree + "/usr/bin/bash.exe"


def run(argv, **kw):
    r = subprocess.run(argv, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, **kw)
    return r.returncode, r.stdout.decode("utf-8", "replace")


def drv(*a, timeout=300):
    return run([BIN + "/launchjob.exe", "-pipe", os.devnull] + list(a), cwd=BIN, timeout=timeout)


# --- the calculation's files (as tests/launch/ecceqm_test.py) -----------------

def libraryShells(basis):
    files = [basis.rstrip("*")]
    if basis.endswith("*"):
        files.append(basis.rstrip("*") + "S" * (len(basis) - len(basis.rstrip("*"))))
    table, element = {}, None
    for name in files:
        lines = open(os.path.join(tree, "data", "admin", "basissets", name + ".BAS")).read().splitlines()
        i = 0
        while i < len(lines):
            m = re.match(r"atom=(\w+)", lines[i])
            if m:
                element = m.group(1)
                table.setdefault(element, [])
            m = re.match(r"contraction shell=(\w+) num_primitives=(\d+)", lines[i])
            if m and element:
                rows = []
                for _ in range(int(m.group(2))):
                    i += 1
                    rows.append(lines[i].split())
                table[element].append((m.group(1), rows))
            i += 1
    return table


def writeCalcFiles(work, name, mol, mult):
    charge, _m, atoms = MOLECULES[mol]
    with open(os.path.join(work, name + ".frag"), "w", newline="\n") as out:
        out.write("fragment\ntitle: %s\nnum_atoms: %d\natom_info: symbol\natom_list:\n"
                  % (name, len(atoms)))
        for a in atoms:
            out.write("%s %.10f %.10f %.10f\n" % a)
    with open(os.path.join(work, name + ".param"), "w", newline="\n") as out:
        out.write("Category: DFT\nTheory: DFT\nRunType: Energy\nCharge: %d\n"
                  "ChemSys.Multiplicity: %d\nES.Theory.DFT.XCFunctionals: B3LYP\n"
                  % (charge, mult))
    elements = sorted({a[0] for a in atoms})
    table = libraryShells("6-31G*")
    with open(os.path.join(work, name + ".gbs"), "w", newline="\n") as out:
        out.write('NameBasis\nbasis "ao basis" spherical print\n')
        for el in elements:
            out.write('%s library "6-31G*"\n' % el)
        out.write("END\nEndNameBasis\n\nNumericalBasis\nbasis \"ao basis\" spherical print\n")
        for el in elements:
            for shell, rows in table[el]:
                out.write("%s %s\n" % (el, shell))
                for row in rows:
                    out.write("  " + "  ".join(row) + "\n")
        out.write("END\nEndNumericalBasis\n")


def generateDeck(work, name):
    with open(os.path.join(work, name + ".gbs")) as gbs:
        r = subprocess.run([PERL, PARSERS + "/std2ECCEQM"], stdin=gbs, env=env,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    text = r.stdout.decode("utf-8", "replace")
    with open(os.path.join(work, name + ".basis"), "w", newline="\n") as h:
        h.write(text)
    check(r.returncode == 0 and text.startswith("basis "), "std2ECCEQM wrote the basis")
    shutil.copy(PARSERS + "/ecceqm.tpl", os.path.join(work, name + ".tpl"))
    rc, out = run([PERL, PARSERS + "/ai.ecceqm", "-p", "-f", "-b", "-n", name, "-t", name + ".tpl"],
                  cwd=work)
    check(rc == 0, "ai.ecceqm wrote the deck (rc=%d) %s" % (rc, out.strip()[-200:]))
    final = os.path.join(work, "ecceqm.qmin")
    shutil.move(os.path.join(work, name + ".tpl"), final)
    deck = open(final).read()
    check("method b3lyp" in deck and "geometry" in deck, "the deck names the method and the geometry")
    return final


def propertyValue(calc, key):
    text = open(os.path.join(calc, "Props", key), errors="replace").read()
    m = re.search(r"<value[^>]*>\s*([-+0-9.eE]+)\s*</value>", text) or \
        re.search(r">\s*([-+]?\d+\.\d+(?:[eE][-+]?\d+)?)\s*<", text)
    return float(m.group(1)) if m else None


def checkOrbitals(calc, nelectrons):
    rc, out = run([BIN + "/mocomp.exe", calc, "--list"])
    rows = [l.split("\t") for l in out.splitlines() if l and l[0].isdigit()]
    if not check(rc == 0 and rows, "mocomp lists the orbitals (%d)" % len(rows)):
        say(out[-500:])
        return
    occ = [float(r[2]) for r in rows]
    check(abs(sum(occ) - nelectrons) < 1e-6, "occupations add up to %d (%.6f)" % (nelectrons, sum(occ)))
    norms = []
    for n in range(1, len(rows) + 1):
        _rc, out = run([BIN + "/mocomp.exe", calc, "--mo", str(n), "--by", "atom"])
        m = re.search(r"c\.S\.c = ([-0-9.]+)", out)
        norms.append(float(m.group(1)) if m else float("nan"))
    worst = max(abs(x - 1.0) for x in norms)
    check(worst < 2e-3, "c.S.c = 1 for all %d orbitals (worst %.5f)" % (len(norms), worst))
    trace = sum(o * x for o, x in zip(occ, norms))
    check(abs(trace - nelectrons) < 5e-3, "trace(P S) = %.4f for %d electrons" % (trace, nelectrons))


def builderDrawsOrbital(url, name):
    out = os.path.join(state, "scene-" + name)
    os.makedirs(out)
    script = os.path.join(out, "scene")
    with open(script, "w", newline="\n") as h:
        h.write("mopanel compute\nmogrid grid\nsnap mo-%s\n" % name)
    e = dict(env, ECCE_VIEWER_SCENE=script, ECCE_VIEWER_SCENE_OUT=out)
    try:
        r = subprocess.run([BIN + "/builder.exe", "-context", url], env=e, timeout=240,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        rc = r.returncode
    except subprocess.TimeoutExpired:
        rc = "timeout"
    grid = os.path.join(out, "grid.txt")
    text = open(grid).read() if os.path.exists(grid) else ""
    failed = open(os.path.join(out, "FAILED")).read() if os.path.exists(os.path.join(out, "FAILED")) else ""
    m = re.search(r"fieldMin (\S+)\s+fieldMax (\S+)", text)
    check(m is not None and float(m.group(1)) < 0 < float(m.group(2)) and not failed,
          "the Builder computes an orbital grid (rc=%s, %s%s)" % (rc, " ".join(text.split()), failed))
    pics = [f for f in os.listdir(out) if f.endswith((".ppm", ".png"))]
    check(bool(pics), "the Builder drew frames: %s" % " ".join(sorted(pics)[:4]))


def main():
    oracle = json.load(open(ORACLE))
    rc, out = run([BASH, tree + "/bin/ecce-broker-win", "start"])
    say(out.strip())
    if not check(rc == 0, "session broker started"):
        return 1
    try:
        rc, out = drv("machines", "ECCE-QM")
        check("localhost" in out.split(), "the Launcher offers localhost for ECCE-QM (%s)" % " ".join(out.split()))
        for name, mol, key, mult, tol in CASES:
            say("--- " + name)
            work = os.path.join(state, "work", name)
            os.makedirs(work)
            writeCalcFiles(work, name, mol, mult)
            deck = generateDeck(work, name)
            output = os.path.join(work, name + ".qmout")
            with open(output, "w") as h:
                r = subprocess.run([BIN + "/ecce-qm.exe", deck], env=env, stdout=h, stderr=subprocess.STDOUT)
            check(r.returncode == 0, "ecce-qm ran on the deck")
            rc, out = run([PERL, PARSERS + "/ECCE-QM.expt", output], cwd=work)
            check(rc == 0, "ECCE-QM.expt read the output %s" % out.strip()[-200:])
            rc, out = drv("create", "file://%s/localdata/users/local" % state, "ecceqm-" + name,
                          "ecceqm_es", deck, "ecceqm.qmin", "localhost", state + "/jobs", env["ECCE_REALUSER"])
            if not check(rc == 0, "calculation created"):
                say(out)
                continue
            url = out.strip().splitlines()[-1]
            rc, out = drv("setup", url, work, os.path.basename(output))
            if not check(rc == 0, "molecule, basis and theory stored"):
                say(out)
                continue
            rc, out = drv("launch", url)
            if not check(rc == 0, "Launch ran to the end"):
                say(out)
                continue
            st = ""
            for _ in range(300):
                st = drv("state", url)[1].strip().splitlines()[-1:] or [""]
                st = st[0]
                if st in ("completed", "failed", "killed", "unsuccessful", "system_failure"):
                    break
                time.sleep(2)
            if not check(st == "completed", "the run completed (%s)" % st):
                say(drv("reason", url)[1])
                continue
            time.sleep(2)
            props = drv("props", url)[1].split()
            for p in ("TE", "MO", "ORBENG", "ORBOCC", "MULLIKEN", "DIPOLE"):
                check(p in props, "%s stored" % p)
            calc = url[len("file://"):].rstrip("/")
            e = propertyValue(calc, "TE")
            want = oracle[key]["orca"]["energy"]
            check(e is not None and abs(e - want) < tol,
                  "total energy %.8f against ORCA %.8f (tolerance %g)" % (e if e is not None else float("nan"), want, tol))
            checkOrbitals(calc, sum(Z[a[0]] for a in MOLECULES[mol][2]) - MOLECULES[mol][0])
            builderDrawsOrbital(url, name)
    finally:
        run([BASH, tree + "/bin/ecce-broker-win", "stop"])
    say("")
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
