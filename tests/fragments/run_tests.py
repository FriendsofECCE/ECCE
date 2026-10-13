#!/usr/bin/env python3
"""
Fragment-orbital analysis (#201): ecce-fragments against Multiwfn's CDA module.

    run_tests.py [build-dir]

Cr(CO)6, ORCA 6.1.1 RHF/def2-SVP, split into Cr and (CO)6.  Both fragments are
closed-shell singlet neutrals at the complex geometry, as the complex is; the
neutral Cr atom converged to an arbitrary d4s2-like closed-shell state, which
only matters for the Cr fragment's own orbital energies.  fixtures/ are
calculation folders made as README.md says; oracle.json is what Multiwfn
(main function 16, Mulliken and SCPA) printed for the same three runs
(gen_oracle.py).  Every fragment-orbital composition of parent MOs 30-90 must
agree within TOLERANCE percentage points; then the chemistry a chemist
checks first, the library's own consistency checks, and that a wrong
fragment set is refused rather than analysed.

Exit status 0 if all passed, 1 on a failure.
"""
import json
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
TOLERANCE = 0.5          # percentage points per contribution
CHECK_TOLERANCE = 1.0e-5  # ORCA prints MO coefficients to 6 decimals
failures = 0


def check(ok, name, why=""):
    global failures
    print(("PASS " if ok else "FAIL ") + name + ("" if ok else ": " + why))
    if not ok:
        failures += 1


def run(build, work, parent, frags, extra, home):
    env = dict(os.environ, ECCE_HOME=REPO, ECCE_REALUSER="fragments",
               ECCE_REALUSERHOME=home)
    env.pop("ECCE_LOCAL_DATA", None)
    cmd = [os.path.join(build, "fragments"), os.path.join(work, parent)]
    for path, label in frags:
        cmd += ["--fragment", os.path.join(work, path), "--label", label]
    return subprocess.run(cmd + extra, capture_output=True, text=True,
                          env=env, timeout=300)


def analyse(build, work, home, method):
    p = run(build, work, "parent", [("cr", "Cr"), ("co6", "(CO)6")],
            ["--json", "--method", method, "--threshold", "0.05",
             "--mo", "30-90", "--tol", str(CHECK_TOLERANCE)], home)
    if p.returncode != 0:
        print(p.stderr)
        return None
    return json.loads(p.stdout)


def main():
    build = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, "build-cmake")
    oracle = json.load(open(os.path.join(HERE, "oracle.json")))
    work = tempfile.mkdtemp(prefix="fragments-")
    try:
        for n in ("parent", "cr", "co6"):
            shutil.copytree(os.path.join(HERE, "fixtures", n),
                            os.path.join(work, n))
        home = os.path.join(work, "home")
        os.makedirs(home)

        results = {}
        for method in ("mulliken", "scpa"):
            res = analyse(build, work, home, method)
            check(res is not None, method + ": analysis ran and its checks passed")
            if res is None:
                continue
            results[method] = res
            worst = (0.0, None)
            problems = []
            for orb in res["orbitals"]:
                ref = oracle[method][str(orb["mo"])]
                got = {"%d:%d" % (o["f"], o["i"]): o["pct"] for o in orb["fo"]}
                if abs(orb["occ"] - ref["occ"]) > 1e-3:
                    problems.append("MO %d occupation" % orb["mo"])
                for key in set(got) | set(ref["fo"]):
                    d = abs(got.get(key, 0.0) - ref["fo"].get(key, 0.0))
                    if d > worst[0]:
                        worst = (d, "MO %d fragment orbital %s" % (orb["mo"], key))
                    if d > TOLERANCE:
                        problems.append("MO %d %s: ecce %.2f, Multiwfn %.2f" % (
                            orb["mo"], key, got.get(key, 0.0),
                            ref["fo"].get(key, 0.0)))
            check(not problems, "%s compositions, 61 MOs, vs Multiwfn "
                  "(worst %.3f points, %s)" % (method, worst[0], worst[1]),
                  "; ".join(problems[:5]))

        res = results.get("mulliken")
        if res:
            mo = {o["mo"]: o for o in res["orbitals"]}
            check(res["homo"] == 54, "HOMO is MO 54")
            check(res["maxCaptureError"] < CHECK_TOLERANCE,
                  "summed composition of every MO is 1 (%.1e)" % res["maxCaptureError"])
            check(res["maxNormError"] < CHECK_TOLERANCE,
                  "parent MOs normalised (%.1e)" % res["maxNormError"])
            check(all(abs(sum(o["fragments"]) - 100.0 * o["capture"]) < 1e-3
                      for o in res["orbitals"]), "per-fragment totals add up")
            # t2g HOMOs: mostly Cr d, with a (CO)6 2pi* share (fragment
            # orbitals 43-45, the unoccupied t2g set at +0.069 Eh).
            for n in (52, 53, 54):
                o = mo[n]
                pi = sum(f["pct"] for f in o["fo"]
                         if f["f"] == 2 and 43 <= f["i"] <= 45)
                check(o["fragments"][0] > 60 and pi > 15 and o["occ"] == 2.0,
                      "MO %d (t2g): Cr %.1f %%, CO 2pi* %.1f %%" % (
                          n, o["fragments"][0], pi))
            # eg*: Cr d with the (CO)6 5sigma eg pair (occupied, orbitals
            # 41-42) above the 10 % threshold.
            for n in (68, 69):
                o = mo[n]
                sig = sum(f["pct"] for f in o["fo"]
                          if f["f"] == 2 and f["i"] in (41, 42))
                check(o["fragments"][0] > 40 and sig >= 10 and o["occ"] == 0.0,
                      "MO %d (eg*): Cr %.1f %%, CO 5sigma %.1f %%" % (
                          n, o["fragments"][0], sig))
            delta = mo[68]["energy"] - mo[54]["energy"]
            check(delta > 0.5, "eg* lies above t2g by %.3f Eh" % delta)
            # the CO 2pi* t2g set lies above the occupied metal d and above
            # the complex's own t2g
            e_pi = [f["e"] for f in mo[54]["fo"] if f["f"] == 2 and 43 <= f["i"] <= 45][0]
            e_d = [f["e"] for f in mo[54]["fo"] if f["f"] == 1][0]
            check(e_pi > e_d and e_pi > mo[54]["energy"],
                  "CO 2pi* (%.3f Eh) above Cr d (%.3f) and t2g (%.3f)" % (
                      e_pi, e_d, mo[54]["energy"]))

        # a wrong fragment set is refused with a reason
        p = run(build, work, "parent", [("cr", "Cr"), ("cr", "Cr again")], [], home)
        check(p.returncode != 0 and "shares parent atoms" in p.stderr,
              "the same atom in two fragments is refused", p.stderr.strip()[-200:])
        p = run(build, work, "parent", [("cr", "Cr")], [], home)
        check(p.returncode != 0 and "do not reproduce" in p.stderr,
              "fragments not covering the parent are refused", p.stderr.strip()[-200:])
        p = run(build, work, "co6", [("cr", "Cr")], [], home)
        check(p.returncode != 0 and "no unused" in p.stderr,
              "a fragment atom absent from the parent is refused", p.stderr.strip()[-200:])
    finally:
        shutil.rmtree(work, ignore_errors=True)
    print("%d failure(s)" % failures)
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
