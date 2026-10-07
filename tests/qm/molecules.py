"""Geometries (angstrom) of the oracle test molecules.  Fixed, not optimised:
the oracle only needs both programs to see the same coordinates."""
import math

def _h2o():
    return [("O", 0, 0, 0.1173), ("H", 0, 0.7572, -0.4692), ("H", 0, -0.7572, -0.4692)]

def _nh3():
    r, hnh = 1.012, math.radians(106.7)
    # angle theta from the C3 axis: cos(hnh) = cos^2 t + sin^2 t cos(120)
    c = (math.cos(hnh) + 0.5) / 1.5
    t = math.acos(math.sqrt(c))
    atoms = [("N", 0, 0, 0.0)]
    for k in range(3):
        p = math.radians(120 * k)
        atoms.append(("H", r * math.sin(t) * math.cos(p), r * math.sin(t) * math.sin(p), -r * math.cos(t)))
    return atoms

def _diatomic(a, b, d):
    return [(a, 0, 0, 0), (b, 0, 0, d)]

def _ch4():
    r = 1.087 / math.sqrt(3)
    return [("C", 0, 0, 0), ("H", r, r, r), ("H", r, -r, -r), ("H", -r, r, -r), ("H", -r, -r, r)]

def _benzene():
    atoms = []
    for k in range(6):
        a = math.radians(60 * k)
        atoms.append(("C", 1.397 * math.cos(a), 1.397 * math.sin(a), 0))
    for k in range(6):
        a = math.radians(60 * k)
        atoms.append(("H", 2.481 * math.cos(a), 2.481 * math.sin(a), 0))
    return atoms

# name -> (charge, multiplicity, atoms)
MOLECULES = {
    "h2o": (0, 1, _h2o()),
    "nh3": (0, 1, _nh3()),
    "co": (0, 1, _diatomic("C", "O", 1.128)),
    "ch4": (0, 1, _ch4()),
    "n2": (0, 1, _diatomic("N", "N", 1.098)),
    "benzene": (0, 1, _benzene()),
    "c2": (0, 1, _diatomic("C", "C", 1.2425)),
    "o2": (0, 3, _diatomic("O", "O", 1.2075)),
    "he2": (0, 1, _diatomic("He", "He", 3.0)),
    "hf": (0, 1, _diatomic("H", "F", 0.917)),
}
