"""The teaching calculations and what each must show.

Settings are what a student picks in the Calculation Editor: code NWChem,
runtype Geometry (Energy for a lone atom), theory RHF/ROHF or RDFT with the
dialogs' own defaults, and the Basis Set Tool's library names.
"""

import re

import analysis as A
import geometry as G
import props as P


class Case(object):
    def __init__(self, name, title, atoms, group, theory, basis, charge=0, mult=1,
                 runtype="Geometry", checks=(), timeout=1500):
        self.name, self.title, self.atoms, self.group = name, title, atoms, group
        self.theory, self.basis = theory, basis
        self.charge, self.mult, self.runtype = charge, mult, runtype
        self.category = "DFT" if theory == "RDFT" else "SCF"
        self.checks, self.timeout = checks, timeout

    @property
    def settings(self):
        b = sorted(set(self.basis.values()))
        return "%s/%s%s" % (self.theory, "+".join(b),
                            " mult %d" % self.mult if self.mult != 1 else "")


def uniform(atoms, basis):
    return {a[0]: basis for a in atoms}


def case(name, title, atoms, group, theory, basis, **kw):
    b = basis if isinstance(basis, dict) else uniform(atoms, basis)
    return Case(name, title, atoms, group, theory, b, **kw)


# --- checks: each takes the Result and returns (ok, text) or None ----------

def pattern(ncore, occupied, lumo, what):
    """Valence occupied groups (degeneracy, occupation), then the LUMO group."""
    expect = list(occupied) + [lumo]

    def run(r):
        got, gs = A.valence_pattern(r.orbs, ncore)
        r.note("MO order", A.describe(gs))
        return got == expect, "%s: valence MO pattern %s (want %s); %s" % (
            what, got, expect, A.describe(gs))
    return run


def bond(i, j, lo, hi, what):
    def run(r):
        d = P.dist(r.final[i], r.final[j])
        r.note(what.split(" (")[0], "%.3f" % d)
        return lo <= d <= hi, "%s = %.3f A (expected %.2f-%.2f)" % (what, d, lo, hi)
    return run


def angle_of(key, center, ligands, lo, hi, what):
    def run(r):
        a = A.angles(r.final, center, ligands)
        mean = sum(a) / len(a)
        r.note(key, "%.2f" % mean)
        r.values[key] = mean
        return lo <= min(a) and max(a) <= hi, "%s %.2f deg (range %.2f-%.2f; expected %.1f-%.1f)" % (
            what, mean, min(a), max(a), lo, hi)
    return run


def degenerate_pair(offset_from_top, what):
    """The two orbitals `offset_from_top` from the HOMO are degenerate."""
    def run(r):
        occ = [o for o in r.orbs if o.occ > 0.5]
        a, b = occ[-1 - offset_from_top], occ[-2 - offset_from_top]
        r.note(what, "%.6f/%.6f" % (a.e, b.e))
        return abs(a.e - b.e) < 1e-4, "%s degenerate to 1e-4: %.6f %.6f (%s %s)" % (
            what, a.e, b.e, a.sym, b.sym)
    return run


def lumo_pair(what):
    def run(r):
        virt = [o for o in r.orbs if o.occ < 0.5]
        a, b = virt[0], virt[1]
        return abs(a.e - b.e) < 1e-4, "%s degenerate to 1e-4: %.6f %.6f (%s %s)" % (
            what, a.e, b.e, a.sym, b.sym)
    return run


def two_single(r):
    single = [o for o in r.orbs if abs(o.occ - 1.0) < 1e-6]
    ok = len(single) == 2 and abs(single[0].e - single[1].e) < 1e-4
    return ok, "two singly-occupied degenerate orbitals: %s" % (
        [(round(o.e, 6), o.sym, o.occ) for o in single])


def homo_lumo(r):
    occ = [o for o in r.orbs if o.occ > 0.5]
    virt = [o for o in r.orbs if o.occ < 0.5]
    r.note("HOMO/LUMO (Eh)", "%.4f/%.4f" % (occ[-1].e, virt[0].e))
    r.values["homo"], r.values["lumo"] = occ[-1].e, virt[0].e
    return occ[-1].e < virt[0].e < 0.5, "HOMO %.4f (%s), LUMO %.4f (%s) Eh" % (
        occ[-1].e, occ[-1].sym, virt[0].e, virt[0].sym)


def pi_count(nocc, what):
    def run(r):
        labels = A.pi_labels(r.group, r.final)
        if labels is None:
            return False, "%s: no pi irreps known for point group %s" % (what, r.group)
        occ = [o for o in r.orbs if o.occ > 0.5]
        n = sum(1 for o in occ if o.sym.lower() in labels)
        r.note("occupied pi", "%d (%s)" % (n, r.group))
        r.values["npi"] = n
        return n == nocc, "%s: %d occupied pi orbitals by label in %s (want %d); labels %s" % (
            what, n, r.group, nocc, sorted({o.sym for o in occ if o.sym.lower() in labels}))
    return run


def linear_pi_count(nocc, ncore, what):
    """Linear molecule: pi orbitals are the degenerate occupied valence pairs
    (the gerade/ungerade pair of 1s cores of a homonuclear molecule is
    degenerate too, so the cores are skipped)."""
    def run(r):
        gs = [g for g in A.groups(r.orbs[ncore:]) if g["occ"] > 0.5]
        n = sum(g["n"] for g in gs if g["n"] == 2)
        r.note("occupied pi", "%d" % n)
        r.values["npi"] = n
        return n == nocc, "%s: %d occupied pi orbitals as degenerate pairs (want %d); %s" % (
            what, n, nocc, A.describe(gs))
    return run


def deck_basis(expect):
    """{element: library name} in the generated deck's basis block."""
    def run(r):
        got = dict(re.findall(r'^\s*(\w+)\s+library\s+"([^"]+)"', r.deck, re.M))
        return got == expect, "deck basis block %s (want %s)" % (got, expect)
    return run


def final_energy_of(r):
    return r.te


#  The molecules.  Orbital patterns: (degeneracy, occupation) of the occupied
#  valence groups above the 1s cores, then the lowest empty group.
POPLE = "6-31G*"
DIFF = "6-31+G*"

CASES = [
    # Set A -- RHF/6-31G* (O2 triplet, ROHF)
    case("c2", "C2", G.diatomic("C", "C", 1.30), "A", "RHF", POPLE, checks=[
        bond(0, 1, 1.20, 1.30, "r(C-C)"),
        #  2sg 2su, then the pi pair (HOMO), then 3sg empty: pi(2p) below sigma(2p)
        pattern(2, [(1, 2), (1, 2), (2, 2)], (1, 0), "C2 pi(2p) below sigma(2p)")]),
    case("o2", "O2 (triplet)", G.diatomic("O", "O", 1.30), "A", "ROHF", POPLE, mult=3, checks=[
        bond(0, 1, 1.12, 1.22, "r(O-O)"),
        two_single,
        #  2sg 2su 3sg(sigma 2p) 1pu(pi 2p), then the half-filled pi* pair, sigma* empty
        pattern(2, [(1, 2), (1, 2), (1, 2), (2, 2), (2, 1)], (1, 0),
                "O2 sigma(2p) below pi(2p)")]),
    case("co", "CO", G.diatomic("C", "O", 1.25), "A", "RHF", POPLE, checks=[
        bond(0, 1, 1.08, 1.15, "r(C-O)"),
        #  3s 4s, 1pi pair, 5s HOMO above the pi pair; empty 2pi* pair
        pattern(2, [(1, 2), (1, 2), (2, 2), (1, 2)], (2, 0), "CO 5sigma above 1pi")]),
    case("hf", "HF", G.diatomic("H", "F", 1.05), "A", "RHF", POPLE, checks=[
        bond(0, 1, 0.88, 0.95, "r(H-F)"),
        pattern(1, [(1, 2), (1, 2), (2, 2)], (1, 0), "HF 2sigma 3sigma 1pi")]),
    case("he", "He atom", G.atom("He"), "A", "RDFT", DIFF, runtype="Energy"),
    case("he2", "He2", G.diatomic("He", "He", 3.0), "A", "RDFT", DIFF, checks=[
        bond(0, 1, 2.0, 99.0, "r(He-He) (no normal bond: > 2 A)")]),
    # Set B -- RDFT/6-31+G*
    case("ch4", "CH4", G.methane(1.15, 108.0), "B", "RDFT", DIFF, checks=[
        angle_of("HCH", 0, [1, 2, 3, 4], 109.17, 109.77, "H-C-H")]),
    case("nh3", "NH3", G.pyramid("N", "H", 1.05, 77.0), "B", "RDFT", DIFF, checks=[
        angle_of("HNH", 0, [1, 2, 3], 104.0, 109.4, "H-N-H")]),
    case("h2o", "H2O", G.bent("O", "H", 1.0, 100.0), "B", "RDFT", DIFF, checks=[
        angle_of("HOH", 0, [1, 2], 102.0, 107.0, "H-O-H")]),
    case("h2s", "H2S", G.bent("S", "H", 1.40, 98.0), "B", "RDFT", DIFF, checks=[
        angle_of("HSH", 0, [1, 2], 89.0, 95.0, "H-S-H"),
        deck_basis({"S": DIFF, "H": DIFF})]),
    case("h2se", "H2Se (Se 6-311G*, H 6-31+G*)", G.bent("Se", "H", 1.55, 95.0), "B", "RDFT",
         {"Se": "6-311G*", "H": DIFF}, checks=[
        angle_of("HSeH", 0, [1, 2], 88.0, 94.0, "H-Se-H"),
        deck_basis({"Se": "6-311G*", "H": DIFF})]),
    case("pf5", "PF5", G.pf5(), "B", "RDFT", DIFF, checks=[
        lambda r: _pf5(r)]),
    case("sf4_seesaw", "SF4 (seesaw start)", G.sf4_seesaw(), "B", "RDFT", DIFF),
    case("sf4_pyr", "SF4 (trigonal-pyramidal start)", G.sf4_pyramid(), "B", "RDFT", DIFF),
    case("clf3_t", "ClF3 (T-shaped start)", G.clf3_tshape(), "B", "RDFT", DIFF),
    case("clf3_plan", "ClF3 (trigonal-planar start)", G.clf3_planar(), "B", "RDFT", DIFF),
    # Set C -- RHF/6-31G*
    case("co2", "CO2", G.linear_triatomic("C", "O", 1.25), "C", "RHF", POPLE, checks=[
        bond(0, 1, 1.12, 1.17, "r(C=O)"), homo_lumo,
        linear_pi_count(4, 3, "CO2")]),
    case("no2m", "NO2-", [("N", 0.0, 0.0, 0.0), ("O", 1.15, 0.0, -0.6), ("O", -1.15, 0.0, -0.6)],
         "C", "RHF", POPLE, charge=-1, checks=[homo_lumo, pi_count(2, "NO2-")]),
    case("butadiene", "1,3-butadiene", G.polyene(4), "C", "RHF", POPLE, checks=[
        homo_lumo, pi_count(2, "butadiene")]),
    case("benzene", "benzene", G.benzene(), "C", "RHF", POPLE, checks=[
        homo_lumo, pi_count(3, "benzene"), degenerate_pair(0, "benzene HOMO pair"),
        lumo_pair("benzene LUMO pair")]),
    case("decapentaene", "all-trans C10H12", G.polyene(10), "C", "RHF", POPLE, timeout=3600,
         checks=[homo_lumo, pi_count(5, "C10H12")]),
]


def _pf5(r):
    a = []
    for n, i in enumerate(range(1, 6)):
        for j in range(i + 1, 6):
            a.append(P.angle(r.final[i], r.final[0], r.final[j]))
    near = lambda c: sum(1 for x in a if abs(x - c) < 3.0)
    r.note("F-P-F", "90:%d 120:%d 180:%d" % (near(90), near(120), near(180)))
    ok = near(90) == 6 and near(120) == 3 and near(180) == 1
    return ok, "F-P-F angles: %d near 90 (want 6), %d near 120 (want 3), %d near 180 (want 1)" % (
        near(90), near(120), near(180))


#  Comparisons across calculations: (name, function(results) -> (ok, text))
def compare(results):
    out = []

    def val(n, k):
        r = results.get(n)
        return r.values.get(k) if r else None

    a = [val("ch4", "HCH"), val("nh3", "HNH"), val("h2o", "HOH")]
    if None not in a:
        out.append((a[0] > a[1] > a[2], "angle order CH4 %.2f > NH3 %.2f > H2O %.2f" % tuple(a)))
    b = [val("h2o", "HOH"), val("h2s", "HSH"), val("h2se", "HSeH")]
    if None not in b:
        out.append((b[0] > b[1] > b[2], "angle order H2O %.2f > H2S %.2f > H2Se %.2f" % tuple(b)))
    for lower, other, what in (("sf4_seesaw", "sf4_pyr", "SF4"), ("clf3_t", "clf3_plan", "ClF3")):
        r1, r2 = results.get(lower), results.get(other)
        if r1 and r2 and r1.te is not None and r2.te is not None:
            d = (r2.te - r1.te) * 627.509
            same = abs(d) < 0.06
            out.append((r1.te <= r2.te + 1e-4,
                        "%s: %s start %.6f, %s start %.6f Eh (%s%.2f kcal/mol)%s" % (
                            what, lower, r1.te, other, r2.te,
                            "+" if d >= 0 else "", d,
                            "; both starts relaxed into the same structure" if same else "")))
    he2, atom = results.get("he2"), results.get("he")
    if he2 and atom and he2.te is not None and atom.te is not None:
        eb = (he2.te - 2 * atom.te) * 627.509
        out.append((True, "He2 binding energy %.4f kcal/mol (E(He2) - 2 E(He))" % eb))
        he2.note("Eb (kcal/mol)", "%.4f" % eb)
    return out
