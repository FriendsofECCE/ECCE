"""Parametric lattice / unit-cell builder.

Two entry points:

* :func:`build_cell` — the general "Bravais lattice + basis" builder. You give
  the cell parameters and a list of fractional basis atoms; it places them in
  Cartesian space (literal **P1** — what you give is what you get). Optional
  supercell replication.

* :func:`make_lattice` — convenience presets (fcc, bcc, sc, hcp, diamond,
  rocksalt, fluorite, perovskite) that just call :func:`build_cell` with a
  predefined cell + basis.

Symmetry (space-group) expansion is intentionally NOT done here yet; the basis
is taken verbatim. A future ``spacegroup=`` option can expand an asymmetric unit
before this placement step (atomipy already ships gemmi for that).
"""

import math

import numpy as np

from .miller import _from_frac_matrix
from .cell_utils import Cell2Box_dim, Box_dim2Cell


def _cell6(cell):
    """Normalize a 3/6/9-element cell to [a, b, c, alpha, beta, gamma] (deg)."""
    c = list(np.asarray(cell, dtype=float).ravel())
    if len(c) == 3:
        return [c[0], c[1], c[2], 90.0, 90.0, 90.0]
    if len(c) == 6:
        return [float(x) for x in c]
    if len(c) == 9:
        return [float(x) for x in Box_dim2Cell(c)[:6]]
    raise ValueError("cell must have 3, 6, or 9 elements")


def build_cell(cell, basis, replicate=(1, 1, 1), resname="XTL", start_index=1):
    """Build atoms from a unit cell and a fractional basis (literal P1).

    Parameters
    ----------
    cell : sequence
        Cell as 3 ``[a, b, c]``, 6 ``[a, b, c, α, β, γ]``, or 9
        ``[lx, ly, lz, 0, 0, xy, 0, xz, yz]`` elements (Å, degrees).
    basis : list of dict
        Each: ``{"element": "Na", "x": 0.0, "y": 0.0, "z": 0.5}`` with x/y/z
        FRACTIONAL. Optional ``"type"`` overrides the atom type (defaults to the
        element). The basis is placed verbatim — no symmetry expansion.
    replicate : (nx, ny, nz)
        Optional supercell factors (default no replication).
    resname : str
        Residue name for all atoms (default "XTL").
    start_index : int
        First atom index (default 1).

    Returns
    -------
    (atoms, Box) : (list of dict, list)
        Atom dicts with Cartesian x/y/z (Å) and the resulting Box_dim.
    """
    if not basis:
        raise ValueError("basis must contain at least one atom")
    cell6 = _cell6(cell)
    M = _from_frac_matrix(cell6)  # fractional -> Cartesian (columns a, b, c)
    nx, ny, nz = (max(1, int(r)) for r in replicate)

    atoms = []
    idx = int(start_index)
    for i in range(nx):
        for j in range(ny):
            for k in range(nz):
                for site in basis:
                    frac = np.array([
                        float(site["x"]) + i,
                        float(site["y"]) + j,
                        float(site["z"]) + k,
                    ])
                    cart = M @ frac
                    el = site.get("element") or site.get("type") or "X"
                    atoms.append({
                        "index": idx,
                        "element": el,
                        "type": site.get("type", el),
                        "resname": resname,
                        "molid": 1,
                        "x": float(cart[0]),
                        "y": float(cart[1]),
                        "z": float(cart[2]),
                    })
                    idx += 1

    super_cell = [cell6[0] * nx, cell6[1] * ny, cell6[2] * nz, cell6[3], cell6[4], cell6[5]]
    Box = Cell2Box_dim(super_cell)
    return atoms, Box


# Preset lattices: each defines its species count, whether the cell is hexagonal,
# and a list of (fractional_position, species_index) basis sites.
_PRESETS = {
    "sc":      {"n_species": 1, "hex": False, "basis": [((0, 0, 0), 0)]},
    "bcc":     {"n_species": 1, "hex": False, "basis": [((0, 0, 0), 0), ((0.5, 0.5, 0.5), 0)]},
    "fcc":     {"n_species": 1, "hex": False, "basis": [((0, 0, 0), 0), ((0.5, 0.5, 0), 0), ((0.5, 0, 0.5), 0), ((0, 0.5, 0.5), 0)]},
    "hcp":     {"n_species": 1, "hex": True,  "basis": [((0, 0, 0), 0), ((1 / 3, 2 / 3, 0.5), 0)]},
    "diamond": {"n_species": 1, "hex": False, "basis": [
        ((0, 0, 0), 0), ((0.5, 0.5, 0), 0), ((0.5, 0, 0.5), 0), ((0, 0.5, 0.5), 0),
        ((0.25, 0.25, 0.25), 0), ((0.75, 0.75, 0.25), 0), ((0.75, 0.25, 0.75), 0), ((0.25, 0.75, 0.75), 0),
    ]},
    "rocksalt": {"n_species": 2, "hex": False, "basis": [
        ((0, 0, 0), 0), ((0.5, 0.5, 0), 0), ((0.5, 0, 0.5), 0), ((0, 0.5, 0.5), 0),          # cation (fcc)
        ((0.5, 0.5, 0.5), 1), ((0, 0, 0.5), 1), ((0, 0.5, 0), 1), ((0.5, 0, 0), 1),          # anion (fcc + ½½½)
    ]},
    "fluorite": {"n_species": 2, "hex": False, "basis": [
        ((0, 0, 0), 0), ((0.5, 0.5, 0), 0), ((0.5, 0, 0.5), 0), ((0, 0.5, 0.5), 0),          # cation (fcc)
        ((0.25, 0.25, 0.25), 1), ((0.75, 0.75, 0.25), 1), ((0.75, 0.25, 0.75), 1), ((0.25, 0.75, 0.75), 1),
        ((0.75, 0.75, 0.75), 1), ((0.25, 0.25, 0.75), 1), ((0.25, 0.75, 0.25), 1), ((0.75, 0.25, 0.25), 1),  # 8 anions (tetrahedral)
    ]},
    "perovskite": {"n_species": 3, "hex": False, "basis": [
        ((0, 0, 0), 0),                                                                      # A (corner)
        ((0.5, 0.5, 0.5), 1),                                                                # B (body centre)
        ((0.5, 0.5, 0), 2), ((0.5, 0, 0.5), 2), ((0, 0.5, 0.5), 2),                          # O (face centres)
    ]},
}


def lattice_types():
    """Return the available preset lattice names."""
    return sorted(_PRESETS.keys())


def make_lattice(lattice, a, species, c=None, replicate=(1, 1, 1), resname=None):
    """Build a preset crystal lattice.

    Parameters
    ----------
    lattice : str
        One of: sc, bcc, fcc, hcp, diamond, rocksalt, fluorite, perovskite.
    a : float
        Lattice parameter a (Å).
    species : str or list of str
        Element(s) for the lattice's species — 1 for sc/bcc/fcc/hcp/diamond,
        2 for rocksalt/fluorite (cation, anion), 3 for perovskite (A, B, X).
    c : float, optional
        c parameter for hcp (default a·√(8/3) ≈ 1.633·a). Ignored for cubic.
    replicate : (nx, ny, nz)
        Optional supercell factors.
    resname : str, optional
        Residue name (default derived from the lattice name).

    Returns
    -------
    (atoms, Box)
    """
    lt = str(lattice).lower()
    if lt not in _PRESETS:
        raise ValueError(f"Unknown lattice '{lattice}'. Available: {', '.join(lattice_types())}")
    p = _PRESETS[lt]
    sp = [species] if isinstance(species, str) else list(species)
    if len(sp) != p["n_species"]:
        raise ValueError(f"Lattice '{lattice}' needs {p['n_species']} species, got {len(sp)}.")

    a = float(a)
    if p["hex"]:
        cc = float(c) if c else a * math.sqrt(8.0 / 3.0)
        cell = [a, a, cc, 90.0, 90.0, 120.0]
    else:
        cell = [a, a, a, 90.0, 90.0, 90.0]

    basis = [{"element": sp[si], "x": fx, "y": fy, "z": fz} for (fx, fy, fz), si in p["basis"]]
    return build_cell(cell, basis, replicate=replicate, resname=(resname or lt.upper()[:3]))
