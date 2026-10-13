#!/usr/bin/env python3
"""qm_raman: ecce-qm's static polarisability (finite field) and Raman
activities and depolarisation ratios against ORCA 6.1.1 (analytic CP-SCF
polarisabilities; Freq at HF, NumFreq at B3LYP) and, for the polarisability,
NWChem 7.2.3 (polar_oracle.json, raman_oracle.json, made by
gen_raman_oracle.py).

    raman_test.py ECCE_QM_EXE [--quick] [--table] [--filter REGEX]

ecce-qm runs at the geometry stored with each oracle entry, so both programs
differentiate the same point.  Degenerate modes are compared as a group: the
split of a degenerate set into individual modes is arbitrary, only the sums of
45 a'^2 and 7 gamma'^2 over the set are defined.  Tolerances are in the
constants below.
"""
import json, math, os, re, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
TOL_ALPHA = 2e-3        # bohr^3 per component; measured at most 9e-4 (CO B3LYP vs ORCA), 1e-4 vs NWChem
TOL_RAMAN_REL, TOL_RAMAN_ABS = 3e-3, 0.02   # activity of a mode group; measured at most 9e-4, A^4/amu
TOL_DEPOL = 2e-3                            # measured at most 2e-4
TOL_FREQ = {"hf": 1.0, "b3lyp": 3.0}
QUICK_POLAR = ("polar_h2o_hf", "polar_co_b3lyp")
QUICK_RAMAN = ("raman_h2o_hf",)


def write_input(path, ent, runtype, extra):
    with open(path, "w") as f:
        f.write("charge 0\nmultiplicity 1\nmethod %s\nbasis %s\nruntype %s\n%s" % (
            ent["method"], ent["basis"], runtype, extra))
        f.write("conv_energy 1e-12\nconv_grad 1e-9\nmaxiter 300\ngeometry\n")
        for a in ent["atoms"]:
            f.write("%s %.10f %.10f %.10f\n" % tuple(a))
        f.write("end\n")


def run(exe, path):
    p = subprocess.run([exe, path], capture_output=True, text=True)
    if p.returncode != 0:
        raise RuntimeError("ecce-qm failed: " + p.stderr[-300:])
    return p.stdout


def tensor(out):
    block = re.search(r"begin polarizability.*?\n(.*?)end polarizability", out, re.S).group(1)
    return [[float(x) for x in row.split()] for row in block.strip().splitlines()]


def check_polar(exe, name, ent, work):
    p = os.path.join(work, "p.qm")
    write_input(p, ent, "energy", "polarizability true\n")
    got = tensor(run(exe, p))
    res = []
    for prog, ref in (("ORCA", ent["alpha"]), ("NWChem", ent.get("nwchem", {}).get("alpha"))):
        if ref is None:
            continue
        d = max(abs(got[i][j] - ref[i][j]) for i in range(3) for j in range(3))
        res.append((prog, d, d <= TOL_ALPHA))
    return res


def groups(freq, act, dep):
    """Modes closer than 2 cm-1 form a group; per group the sums of 45 a'^2
    and 7 gamma'^2 recovered from each mode's activity and depolarisation."""
    out = []
    for f, s, r in zip(freq, act, dep):
        a45 = s * (3 - 4 * r) / (3 + 3 * r)
        g7 = s - a45
        if out and abs(f - out[-1][0]) < 2.0:
            out[-1][1] += a45
            out[-1][2] += g7
            out[-1][3] += 1
        else:
            out.append([f, a45, g7, 1])
    return out


def check_raman(exe, name, ent, work):
    p = os.path.join(work, "r.qm")
    write_input(p, ent, "frequency", "raman true\n")
    out = run(exe, p)
    ntr = int(re.search(r"^vib_transrot\s+(\d+)", out, re.M).group(1))
    rows = re.search(r"begin vibrations.*?\n(.*?)end vibrations", out, re.S).group(1).splitlines()
    freq = [float(r.split()[1]) for r in rows][ntr:]
    rrows = re.search(r"begin raman.*?\n(.*?)end raman", out, re.S).group(1).splitlines()
    act = [float(r.split()[1]) for r in rrows][ntr:]
    dep = [float(r.split()[2]) for r in rrows][ntr:]
    if len(rrows) != len(rows) or any(float(r.split()[1]) != 0 for r in rrows[:ntr]):
        return False, "raman block is not row-aligned with vibrations", 0, 0, 0
    if len(freq) != len(ent["freq"]):
        return False, "%d modes, ORCA %d" % (len(freq), len(ent["freq"])), 0, 0, 0
    df = max(abs(a - b) for a, b in zip(freq, ent["freq"]))
    ga = groups(freq, act, dep)
    gb = groups(ent["freq"], ent["activity"], ent["depol"])
    if [g[3] for g in ga] != [g[3] for g in gb]:
        return False, "degeneracy pattern differs", df, 0, 0
    ds = dd = 0.0
    ok = df <= TOL_FREQ[ent["method"]]
    for x, y in zip(ga, gb):
        sx, sy = x[1] + x[2], y[1] + y[2]
        rx = 3 * x[2] / 7 / (x[1] + 4 * x[2] / 7) if sx > 0 else 0
        ry = 3 * y[2] / 7 / (y[1] + 4 * y[2] / 7) if sy > 0 else 0
        ds = max(ds, abs(sx - sy) / max(abs(sy), 1e-9))
        dd = max(dd, abs(rx - ry))
        ok = ok and (abs(sx - sy) <= TOL_RAMAN_ABS or abs(sx - sy) <= TOL_RAMAN_REL * abs(sy)) and abs(rx - ry) <= TOL_DEPOL
    return ok, "", df, ds, dd


def main():
    args = sys.argv[1:]
    exe = args.pop(0)
    quick = "--quick" in args
    table = "--table" in args
    pat = re.compile(args[args.index("--filter") + 1]) if "--filter" in args else None
    bad = 0
    with tempfile.TemporaryDirectory() as work:
        for fname, kind in (("polar_oracle.json", "polar"), ("raman_oracle.json", "raman")):
            oracle = json.load(open(os.path.join(HERE, fname)))
            for name, ent in sorted(oracle.items()):
                if quick and name not in (QUICK_POLAR if kind == "polar" else QUICK_RAMAN):
                    continue
                if pat and not pat.search(name):
                    continue
                if kind == "polar":
                    for prog, d, ok in check_polar(exe, name, ent, work):
                        bad += not ok
                        if table:
                            print("| %s | %s | %.1e |" % (name, prog, d))
                        elif not ok:
                            print("FAIL %s vs %s: max d(alpha) %.2e" % (name, prog, d))
                else:
                    ok, why, df, ds, dd = check_raman(exe, name, ent, work)
                    bad += not ok
                    if table:
                        print("| %s | %.2f | %.1e | %.1e |" % (name, df, ds, dd))
                    elif not ok:
                        print("FAIL %s: %s dfreq %.2f dS(rel) %.2e ddepol %.2e" % (name, why, df, ds, dd))
    print("qm_raman: %s" % ("FAILED (%d)" % bad if bad else "ok"))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
