"""Geometric region cuts — carve nanoparticles, nanowires, and slabs.

Companions to the Miller-plane cut (:func:`atomipy.cut_planes`):

* :func:`cut_sphere`   — keep atoms inside/outside a sphere   → nanoparticle
* :func:`cut_cylinder` — keep atoms inside/outside a cylinder → nanowire / pore

Both default their centre to the cell centre (or the structure centroid if no
box is given) and share the ``whole_molecules`` / ``reindex`` semantics of the
Miller cut so molecules aren't sliced.
"""

import numpy as np

_TOL = 1e-9


def _coords(atoms):
    return np.array(
        [[float(a.get("x", 0.0)), float(a.get("y", 0.0)), float(a.get("z", 0.0))] for a in atoms]
    )


def _default_center(atoms, Box):
    """Cell centre (fractional 0.5,0.5,0.5) if a box is given, else the centroid."""
    if Box is not None:
        from .miller import _from_frac_matrix
        M = _from_frac_matrix(Box)
        return M @ np.array([0.5, 0.5, 0.5])
    return _coords(atoms).mean(axis=0)


def _finish(atoms, mask, reindex):
    kept = [dict(a) for a, m in zip(atoms, mask) if m]
    if reindex:
        for i, a in enumerate(kept):
            a["index"] = i + 1
    return kept


def _molecule_reduce(values, atoms):
    """Replace each atom's value with its molecule's mean (by 'molid')."""
    molids = np.array([a.get("molid", i) for i, a in enumerate(atoms)])
    out = values.copy()
    for mid in np.unique(molids):
        sel = molids == mid
        out[sel] = values[sel].mean()
    return out


def cut_sphere(atoms, Box=None, radius=10.0, center=None, side="inside",
               whole_molecules=False, reindex=True):
    """Keep atoms inside (or outside) a sphere — a spherical nanoparticle.

    Parameters
    ----------
    atoms : list of dict
        Atom records with 'x', 'y', 'z' (Cartesian, Angstrom).
    Box : sequence, optional
        3/6/9-element cell; used only to default the centre to the cell centre.
    radius : float
        Sphere radius in Angstrom.
    center : (cx, cy, cz), optional
        Sphere centre (Angstrom). Defaults to the cell centre / structure centroid.
    side : {'inside', 'outside'}
        Keep atoms with r <= radius ('inside') or r >= radius ('outside').
    whole_molecules : bool
        Decide per molecule by its centroid (uses 'molid') so molecules aren't cut.
    reindex : bool
        Renumber surviving atoms' 'index' 1..N (default True).

    Returns
    -------
    list of dict
        Surviving atoms (new list; originals untouched).
    """
    if not atoms:
        return []
    c = np.asarray(center, dtype=float) if center is not None else _default_center(atoms, Box)
    d = np.linalg.norm(_coords(atoms) - c, axis=1)
    if whole_molecules:
        d = _molecule_reduce(d, atoms)
    mask = (d <= radius + _TOL) if side == "inside" else (d >= radius - _TOL)
    return _finish(atoms, mask, reindex)


def cut_cylinder(atoms, Box=None, radius=10.0, axis="z", center=None, side="inside",
                 length=None, whole_molecules=False, reindex=True):
    """Keep atoms inside (or outside) a cylinder — a nanowire (or a pore).

    Parameters
    ----------
    atoms : list of dict
        Atom records with 'x', 'y', 'z' (Cartesian, Angstrom).
    Box : sequence, optional
        3/6/9-element cell; used only to default the centre to the cell centre.
    radius : float
        Cylinder radius in Angstrom (distance from the axis line).
    axis : {'x', 'y', 'z'} or 3-vector
        Cylinder axis direction.
    center : (cx, cy, cz), optional
        A point on the axis (Angstrom). Defaults to the cell centre / centroid.
    side : {'inside', 'outside'}
        Keep atoms with radial distance <= radius ('inside') or >= ('outside').
    length : float, optional
        If given (and side='inside'), also cap the cylinder to |along-axis| <=
        length/2 about the centre, giving a finite rod.
    whole_molecules : bool
        Decide per molecule by its centroid (uses 'molid') so molecules aren't cut.
    reindex : bool
        Renumber surviving atoms' 'index' 1..N (default True).

    Returns
    -------
    list of dict
        Surviving atoms (new list; originals untouched).
    """
    if not atoms:
        return []
    axis_map = {"x": [1.0, 0.0, 0.0], "y": [0.0, 1.0, 0.0], "z": [0.0, 0.0, 1.0]}
    u = np.asarray(axis_map[axis] if isinstance(axis, str) else axis, dtype=float)
    n = np.linalg.norm(u)
    if n == 0:
        raise ValueError("Cylinder axis must be non-zero.")
    u = u / n

    c = np.asarray(center, dtype=float) if center is not None else _default_center(atoms, Box)
    rel = _coords(atoms) - c
    along = rel @ u                      # signed distance along the axis
    perp = rel - np.outer(along, u)      # component perpendicular to the axis
    radial = np.linalg.norm(perp, axis=1)
    if whole_molecules:
        radial = _molecule_reduce(radial, atoms)
        along = _molecule_reduce(along, atoms)

    mask = (radial <= radius + _TOL) if side == "inside" else (radial >= radius - _TOL)
    if length is not None and side == "inside":
        mask &= np.abs(along) <= (float(length) / 2.0 + _TOL)
    return _finish(atoms, mask, reindex)
