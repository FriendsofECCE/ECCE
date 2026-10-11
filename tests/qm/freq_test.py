#!/usr/bin/env python3
"""qm_freq: ecce-qm harmonic frequencies, IR intensities and thermochemistry
against ORCA 6.1.1 (freq_oracle.json), and restricted open-shell O2 against
the second derivative of its own energies.

    freq_test.py ECCE_QM_EXE [--quick] [--table] [--filter REGEX]

ecce-qm runs `runtype frequency` at the geometry ORCA ended its TightOpt on,
so both programs differentiate the same point.  Tolerances: frequencies 1 cm-1
(HF) and 3 cm-1 (B3LYP; the DFT grids and the neglected grid-weight terms
differ), IR intensities 2 % or 0.5 km/mol, ZPE 1e-5 Eh, thermal energy,
enthalpy, T*S and G 5e-5 Eh.  ORCA runs with QuasiRRHO off (plain RRHO).
"""
import json, math, os, re, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
TOL_F = {"hf": 1.0, "b3lyp": 3.0}
TOL_IR_REL, TOL_IR_ABS, TOL_ZPE, TOL_G = 0.02, 0.5, 1e-5, 5e-5
QUICK = ("h2o_hf_r", "co_hf_r", "o2_hf_u", "nh3_b3lyp_r")

def write_input(path, ent, method, mult, atoms, runtype, sigma=None):
    with open(path, "w") as f:
        if sigma:
            f.write("symmetry_number %d\n" % sigma)
        f.write("charge 0\nmultiplicity %d\nmethod %s\nbasis %s\nruntype %s\n" % (mult, method, ent["basis"], runtype))
        f.write("conv_energy 1e-12\nconv_grad 1e-9\nmaxiter 300\ngeometry\n")
        for a in atoms:
            f.write("%s %.10f %.10f %.10f\n" % tuple(a))
        f.write("end\n")

def run(exe, path):
    p = subprocess.run([exe, path], capture_output=True, text=True)
    if p.returncode != 0:
        raise RuntimeError("ecce-qm failed: " + p.stderr[-300:])
    return p.stdout

def key(out, name):
    return float(re.search(r"^%s\s+(\S+)" % name, out, re.M).group(1))

def parse(out):
    ntr = int(re.search(r"^vib_transrot\s+(\d+)", out, re.M).group(1))
    rows = re.search(r"begin vibrations.*?\n(.*?)end vibrations", out, re.S).group(1).splitlines()
    f = [float(r.split()[1]) for r in rows][ntr:]
    ir = [float(r.split()[2]) for r in rows][ntr:]
    resid = [float(x) for x in re.search(r"begin transrot_residual.*?\n(.*?)end transrot", out, re.S).group(1).split()]
    return dict(freq=f, ir=ir, resid=resid, zpe=key(out, "zero_point_energy"), e_thermal=key(out, "thermal_energy"),
                enthalpy=key(out, "enthalpy"), ts=key(out, "entropy_ts"), gibbs=key(out, "gibbs_energy"),
                sigma=int(key(out, "symmetry_number")))

def compare(ent, got):
    tol = TOL_F[ent["method"]]
    df = max(abs(a - b) for a, b in zip(got["freq"], ent["freq"]))
    dir_ = max(min(abs(a - b), abs(a - b) / max(abs(b), 1e-9) * 100) for a, b in zip(got["ir"], ent["ir"]))
    dir_ok = all(abs(a - b) <= TOL_IR_ABS or abs(a - b) <= TOL_IR_REL * abs(b) for a, b in zip(got["ir"], ent["ir"]))
    dz = got["zpe"] - ent["zpe"]
    dth = max(abs(got[k] - ent[kk]) for k, kk in (("e_thermal", "e_thermal"), ("enthalpy", "enthalpy"), ("ts", "ts"), ("gibbs", "gibbs")))
    dg = got["gibbs"] - ent["gibbs"]
    ok = (len(got["freq"]) == len(ent["freq"]) and df <= tol and dir_ok and abs(dz) <= TOL_ZPE and dth <= TOL_G)
    return ok, df, dir_, dz, dg, dth

def rohf_o2(exe, work):
    """Harmonic frequency of restricted open-shell O2 from energies at five
    bond lengths (quartic fit), against the analytic-gradient Hessian."""
    import numpy as np
    ent = json.load(open(os.path.join(HERE, "freq_oracle.json")))["o2_hf_u"]
    r0 = math.dist(ent["atoms"][0][1:], ent["atoms"][1][1:])
    def energy(r):
        p = os.path.join(work, "e.qm")
        write_input(p, ent, "rohf", 3, [("O", 0, 0, 0), ("O", 0, 0, r)], "energy")
        return key(run(exe, p), "energy_total")
    # find the minimum first, then the curvature there
    r = 1.17
    for _ in range(4):
        xs = r + 0.01 * np.arange(-3, 4)
        c = np.polyfit(xs - r, [energy(x) for x in xs], 4)
        step = -c[3] / (2 * c[2])
        r += step
    xs = r + 0.01 * np.arange(-3, 4)
    c = np.polyfit(xs - r, [energy(x) for x in xs], 4)
    k_au = 2 * c[2] * 0.529177210903 ** 2                         # hartree/bohr^2
    mu = 15.99491461956 / 2
    omega = math.sqrt(k_au * 4.3597447222071e-18 / (0.529177210903e-10 ** 2) / (mu * 1.66053906660e-27)) / (2 * math.pi * 299792458.0 * 100)
    p = os.path.join(work, "f.qm")
    write_input(p, ent, "rohf", 3, [("O", 0, 0, 0), ("O", 0, 0, r)], "frequency")
    got = parse(run(exe, p))
    return omega, got["freq"][0], got["ir"][0], got["resid"]

def main():
    args = sys.argv[1:]
    exe = args.pop(0)
    quick = "--quick" in args
    table = "--table" in args
    pat = re.compile(args[args.index("--filter") + 1]) if "--filter" in args else None
    oracle = json.load(open(os.path.join(HERE, "freq_oracle.json")))
    bad = 0
    if table:
        print("| case | max d(freq) cm-1 | max d(IR) (km/mol, or %) | d(ZPE) Eh | d(G) Eh | max d(E,H,TS,G) Eh | smallest TR before projection (cm-1) |")
        print("|---|---|---|---|---|---|---|")
    with tempfile.TemporaryDirectory() as work:
        for name, ent in sorted(oracle.items()):
            if (quick and name not in QUICK) or (pat and not pat.search(name)):
                continue
            mult = 3 if ent["molecule"] == "o2" else 1
            prefix = "u" if ent["reference"] == "u" else ""
            p = os.path.join(work, "f.qm")
            # ORCA finds a lower point group for some DFT geometries (CH4 comes
            # out C3v, sigma 4); those get its number so the thermochemistry
            # is comparable, the HF ones must find it themselves.
            fix = None if ent["method"] == "hf" else ent["symmetry_number"]
            write_input(p, ent, prefix + ent["method"], mult, ent["atoms"], "frequency", fix)
            got = parse(run(exe, p))
            ok, df, dir_, dz, dg, dth = compare(ent, got)
            if got["sigma"] != ent["symmetry_number"]:
                print("FAIL %s: symmetry number %d, ORCA %d" % (name, got["sigma"], ent["symmetry_number"]))
                ok = False
            bad += not ok
            if table:
                print("| %s | %.2f | %.2f | %.1e | %.1e | %.1e | %s |" % (
                    name, df, dir_, dz, dg, dth, " ".join("%.2f" % abs(x) for x in got["resid"])))
            elif not ok:
                print("FAIL %s: dfreq %.2f dir %.2f dzpe %.2e dG %.2e dthermo %.2e" % (name, df, dir_, dz, dg, dth))
        if not pat or pat.search("o2_rohf"):
            omega, f, ir, resid = rohf_o2(exe, work)
            ok = abs(omega - f) <= 0.5 and abs(ir) < 0.1
            bad += not ok
            if table:
                print("| o2_rohf (energy second derivative: %.2f cm-1) | %.2f | IR %.3f | | | | %s |" % (
                    omega, f - omega, ir, " ".join("%.2f" % abs(x) for x in resid)))
            elif not ok:
                print("FAIL o2_rohf: analytic %.2f numeric %.2f" % (f, omega))
    print("qm_freq: %s" % ("FAILED (%d)" % bad if bad else "ok"))
    return 1 if bad else 0

if __name__ == "__main__":
    sys.exit(main())
