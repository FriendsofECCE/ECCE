#!/usr/bin/env python3
"""qm_ts: ecce-qm transition-state search (runtype ts) on HCN <-> HNC against
ORCA 6.1.1 OptTS (ts_oracle.json, #452).

    ts_test.py ECCE_QM_EXE [--quick] [--table]

Compared with ORCA: the TS energy (1e-5 Eh), C-N, C-H and N-H distances
(0.002 angstrom), the H-C-N angle (0.2 degrees) and the three frequencies
(2 cm-1).  Independent of the oracle: exactly one imaginary frequency, the
progress lines of the optimisation, the converged exit status.  Variants
(full run only): finite-difference Hessian, Hessian recomputed every 3 steps.
"""
import json, os, re, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from ts_cases import CASES, GUESS, internals

TOL_E, TOL_R, TOL_A, TOL_F = 1e-5, 2e-3, 0.2, 2.0
VARIANTS = {"": "", "numeric": "numeric_hessian true\n", "every3": "ts_hessian_every 3\n"}


def qm_input(case, extra):
    return ("runtype ts\nmethod hf\nbasis %s\nopt_conv tight\nmaxsteps 60\nconv_energy 1e-10\n%s" % (case["basis"], extra)
            + "geometry\n" + "".join("%s %.8f %.8f %.8f\n" % a for a in GUESS) + "end\n")


def block(out, name):
    m = re.findall(r"^begin %s\b[^\n]*\n(.*?)^end %s" % (name, name), out, re.S | re.M)
    return m[-1].splitlines() if m else []


def check(exe, name, variant, table):
    case, oracle = CASES[name], json.load(open(os.path.join(HERE, "ts_oracle.json")))[name]
    with tempfile.TemporaryDirectory() as work:
        inp = os.path.join(work, "ts.qm")
        open(inp, "w").write(qm_input(case, VARIANTS[variant]))
        p = subprocess.run([exe, inp], capture_output=True, text=True)
    out, fails = p.stdout, []
    if p.returncode != 0:
        return ["exit status %d: %s" % (p.returncode, p.stderr[-300:])]
    e = float(re.search(r"^energy_total\s+(\S+)", out, re.M).group(1))
    xyz = [[float(c) for c in l.split()[2:5]] for l in block(out, "geometry")]
    got = internals(xyz)
    n = int(re.search(r"^vib_imaginary\s+(\d+)", out, re.M).group(1))
    nrot = int(re.search(r"^vib_transrot\s+(\d+)", out, re.M).group(1))
    freq = [float(l.split()[1]) for l in block(out, "vibrations")][nrot:]
    if n != 1:
        fails.append("%d imaginary frequencies" % n)
    if not re.search(r"^progress opt step=\d+ converged=yes", out, re.M):
        fails.append("no converged progress line")
    if not re.search(r"^progress opt step=1 energy=", out, re.M):
        fails.append("no progress line for the first step")
    if abs(e - oracle["energy"]) > TOL_E:
        fails.append("energy %.8f against ORCA %.8f" % (e, oracle["energy"]))
    for lab, g, o, tol in zip(["C-N", "C-H", "N-H", "H-C-N"], got, oracle["internals"], [TOL_R] * 3 + [TOL_A]):
        if abs(g - o) > tol:
            fails.append("%s %.4f against ORCA %.4f" % (lab, g, o))
    if len(freq) != 3 or any(abs(a - b) > TOL_F for a, b in zip(freq, oracle["freq"])):
        fails.append("frequencies %s against ORCA %s" % (freq, oracle["freq"]))
    if table:
        print("  %-9s ecce-qm %14.8f %s %s" % (variant or "default", e, [round(x, 4) for x in got], freq))
        print("  %-9s ORCA    %14.8f %s %s" % ("", oracle["energy"], [round(x, 4) for x in oracle["internals"]], oracle["freq"]))
    return fails


def main():
    args = sys.argv[1:]
    exe, quick, table = args[0], "--quick" in args, "--table" in args
    bad = 0
    for name, case in CASES.items():
        if quick and not case["quick"]:
            continue
        for variant in VARIANTS if not quick else [""]:
            fails = check(exe, name, variant, table)
            print("%-22s %s" % (name + (" " + variant if variant else ""), "ok" if not fails else "FAIL"))
            for f in fails:
                print("   ", f)
            bad += bool(fails)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
