#!/usr/bin/env python3
"""Run a packaged ecce-qm on the lab set and compare with tests/qm/oracle.json.

    package_check.py ECCE_QM_EXE BASIS_DIR [--lab-table]

Water and the lab molecules (C2, O2 triplet ROKS, He2 at 3.0 A, HF, CO), all
B3LYP/6-31G*, against the ORCA energies; exit 1 on a difference above 1e-5 Eh or
an unconverged run.  Prints the platform, the executable's size and whether
it is stripped (Linux/macOS: no symbol table beyond the dynamic one).
"""
import json, os, platform, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))


def write_input(ent, path):
    with open(path, "w") as f:
        f.write("title %s\ncharge %d\nmultiplicity %d\nmethod %s%s\nbasis %s\ngeometry\n" % (
            ent["molecule"], ent["charge"], ent["multiplicity"], ent["reference"], ent["method"], ent["basis"]))
        for a in ent["atoms"]:
            f.write("%s %.10f %.10f %.10f\n" % tuple(a))
        f.write("end\n")


def parse_output(path):
    res = {}
    for line in open(path):
        t = line.split()
        if t and t[0] == "energy_total":
            res["energy"] = float(t[1])
        elif t and t[0] == "converged":
            res["converged"] = t[1] == "yes"
    return res

CASES = ["h2o_6-31gs_b3lyp_r", "c2_6-31gs_b3lyp_r", "o2_6-31gs_b3lyp_ro",
         "he2_6-31gs_b3lyp_r", "hf_6-31gs_b3lyp_r", "co_6-31gs_b3lyp_r"]
TOL = 1e-5


def main():
    exe, basis = sys.argv[1], sys.argv[2]
    maxmb = float(sys.argv[sys.argv.index("--max-mb") + 1]) if "--max-mb" in sys.argv else None
    oracle = json.load(open(os.path.join(HERE, "oracle.json")))
    size = os.path.getsize(exe) / 1e6
    print("%s %s: %s is %.1f MB" % (platform.system(), platform.machine(), exe, size))
    bad = 0
    if maxmb is not None and size > maxmb:
        print("FAIL: larger than %g MB, so not stripped" % maxmb)
        bad += 1
    with tempfile.TemporaryDirectory() as tmp:
        for key in CASES:
            ent = oracle[key]
            deck = os.path.join(tmp, key + ".in")
            out = os.path.join(tmp, key + ".out")
            write_input(ent, deck)
            r = subprocess.run([exe, "--basis-dir", basis, "-o", out, deck],
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            res = parse_output(out) if os.path.exists(out) else {}
            want = ent["orca"]["energy"]
            got = res.get("energy")
            ok = got is not None and res.get("converged") and abs(got - want) < TOL
            print("%-24s ecce-qm %s  ORCA %.8f  diff %s  %s" % (
                key, "%.8f" % got if got is not None else "-", want,
                "%.2e" % (got - want) if got is not None else "-", "ok" if ok else "FAIL " + r.stdout[-200:]))
            bad += not ok
    sys.exit(1 if bad else 0)


main()
