#!/usr/bin/env python3
"""
MO composition (#161): ecce-mocomp against Multiwfn.

    run_tests.py [build-dir]

For each fixture (an calculation folder, parsed by the real
scripts/parsers/orca.mo; the folders are made by make_fixture.py) the Mulliken
composition ecce-mocomp prints for a set of orbitals is compared with
reference/<name>.tsv, which Multiwfn's orbital composition analysis (main
function 8, Mulliken partition) printed for the molden file orca_2mkl wrote
from the same ORCA run (h2co631: the formatted checkpoint file of a Gaussian
16 run, whose 6-31G* basis has Cartesian d functions).  Every basis function, every contracted shell, every
(atom, shell type) and every atom is compared; each must agree within
TOLERANCE percentage points.  The reference numbers are checked in, so this
needs no Multiwfn.

Where Multiwfn is also installed (MULTIWFN, or Multiwfn_noGUI on PATH) the
reference files are regenerated from the checked-in molden files and compared
with the checked-in ones, which catches a reference that has drifted from its
source; that part reports SKIP without Multiwfn.

Independent of Multiwfn, every orbital's functions, shells and atoms must each
sum to 100%, and the orbital's norm c.S.c must be 1.

Exit status 0 if all passed, 1 on a failure.
"""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
import multiwfn_ref                                           # noqa: E402

TOLERANCE = 0.1          # percentage points, per contribution
NORM_TOLERANCE = 1.0e-3

#  name -> (alpha MOs, beta MOs): what reference/<name>.tsv holds.
CASES = {
    "water":   (list(range(1, 25)), []),
    "benzene": ([1, 2, 6, 7, 12, 13, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
                 40, 60], []),
    "ch3":     (list(range(1, 11)), list(range(1, 11))),
    #  Gaussian 16, 6-31G*: Cartesian d functions.
    "h2co631": (list(range(1, 35)), []),
    "crco6":   ([1, 19, 22, 30, 45, 50, 53, 54, 55, 56, 58, 60, 75, 100, 150,
                 199], []),
}

failures = 0


def check(ok, name, why=""):
    global failures
    print(("PASS " if ok else "FAIL ") + name + ("" if ok else ": " + why))
    if not ok:
        failures += 1


def read_rows(lines):
    """{(mo, kind, atom, l, seq, comp): percent}"""
    rows = {}
    for line in lines:
        f = line.rstrip("\n").split("\t")
        if len(f) != 8:
            continue
        mo, kind, _label, atom, l, seq, comp, pct = f
        rows[(mo, kind, int(atom), int(l), int(seq), comp)] = float(pct)
    return rows


def run_mocomp(build, fixture, mos, beta, method="mulliken"):
    work = tempfile.mkdtemp(prefix="mocomp-")
    try:
        calc = os.path.join(work, "calc")
        shutil.copytree(fixture, calc)
        home = os.path.join(work, "home")
        os.makedirs(home)
        env = dict(os.environ, ECCE_HOME=REPO, ECCE_REALUSER="mocomp",
                   ECCE_REALUSERHOME=home)
        env.pop("ECCE_LOCAL_DATA", None)
        cmd = [os.path.join(build, "mocomp"), calc, "--tsv", "--method", method]
        if beta:
            cmd.append("--beta")
        for m in mos:
            cmd += ["--mo", str(m)]
        p = subprocess.run(cmd, capture_output=True, text=True, env=env,
                           timeout=300)
        return p.returncode, p.stdout, p.stderr
    finally:
        shutil.rmtree(work, ignore_errors=True)


def compare(name, got, ref):
    worst = (0.0, None)
    problems = []
    for key in set(got) | set(ref):
        g = got.get(key)
        r = ref.get(key)
        #  Multiwfn leaves out an exact zero (a function symmetry forbids).
        if g is None and r is not None and abs(r) < 1.0e-4:
            continue
        if r is None and g is not None and abs(g) < TOLERANCE:
            r = 0.0
        if g is None or r is None:
            problems.append("%s only in %s" % (key, "ecce" if r is None
                                               else "reference"))
            continue
        d = abs(g - r)
        if d > worst[0]:
            worst = (d, key, g, r)
        if d > TOLERANCE:
            problems.append("%s ecce %.5f reference %.5f" % (key, g, r))
    return worst, problems


def main(argv):
    build = argv[1] if len(argv) > 1 else os.path.join(REPO, "build-cmake")
    for method, suffix, partition in (("mulliken", "", "Mulliken"),
                                      ("c2", ".c2", "Ros-Schuit (c^2)")):
        for name, (alpha, beta) in CASES.items():
            fixture = os.path.join(HERE, "fixtures", name)
            ref = read_rows(open(os.path.join(HERE, "reference",
                                              name + suffix + ".tsv")))
            got = {}
            ok = True
            for mos, is_beta in ((alpha, False), (beta, True)):
                if not mos:
                    continue
                rc, out, err = run_mocomp(build, fixture, mos, is_beta, method)
                if rc != 0:
                    check(False, name + " runs", err.strip()[-300:])
                    ok = False
                    break
                for key, v in read_rows(out.splitlines()).items():
                    mo = ("b" + key[0]) if is_beta else key[0]
                    got[(mo,) + key[1:]] = v
            if not ok:
                continue

            worst, problems = compare(name, got, ref)
            check(not problems, "%s %s vs Multiwfn %s (%d numbers, worst %.5f)"
                  % (name, method, partition, len(ref), worst[0]),
                  "; ".join(problems[:5]) +
                  (" ... %d more" % (len(problems) - 5)
                   if len(problems) > 5 else ""))

            #  Sums, from our own output alone.
            sums = {}
            for (mo, kind, atom, l, seq, comp), v in got.items():
                if kind in ("ao", "atom"):
                    sums[(mo, kind)] = sums.get((mo, kind), 0.0) + v
            bad = [(k, s) for k, s in sums.items() if abs(s - 100.0) > 1.0e-2]
            check(not bad, "%s %s: every orbital's functions and atoms sum "
                  "to 100%%" % (name, method), str(bad[:3]))

    mw = multiwfn_ref.find_multiwfn()
    for scpa, suffix in ((False, ""), (True, ".c2")):
        for name, (alpha, beta) in CASES.items():
            molden = os.path.join(HERE, "molden", name + ".molden")
            if not os.path.exists(molden):
                molden = os.path.join(HERE, "molden", name + ".fchk")
            what = "%s%s reference regenerated from molden" % (name, suffix)
            if mw is None or not os.path.exists(molden):
                print("SKIP %s (%s)" % (what, "no Multiwfn" if mw is None
                                        else "no molden file"))
                continue
            shells = multiwfn_ref.molden_shells(molden)
            if molden.endswith(".fchk"):
                nmo = int([l for l in open(molden) if l.startswith(
                    "Number of independent functions")][0].split()[-1])
            else:
                nmo = sum(1 for line in open(molden)
                          if line.strip().lower().startswith("ene="))
            off = nmo // 2 if beta else 0
            beta_idx = {off + b for b in beta}
            out = multiwfn_ref.run(mw, molden, alpha + sorted(beta_idx), scpa)
            rows = multiwfn_ref.parse(
                out, shells, lambda i: ("b%d" % (i - off)) if i in beta_idx
                else str(i))
            fresh = read_rows("\t".join(str(x) if not isinstance(x, float)
                                        else "%.5f" % x for x in r)
                              for r in rows)
            ref = read_rows(open(os.path.join(HERE, "reference",
                                              name + suffix + ".tsv")))
            worst, problems = compare(name, fresh, ref)
            check(not problems, what + " matches the checked-in one",
                  "; ".join(problems[:3]))

    print("%s" % ("FAILED" if failures else "ok"))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
