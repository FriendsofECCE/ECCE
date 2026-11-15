#!/usr/bin/env python3
"""ecce-npa against NBO 7's NPA (#1054): every fixture's natural charges (and
spin populations, open shell) within 0.005 e of NBO's from the same ORCA run.
Fixtures: tests/npa/make_fixtures.py.  Prints one ok/FAIL line per check.

    python3 tests/npa/run_tests.py <ecce-npa>
"""
import glob, json, os, subprocess, sys

TOL = 0.005
exe = sys.argv[1]
here = os.path.dirname(os.path.abspath(__file__))
env = dict(os.environ)
env.setdefault("ECCE_HOME", os.path.dirname(os.path.dirname(here)))   # the CLI asserts it is set
fails = 0


def check(ok, what):
    global fails
    print(("ok   " if ok else "FAIL ") + what, flush=True)
    fails += 0 if ok else 1


dirs = sorted(glob.glob(os.path.join(here, "fixtures", "*", "nbo_reference.json")))
check(len(dirs) >= 10, "%d fixtures" % len(dirs))
print("%-12s %4s %-3s %10s %10s %9s" % ("fixture", "atom", "", "ECCE", "NBO 7", "diff"))
for ref_path in dirs:
    d = os.path.dirname(ref_path)
    name = os.path.basename(d)
    ref = json.load(open(ref_path))
    p = subprocess.run([exe, os.path.join(d, "results.h5"), "--json"], env=env,
                       capture_output=True, text=True)
    if p.returncode:
        check(False, "%s: ecce-npa failed: %s" % (name, p.stderr.strip()))
        continue
    got = json.loads(p.stdout)
    atoms = got["atoms"]
    check(len(atoms) > 0 and [a["element"] for a in atoms] == ref["elements"], "%s: atoms in NBO's order" % name)
    worst = 0.0
    for i, (a, q) in enumerate(zip(atoms, ref["charge"])):
        diff = a["charge"] - q
        worst = max(worst, abs(diff))
        print("%-12s %4d %-3s %10.5f %10.5f %9.5f" % (name, i + 1, a["element"], a["charge"], q, diff))
    check(worst <= TOL, "%s: charges within %.3f e of NBO (worst %.5f)" % (name, TOL, worst))
    if "spin" in ref:
        ws = max(abs(a["spin"] - s) for a, s in zip(atoms, ref["spin"]))
        check(ws <= TOL, "%s: spin populations within %.3f of NBO (worst %.5f)" % (name, TOL, ws))
    if "I" in ref["elements"]:   # the job store records the ECP (#1055)
        i = ref["elements"].index("I")
        check(atoms[i]["ecp_core"] == 28 and "note" not in got,
              "%s: I ECP core %d read from results.h5%s" % (name, atoms[i]["ecp_core"],
                                                          " (guessed: %s)" % got["note"] if "note" in got else ""))
    check(abs(sum(a["charge"] for a in atoms) - round(sum(ref["charge"]))) < 1e-4,
          "%s: charges sum to the molecular charge" % name)

print("summary: %d failure(s)" % fails)
sys.exit(1 if fails else 0)
