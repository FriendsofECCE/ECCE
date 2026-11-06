#!/usr/bin/env python3
"""ecce-fragorb (#201): CLI behaviour, Cr(CO)6 against Multiwfn, library tests.

    run_tests.py <ecce-fragorb> <fragorb_h5_test>

Fixtures: ORCA 6.1.1 RHF/def2-SVP of Cr(CO)6 (the geometry of
tests/modiagram/fixtures/orca-crco6), of Cr alone (singlet, closed shell) and
of the neutral (CO)6 at the complex geometry (inputs/), converted with
`ecce-results convert` from orca_2mkl -molden.  cr7 is Cr as ROHF septet, the
physical fragment.  reference/*.tsv are Multiwfn 2026.9.20 CDA numbers for the
same moldens (multiwfn_reference.py); the singlet Cr and (CO)6 are its
fragments.  Tolerance 0.1 percentage points; Multiwfn prints 0.01 and the largest
difference (0.05, a 0.05 % share of a high Cr virtual) comes from its own integrals.
"""
import json
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
FIX = os.path.join(HERE, "fixtures")
fails = 0


def check(cond, msg):
    global fails
    if not cond:
        fails += 1
        print("FAIL", msg)


def run(args, **kw):
    return subprocess.run(args, capture_output=True, text=True, **kw)


def main():
    exe, libtest = sys.argv[1], sys.argv[2]
    cx, cr, cr7, co6 = [os.path.join(FIX, n) for n in ("crco6", "cr", "cr7", "co6")]

    r = run([exe, "--help"])
    check(r.returncode == 0 and "ecce-fragorb analyse" in r.stdout and "ecce-fragorb dos" in r.stdout, "--help")

    # Multiwfn comparison, folder arguments and an .h5 path
    for method, ref in (("mulliken", "crco6_cr_co6_multiwfn_mulliken.tsv"), ("scpa", "crco6_cr_co6_multiwfn_scpa.tsv")):
        r = run([exe, "analyse", cx, os.path.join(cr, "results.h5"), co6, "--method", method, "--json"])
        check(r.returncode == 0, "analyse %s: %s" % (method, r.stderr))
        if r.returncode:
            continue
        j = json.loads(r.stdout)
        ch = j["channels"][0]
        off = [0]
        for f in range(2):
            off.append(off[-1] + sum(1 for fo in ch["fo"] if fo["fragment"] == f))
        ref_rows = {}
        for line in open(os.path.join(HERE, "reference", ref)):
            if line.startswith("#"):
                continue
            mo, fr, fo, pct = line.split()
            ref_rows[(int(mo), int(fr), int(fo))] = float(pct)
        worst, n = 0.0, 0
        for (mo, fr, fo), pct in ref_rows.items():
            mine = 100 * ch["mo"][mo - 1]["composition"][off[fr - 1] + fo - 1]
            worst = max(worst, abs(mine - pct))
            n += 1
        # nothing sizeable that Multiwfn does not list
        mos = sorted({k[0] for k in ref_rows})
        for mo in mos:
            for k, v in enumerate(ch["mo"][mo - 1]["composition"]):
                fr = 1 if k < off[1] else 2
                fo = k + 1 - off[fr - 1]
                if abs(v) * 100 > 0.05 and (mo, fr, fo) not in ref_rows:
                    check(False, "%s MO %d FO %d/%d: %.3f%% not in Multiwfn's list" % (method, mo, fr, fo, 100 * v))
        print("%s: %d compositions of %d MOs against Multiwfn, max difference %.4f percentage points" % (method, n, len(mos), worst))
        check(worst < 0.1, "%s differs from Multiwfn by %.4f" % (method, worst))
        check(max(abs(m["remainder"]) for m in ch["mo"]) < 1e-6, "remainder")

    # Cr(CO)6 with the physical Cr fragment (ROHF septet): d-set and CO pi* shares
    r = run([exe, "analyse", cx, cr7, co6, "--json"])
    j = json.loads(r.stdout)
    ch = j["channels"][0]
    fo = ch["fo"]
    def share(mo, frag, lo, hi):
        return 100 * sum(ch["mo"][mo - 1]["composition"][k] for k in range(len(fo))
                         if fo[k]["fragment"] == frag and lo <= fo[k]["index"] <= hi)
    for mo in (52, 53, 54):   # t2g: Cr 3d and the CO 2pi* t2g set
        check(abs(share(mo, 0, 11, 15) - 71.25) < 0.1 and abs(share(mo, 1, 43, 45) - 23.54) < 0.1, "t2g MO %d" % mo)
    for mo in (68, 69):       # eg*: Cr 3d, CO sigma* (59-60) and 5sigma (41-42)
        check(abs(share(mo, 0, 11, 15) - 55.72) < 0.1 and abs(share(mo, 1, 59, 60) - 28.92) < 0.1, "eg* MO %d" % mo)
    check(any(c["mo"] == 52 and c["fo"] <= 15 for c in j["connections"]), "connection to the t2g MO")

    # text output and refusals
    r = run([exe, "analyse", cx, cr7, co6, "--levels"])
    check(r.returncode == 0 and "FO 11-15" in r.stdout and "connections" in r.stdout, "text output")
    r = run([exe, "analyse", cx, cr7])
    check(r.returncode == 1 and "do not cover" in r.stderr, "uncovered: " + r.stderr)
    r = run([exe, "analyse", cx, cr7, "--allow-uncovered", "--json"])
    check(r.returncode == 0, "--allow-uncovered")
    r = run([exe, "analyse", cx, cr, cr7, co6])
    check(r.returncode == 1 and "overlap" in r.stderr, "overlapping fragments: " + r.stderr)
    r = run([exe, "analyse", cx, os.path.join(FIX, "nowhere"), co6])
    check(r.returncode == 1 and "no such file" in r.stderr, "missing input: " + r.stderr)
    r = run([exe, "analyse", cx, co6, co6])
    check(r.returncode == 1, "same fragment twice")

    # dos
    r = run([exe, "dos", cx, cr7, co6, "--emin", "-30", "--emax", "10", "--json"])
    check(r.returncode == 0, "dos: " + r.stderr)
    if r.returncode == 0:
        d = json.loads(r.stdout)
        check(d["pairs"] == ["cr7-co6"] and d["occupied_overlap"][0] > 1, "dos pair / overlap population")
        check(max(abs(g["pdos"][0] + g["pdos"][1] - g["tdos"]) for g in d["grid"]) < 1e-5, "PDOS sums to TDOS")
    r = run([exe, "dos", cx, "--set", "Cr=1", "--set", "CO=2-13", "--shape", "lorentzian"])
    check(r.returncode == 0 and "OPDOS(Cr-CO)" in r.stdout, "dos --set: " + r.stderr)

    r = run([libtest, os.path.join(cx, "results.h5")])
    sys.stdout.write(r.stdout)
    sys.stderr.write(r.stderr)
    check(r.returncode == 0 and "RESULT OK" in r.stdout, "library test")

    print("RESULT FAIL" if fails else "RESULT OK")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
