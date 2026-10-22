"""Relaxed-scan cases shared by scan_test.py and gen_scan_oracle.py."""
import math


# Energy of one H atom in 6-31G* (UHF), hartree: the separated-atom limit of H2.
E_H = -0.498233


def ethane():
    """Staggered ethane, C-C 1.54, C-H 1.09, H-C-C 110 degrees.  Atoms: C, C,
    three H on the first (3..5), three on the second (6..8)."""
    rcc, rch, t = 1.54, 1.09, math.radians(110.0)
    atoms = [("C", 0.0, 0.0, 0.0), ("C", 0.0, 0.0, rcc)]
    for k in range(3):
        a = math.radians(120.0 * k)
        atoms.append(("H", rch * math.sin(t) * math.cos(a), rch * math.sin(t) * math.sin(a), rch * math.cos(t)))
    for k in range(3):
        a = math.radians(120.0 * k + 60.0)
        atoms.append(("H", rch * math.sin(t) * math.cos(a), rch * math.sin(t) * math.sin(a), rcc - rch * math.cos(t)))
    return atoms


def h2():
    return [("H", 0.0, 0.0, 0.0), ("H", 0.0, 0.0, 0.74)]


# scan: ecce-qm "scan" line (coordinate and atoms), from, to, points;
# orca_scan: the same coordinate, counted from 0.
CASES = {
    "ethane_hf_sto3g": dict(atoms=ethane, method="hf", orca="HF", basis="STO-3G", mult=1,
                            scan=("torsion 3 1 2 6", 60.0, 180.0, 13), orca_scan="D 2 0 1 5", quick=True),
    "ethane_b3lyp_631gs": dict(atoms=ethane, method="b3lyp", orca="B3LYP", basis="6-31G*", mult=1,
                               scan=("torsion 3 1 2 6", 60.0, 180.0, 13), orca_scan="D 2 0 1 5", quick=False),
    "h2_rhf_631gs": dict(atoms=h2, method="rhf", orca="RHF", basis="6-31G*", mult=1,
                         scan=("bond 1 2", 0.5, 3.0, 26), orca_scan="B 0 1", quick=True, limit="rhf"),
    "h2_uhf_631gs": dict(atoms=h2, method="uhf", orca="UHF", basis="6-31G*", mult=1,
                         scan=("bond 1 2", 0.5, 3.0, 26), orca_scan="B 0 1", quick=False, bs=True, limit="uhf"),
}


def qm_input(c, points=None):
    kind, a, b, n = c["scan"]
    n = points or n
    s = ("runtype optimize\nmethod %s\nbasis %s\nmultiplicity %d\nopt_conv tight\nmaxsteps 200\n"
         "conv_energy 1e-10\n" % (c["method"], c["basis"], c["mult"]))
    if c.get("bs"):
        s += "guess_mix true\n"
    s += "geometry\n" + "".join("%s %.8f %.8f %.8f\n" % at for at in c["atoms"]()) + "end\n"
    s += "scan\n%s %g %g %d\nend\n" % (kind, a, b, n)
    return s
