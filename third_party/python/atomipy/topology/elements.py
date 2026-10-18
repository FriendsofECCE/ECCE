"""Minimal element table (symbol -> atomic number, standard atomic weight in
amu). Raw-imported atomipy atoms carry `element` but not `mass`; the adapter
fills mass from here when absent. Covers the common mineral/water/ion set with a
graceful fallback."""
from __future__ import annotations

from typing import Optional, Tuple

# symbol: (Z, mass)
ELEMENTS = {
    "H": (1, 1.00794), "He": (2, 4.002602),
    "Li": (3, 6.941), "Be": (4, 9.012182), "B": (5, 10.811),
    "C": (6, 12.0107), "N": (7, 14.0067), "O": (8, 15.9994),
    "F": (9, 18.9984032), "Ne": (10, 20.1797),
    "Na": (11, 22.98976928), "Mg": (12, 24.305), "Al": (13, 26.9815386),
    "Si": (14, 28.0855), "P": (15, 30.973762), "S": (16, 32.065),
    "Cl": (17, 35.453), "Ar": (18, 39.948),
    "K": (19, 39.0983), "Ca": (20, 40.078), "Sc": (21, 44.955912),
    "Ti": (22, 47.867), "V": (23, 50.9415), "Cr": (24, 51.9961),
    "Mn": (25, 54.938045), "Fe": (26, 55.845), "Co": (27, 58.933195),
    "Ni": (28, 58.6934), "Cu": (29, 63.546), "Zn": (30, 65.38),
    "Ga": (31, 69.723), "Ge": (32, 72.63), "As": (33, 74.9216),
    "Se": (34, 78.96), "Br": (35, 79.904), "Rb": (37, 85.4678),
    "Sr": (38, 87.62), "Y": (39, 88.90585), "Zr": (40, 91.224),
    "Mo": (42, 95.96), "Ag": (47, 107.8682), "Cd": (48, 112.411),
    "Sn": (50, 118.71), "I": (53, 126.90447), "Cs": (55, 132.9054519),
    "Ba": (56, 137.327), "La": (57, 138.90547), "Ce": (58, 140.116),
    "Pb": (82, 207.2), "U": (92, 238.02891),
}


def mass_of(element: Optional[str]) -> Optional[float]:
    if not element:
        return None
    return ELEMENTS.get(_norm(element), (None, None))[1]


def atomic_number_of(element: Optional[str]) -> Optional[int]:
    if not element:
        return None
    return ELEMENTS.get(_norm(element), (None, None))[0]


def _norm(sym: str) -> str:
    s = "".join(c for c in str(sym) if c.isalpha())
    if not s:
        return ""
    return s[0].upper() + s[1:].lower()
