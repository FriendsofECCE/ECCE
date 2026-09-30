"""Chemistry read off a finished calculation's stored properties."""

import math

import props as P

#  Irreps that are odd under the molecular plane (sigma-h), so pi-type, per
#  point group.  D2h and C2v depend on which Cartesian plane holds the atoms.
D2H_ODD = {"z": ("b2g", "b3g", "au", "b1u"),     # plane xy
           "y": ("b1g", "b3g", "au", "b2u"),     # plane xz
           "x": ("b1g", "b2g", "au", "b3u")}     # plane yz
C2V_ODD = {"y": ("a2", "b2"), "x": ("a2", "b1")}  # plane xz / yz
OTHER_ODD = {"c2h": ("au", "bg"),
             "d6h": ("a1u", "a2u", "b1g", "b2g", "e1g", "e2u"),
             "cs": ("a\"",), "c1": ()}


class Orbital(object):
    def __init__(self, e, occ, sym):
        self.e, self.occ, self.sym = e, occ, sym


def orbitals(pr):
    e, o = pr.vector("ORBENG"), pr.vector("ORBOCC")
    s = pr.strings("ORBSYM") if pr.has("ORBSYM") else ["?"] * len(e)
    return [Orbital(*t) for t in zip(e, o, s)]


def groups(orbs, tol=1e-4):
    """Consecutive orbitals within `tol` Hartree: [(n, occ, energy, syms)]."""
    out = []
    for o in orbs:
        if out and abs(o.e - out[-1]["e"]) < tol:
            out[-1]["n"] += 1
            out[-1]["syms"].append(o.sym)
        else:
            out.append({"n": 1, "occ": o.occ, "e": o.e, "syms": [o.sym]})
    return out


def valence_pattern(orbs, ncore, nvirt_groups=1):
    """[(degeneracy, occupation)] for the valence occupied groups, then the
    lowest `nvirt_groups` empty ones."""
    gs = groups(orbs[ncore:])
    occ = [g for g in gs if g["occ"] > 0.5]
    virt = [g for g in gs if g["occ"] <= 0.5][:nvirt_groups]
    return [(g["n"], round(g["occ"])) for g in occ + virt], occ + virt


def describe(pattern_groups):
    return " ".join("%s%s(%.3f)" % ("x".join(sorted(set(g["syms"]))),
                                    "" if g["n"] == 1 else "*%d" % g["n"], g["e"])
                    for g in pattern_groups)


def flat_axis(frame, tol=1e-3):
    """The Cartesian axis normal to a planar molecule, or None."""
    for i, name in enumerate("xyz"):
        if all(abs(a[i]) < tol for a in frame):
            return name
    return None


def pi_labels(group, frame):
    g = (group or "").lower()
    ax = flat_axis(frame)
    if g == "d2h" and ax:
        return D2H_ODD[ax]
    if g == "c2v" and ax in C2V_ODD:
        return C2V_ODD[ax]
    return OTHER_ODD.get(g)


def angles(frame, center, ligands):
    return sorted(P.angle(frame[i], frame[center], frame[j])
                  for n, i in enumerate(ligands) for j in ligands[n + 1:])
