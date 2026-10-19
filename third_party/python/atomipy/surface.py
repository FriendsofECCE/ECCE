"""Surface / slab builder — oriented supercell exposing a Miller (hkl) face.

:func:`make_slab` builds a periodic supercell whose (hkl) plane is perpendicular
to z (the surface lies in the xy-plane), optionally stacked over several layers
and capped with a vacuum gap for surface simulations.

Method (clean-room, standard crystallography):

1. Reduce (h, k, l) to coprime indices.
2. Find two lattice vectors lying IN the plane (h·u = 0) and one stacking vector
   via the extended-gcd construction — a unimodular basis change of the same
   lattice, so the atom count per cell is preserved.
3. Re-express atoms in the new cell and wrap into it.
4. Stack ``layers`` repeats along the surface normal.
5. Standardise to GROMACS lower-triangular form (a ∥ x, b in xy-plane).
6. Optionally add vacuum along z (slab), and either WARN if the resulting box
   breaks GROMACS tilt limits or REDUCE it when ``gromacs_box=True``.

GROMACS note: the mathematically natural oriented cell can have large tilts that
GROMACS rejects (it requires |b_x| ≤ a_x/2, etc.). Set ``gromacs_box=True`` to
reduce the box (an "ad-hoc" lattice shift that wraps atoms back in) so it is
ready for GROMACS; leave it ``False`` (default) to keep the natural cell — a
warning is emitted if that cell is not GROMACS-legal. Many users (OpenMM,
LAMMPS, analysis-only) do not need the reduction.
"""

import math
import warnings

import numpy as np

from .miller import _from_frac_matrix, hkil_to_hkl


def _ext_gcd(a, b):
    """Extended Euclid: return (x, y) with a·x + b·y = gcd(a, b)."""
    if b == 0:
        return 1, 0
    if a % b == 0:
        return 0, 1
    x, y = _ext_gcd(b, a % b)
    return y, x - y * (a // b)


def _surface_basis(h, k, l, L):
    """Integer 3×3 basis (rows c1, c2 in-plane; c3 stacking) for plane (hkl).

    L holds the cell vectors as rows. The two in-plane rows are chosen as
    orthogonal as the metric allows (ASE-style k1/k2 optimisation).
    """
    zeros = [h == 0, k == 0, l == 0]
    if sum(zeros) >= 2:
        if h != 0:
            return np.array([[0, 1, 0], [0, 0, 1], [1, 0, 0]])
        if k != 0:
            return np.array([[0, 0, 1], [1, 0, 0], [0, 1, 0]])
        return np.array([[1, 0, 0], [0, 1, 0], [0, 0, 1]])

    a1, a2, a3 = L[0], L[1], L[2]
    p, q = _ext_gcd(k, l)

    k1 = np.dot(p * (k * a1 - h * a2) + q * (l * a1 - h * a3), l * a2 - k * a3)
    k2 = np.dot(l * (k * a1 - h * a2) - k * (l * a1 - h * a3), l * a2 - k * a3)
    if abs(k2) > 1e-10:
        i = -int(round(k1 / k2))
        p, q = p + i * l, q - i * k

    a, b = _ext_gcd(p * k + q * l, h)
    g = math.gcd(int(l), int(k)) or 1
    c1 = np.array([p * k + q * l, -p * h, -q * h])
    c2 = np.array([0, l // g, -k // g])
    c3 = np.array([b, a * p, a * q])
    return np.array([c1, c2, c3])


def _standardize_cell(V):
    """Rotate cell rows V into GROMACS lower-triangular form (a ∥ x, b in xy)."""
    a1, a2, a3 = V
    ax = np.linalg.norm(a1)
    bx = np.dot(a2, a1) / ax
    by = math.sqrt(max(np.dot(a2, a2) - bx * bx, 0.0))
    cx = np.dot(a3, a1) / ax
    cy = (np.dot(a2, a3) - bx * cx) / by
    cz = math.sqrt(max(np.dot(a3, a3) - cx * cx - cy * cy, 0.0))
    return np.array([[ax, 0.0, 0.0], [bx, by, 0.0], [cx, cy, cz]])


def _gromacs_reduce(L, reduce_c):
    """Reduce tilts so |b_x|,|c_x| ≤ a_x/2 and |c_y| ≤ b_y/2 (GROMACS limits)."""
    L = L.astype(float).copy()
    a1, a2, a3 = L
    if reduce_c:
        while a3[1] > a2[1] / 2:
            a3 -= a2
        while a3[1] < -a2[1] / 2:
            a3 += a2
        while a3[0] > a1[0] / 2:
            a3 -= a1
        while a3[0] < -a1[0] / 2:
            a3 += a1
    while a2[0] > a1[0] / 2:
        a2 -= a1
    while a2[0] < -a1[0] / 2:
        a2 += a1
    return np.array([a1, a2, a3])


def _box_dim_from_cell(L):
    """GROMACS 9-element Box_dim from a lower-triangular cell (rows a, b, c)."""
    ax, _, _ = L[0]
    bx, by, _ = L[1]
    cx, cy, cz = L[2]
    return [float(ax), float(by), float(cz), 0.0, 0.0, float(bx), 0.0, float(cx), float(cy)]


def _violates_gromacs(L, check_c):
    a1, a2, a3 = L
    eps = 1e-6
    if abs(a2[0]) > a1[0] / 2 + eps:
        return True
    if check_c and (abs(a3[0]) > a1[0] / 2 + eps or abs(a3[1]) > a2[1] / 2 + eps):
        return True
    return False


def make_slab(atoms, Box, hkl, layers=1, vacuum=0.0, gromacs_box=False,
              reindex=True, tol=1e-8):
    """Build an oriented supercell / slab exposing the (hkl) face along z.

    Parameters
    ----------
    atoms : list of dict
        Atom records with 'x', 'y', 'z' (Cartesian, Angstrom).
    Box : sequence
        3/6/9-element cell of the input crystal.
    hkl : sequence of int
        Miller indices (h, k, l) — or Miller–Bravais (h, k, i, l), auto-reduced.
    layers : int
        Number of (hkl) layers to stack along the surface normal (default 1).
    vacuum : float
        Vacuum gap (Angstrom) added along z; >0 makes a free-standing slab
        (z becomes non-periodic). 0 keeps a fully periodic oriented supercell.
    gromacs_box : bool
        If True, reduce the box to GROMACS tilt limits (atoms wrapped back in).
        If False (default), keep the natural cell and WARN if it isn't
        GROMACS-legal.
    reindex : bool
        Renumber surviving atoms' 'index' 1..N (default True).
    tol : float
        Wrapping tolerance.

    Returns
    -------
    (atoms, Box) : (list of dict, list)
        Slab atoms (new list; originals untouched) and the 9-element Box_dim.
    """
    if not atoms:
        return [], list(np.asarray(Box, dtype=float).ravel()) if Box is not None else []

    hkl = list(hkl)
    if len(hkl) == 4:
        hkl = list(hkil_to_hkl(*hkl))
    h, k, l = (int(round(v)) for v in hkl)
    if h == 0 and k == 0 and l == 0:
        raise ValueError("At least one of h, k, l must be non-zero.")
    g = math.gcd(math.gcd(abs(h), abs(k)), abs(l)) or 1
    h, k, l = h // g, k // g, l // g
    layers = max(1, int(layers))

    M = _from_frac_matrix(Box)      # columns a, b, c
    L = M.T                          # rows a, b, c
    B = _surface_basis(h, k, l, L)
    if np.linalg.det(B) < 0:         # keep right-handed
        B = B[[1, 0, 2]]
    V = B @ L                        # new cell vectors (rows)

    cart = np.array([[float(a.get("x", 0.0)), float(a.get("y", 0.0)), float(a.get("z", 0.0))] for a in atoms])
    F_old = cart @ np.linalg.inv(L)              # old fractional
    Binv = np.linalg.inv(B)
    F_new = F_old @ Binv                          # fractional in new cell
    F_new -= np.floor(F_new + tol)                # wrap into [0, 1)

    # Stack `layers` repeats along the new c-axis (the surface normal direction).
    if layers > 1:
        tiles = [np.column_stack([F_new[:, 0], F_new[:, 1], (F_new[:, 2] + i) / layers]) for i in range(layers)]
        F_new = np.vstack(tiles)
        base = atoms * layers
        V = np.array([V[0], V[1], V[2] * layers])
    else:
        base = atoms

    L_std = _standardize_cell(V)
    cart_new = F_new @ L_std

    periodic_c = vacuum <= 0
    if vacuum > 0:
        zmin = cart_new[:, 2].min()
        zmax = cart_new[:, 2].max()
        thickness = zmax - zmin
        Lz = thickness + vacuum
        cart_new[:, 2] += (vacuum / 2.0 - zmin)
        L_std = np.array([L_std[0], L_std[1], [0.0, 0.0, Lz]])

    if gromacs_box:
        L_red = _gromacs_reduce(L_std, reduce_c=periodic_c)
        if not np.allclose(L_red, L_std):
            # re-wrap atoms into the reduced cell (xy always; z only if periodic)
            frac = cart_new @ np.linalg.inv(L_red)
            frac[:, 0] -= np.floor(frac[:, 0] + tol)
            frac[:, 1] -= np.floor(frac[:, 1] + tol)
            if periodic_c:
                frac[:, 2] -= np.floor(frac[:, 2] + tol)
            cart_new = frac @ L_red
        L_std = L_red
    elif _violates_gromacs(L_std, check_c=periodic_c):
        warnings.warn(
            "make_slab: the oriented cell exceeds GROMACS tilt limits "
            "(|b_x| ≤ a_x/2, etc.). It is valid for OpenMM/LAMMPS/analysis, but "
            "pass gromacs_box=True to reduce it for GROMACS.",
            stacklevel=2,
        )

    out = []
    for i, a in enumerate(base):
        na = dict(a)
        na["x"], na["y"], na["z"] = float(cart_new[i, 0]), float(cart_new[i, 1]), float(cart_new[i, 2])
        if reindex:
            na["index"] = i + 1
        out.append(na)

    return out, _box_dim_from_cell(L_std)
