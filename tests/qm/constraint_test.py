#!/usr/bin/env python3
"""qm_constraints: constrained optimisations with a dummy atom (#309).

    constraint_test.py ECCE_QM_EXE [--quick]

NH3 with a dummy atom X on the C3 axis and the three X-N-H angles held at
100, 110 and 120 degrees (HF and B3LYP/6-31G*) against NWChem with zcoord
constraints, the way ai.nwchem writes them; a bond and an angle constraint on
water against ORCA's %geom Constraints.  Compared: final energy (1e-5 Eh) and
every N-H (O-H) length (1e-3 angstrom).  Exits 77 when the other codes are
not installed.
"""
import glob, math, os, re, shutil, subprocess, sys, tempfile

TOL_E, TOL_R = 1e-5, 1e-3
NWCHEM = shutil.which("nwchem")
MPIRUN = shutil.which("mpirun")
ORCA = (glob.glob("/opt/orca/orca_6*/orca") or [None])[0]


def nh3x(angle):
    """N at the origin, X 1 A up the axis, H at the constrained angle to X."""
    t = math.radians(angle)
    atoms = [("N", 0.0, 0.0, 0.0)]
    for k in range(3):
        p = math.radians(120 * k)
        atoms.append(("H", math.sin(t) * math.cos(p), math.sin(t) * math.sin(p), math.cos(t)))
    atoms.append(("X", 0.0, 0.0, 1.0))
    return atoms


def water(angle=104.0):
    t = math.radians(angle / 2)
    r = 0.96
    return [("O", 0.0, 0.0, 0.0), ("H", r * math.sin(t), 0.0, r * math.cos(t)),
            ("H", -r * math.sin(t), 0.0, r * math.cos(t))]


def geometry_lines(atoms):
    return "".join("%s %.10f %.10f %.10f\n" % a for a in atoms)


def run_qm(exe, work, atoms, method, constraints):
    inp = os.path.join(work, "qm.in")
    with open(inp, "w") as f:
        f.write("runtype optimize\nmethod %s\nbasis 6-31G*\nopt_conv tight\nmaxsteps 200\n"
                "conv_energy 1e-10\ngrid 99 590\n" % method)
        f.write("geometry\n" + geometry_lines(atoms) + "end\nconstraints\n" + constraints + "end\n")
    p = subprocess.run([exe, inp], capture_output=True, text=True)
    if p.returncode != 0:
        raise RuntimeError("ecce-qm failed: " + (p.stdout[-300:] + p.stderr[-300:]))
    out = p.stdout
    block = re.search(r"begin atoms.*?\n(.*?)end atoms", out, re.S).group(1)
    geo = [[l.split()[1]] + [float(v) for v in l.split()[3:6]] for l in block.splitlines()]
    return float(re.search(r"^energy_total\s+(\S+)", out, re.M).group(1)), geo


def run_nwchem(work, atoms, method, zcoord):
    inp = os.path.join(work, "nw.nw")
    dft = ""
    task = "scf"
    if method == "b3lyp":
        # libxc's B3LYP5 (VWN5), not NWChem's VWN1RPA default
        dft = ("dft\n xc hfexch 0.20 slater 0.80 becke88 nonlocal 0.72 vwn_5 0.19 lyp 0.81\n"
               " grid xfine\n convergence energy 1e-10\nend\n")
        task = "dft"
    with open(inp, "w") as f:
        f.write("start nw\necho\ngeometry noautosym units angstrom\n" + geometry_lines(atoms) +
                "zcoord\n" + zcoord + "end\nend\n"
                'basis "ao basis" spherical\n * library 6-31G*\nend\n'
                "scf\n thresh 1e-10\nend\n" + dft +
                "driver\n tight\n maxiter 200\nend\ntask %s optimize\n" % task)
    p = subprocess.run([MPIRUN, "--oversubscribe", "-np", "2", NWCHEM, inp], cwd=work, capture_output=True, text=True)
    out = p.stdout
    if "Optimization converged" not in out and "converged" not in out.lower():
        raise RuntimeError("nwchem did not converge: " + out[-400:])
    e = float(re.findall(r"Total (?:SCF|DFT) energy\s+=\s+(-?\d+\.\d+)", out)[-1])
    block = out.split("Output coordinates in angstrom")[-1].split("\n")[4:]
    geo = []
    for l in block:
        m = re.match(r"\s*\d+\s+(\S+)\s+[\d.]+\s+(-?\d+\.\d+)\s+(-?\d+\.\d+)\s+(-?\d+\.\d+)", l)
        if not m:
            break
        sym = m.group(1)
        geo.append(["X" if sym.lower().startswith("x") else sym[0].upper() + sym[1:].lower()] + [float(v) for v in m.groups()[1:]])
    return e, geo


def run_orca(work, atoms, method, constraint):
    inp = os.path.join(work, "o.inp")
    kw = "HF" if method == "hf" else "B3LYP NoRI DefGrid3"
    with open(inp, "w") as f:
        f.write("! %s 6-31G* TightOpt VeryTightSCF NoAutoStart NoFrozenCore\n%%scf MaxIter 300 end\n"
                "%%geom Constraints\n %s\n end\nend\n* xyz 0 1\n" % (kw, constraint) + geometry_lines(atoms) + "*\n")
    txt = subprocess.run([ORCA, inp], cwd=work, capture_output=True, text=True).stdout
    if "THE OPTIMIZATION HAS CONVERGED" not in txt:
        raise RuntimeError("orca did not converge: " + txt[-400:])
    e = float(re.findall(r"FINAL SINGLE POINT ENERGY\s+(-?\d+\.\d+)", txt)[-1])
    geo = []
    for line in txt.split("CARTESIAN COORDINATES (ANGSTROEM)")[-1].split("\n")[2:]:
        m = re.match(r"\s*([A-Z][a-z]?)\s+(-?\d+\.\d+)\s+(-?\d+\.\d+)\s+(-?\d+\.\d+)", line)
        if not m:
            break
        geo.append([m.group(1)] + [float(v) for v in m.groups()[1:]])
    return e, geo


def bonds(geo, centre=0):
    """Distances from atom `centre` to every real atom that is not a dummy and not itself."""
    return [math.dist(geo[centre][1:], a[1:]) for i, a in enumerate(geo) if i != centre and a[0] != "X"]


def compare(label, ref, got, rows):
    de = got[0] - ref[0]
    dr = max(abs(a - b) for a, b in zip(bonds(ref[1]), bonds(got[1])))
    ok = abs(de) < TOL_E and dr < TOL_R
    rows.append((label, ref[0], got[0], de, dr, ok))
    return ok


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    quick = "--quick" in sys.argv
    if not args:
        sys.exit("usage: constraint_test.py ECCE_QM_EXE [--quick]")
    exe = args[0]
    have_nw, have_orca = NWCHEM and MPIRUN, ORCA
    if not have_nw and not have_orca:
        print("neither nwchem nor ORCA is installed")
        sys.exit(77)
    rows, work = [], tempfile.mkdtemp(prefix="qmcons")
    try:
        if have_nw:
            cases = [(110, "hf")] if quick else [(a, m) for a in (100, 110, 120) for m in ("hf", "b3lyp")]
            for angle, method in cases:
                atoms = nh3x(angle)
                qc = "".join("angle 5 1 %d %d\n" % (h, angle) for h in (2, 3, 4))
                zc = "".join("  angle 5 1 %d %d constant\n" % (h, angle) for h in (2, 3, 4))
                d = tempfile.mkdtemp(dir=work)
                ref = run_nwchem(d, atoms, method, zc)
                got = run_qm(exe, d, atoms, method, qc)
                compare("NH3+X %d deg %s vs NWChem" % (angle, method.upper()), ref, got, rows)
        if have_orca:
            cases = [("bond 1 2 1.05\n", "{ B 0 1 1.05 C }", "O-H 1.05"),
                     ("angle 2 1 3 95\n", "{ A 1 0 2 95 C }", "H-O-H 95")]
            if quick:
                cases = cases[:1]
            for qc, oc, name in cases:
                d = tempfile.mkdtemp(dir=work)
                atoms = water()
                ref = run_orca(d, atoms, "hf", oc)
                got = run_qm(exe, d, atoms, "hf", qc)
                compare("H2O %s HF vs ORCA" % name, ref, got, rows)
    finally:
        shutil.rmtree(work, ignore_errors=True)
    print("%-34s %16s %16s %10s %9s" % ("case", "reference", "ecce-qm", "dE", "max dR"))
    for label, e0, e1, de, dr, ok in rows:
        print("%-34s %16.8f %16.8f %10.2e %9.5f %s" % (label, e0, e1, de, dr, "ok" if ok else "FAIL"))
    sys.exit(0 if all(r[-1] for r in rows) else 1)


if __name__ == "__main__":
    main()
