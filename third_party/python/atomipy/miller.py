"""Miller-plane geometry.

Compute the polygon(s) where a set of (h, k, l) lattice planes intersect the
simulation cell — useful for visualising crystallographic planes on top of a
structure. Pure geometry (no plotting); ported from the MATLAB atom toolbox
``show_miller.m``.

Main entry point: :func:`miller_planes`. Helper: :func:`d_spacing`.
"""

import numpy as np

from .cell_utils import Box_dim2Cell

# 8 corners of the fractional unit cube and its 12 edges (0-based vertex indices).
_VFRAC = np.array(
    [
        [0, 0, 0], [1, 0, 0], [0, 1, 0], [1, 1, 0],
        [0, 0, 1], [1, 0, 1], [0, 1, 1], [1, 1, 1],
    ],
    dtype=float,
)
_EDGES = np.array(
    [
        [0, 1], [2, 3], [4, 5], [6, 7],
        [0, 2], [1, 3], [4, 6], [5, 7],
        [0, 4], [1, 5], [2, 6], [3, 7],
    ]
)


def _cell_from_box(Box):
    """Return (a, b, c, alpha, beta, gamma) [deg] from a 3/6/9-element Box."""
    box = list(np.asarray(Box, dtype=float).ravel())
    n = len(box)
    if n == 3:
        a, b, c = box
        return a, b, c, 90.0, 90.0, 90.0
    if n == 6:
        return tuple(box)
    if n == 9:
        cell = Box_dim2Cell(box)
        return tuple(float(x) for x in cell[:6])
    raise ValueError("Box must have 3, 6, or 9 elements.")


def _from_frac_matrix(Box):
    """3x3 matrix mapping fractional -> Cartesian coordinates (columns a, b, c)."""
    a, b, c, al, be, ga = _cell_from_box(Box)
    al, be, ga = np.radians([al, be, ga])
    v = np.sqrt(
        max(
            0.0,
            1 - np.cos(al) ** 2 - np.cos(be) ** 2 - np.cos(ga) ** 2
            + 2 * np.cos(al) * np.cos(be) * np.cos(ga),
        )
    )
    return np.array(
        [
            [a, b * np.cos(ga), c * np.cos(be)],
            [0, b * np.sin(ga), c * (np.cos(al) - np.cos(be) * np.cos(ga)) / np.sin(ga)],
            [0, 0, c * v / np.sin(ga)],
        ]
    )


def hkil_to_hkl(h, k, i=None, l=0):
    """Convert hexagonal Miller-Bravais (h k i l) PLANE indices to Miller (h k l).

    For planes the third index is redundant: i = -(h + k). It is simply dropped
    (the hexagonal cell geometry is carried by the Box). If ``i`` is supplied and
    inconsistent, a ValueError is raised.

    Example: quartz (10-10) -> (100), (10-11) -> (101), (11-20) -> (110),
    (0001) -> (001).
    """
    h, k, l = int(round(h)), int(round(k)), int(round(l))
    if i is not None:
        i = int(round(i))
        if i != -(h + k):
            raise ValueError(
                f"Inconsistent Miller-Bravais indices: i must equal -(h+k) = {-(h + k)}, got {i}."
            )
    return (h, k, l)


def d_spacing(h, k, l, Box):
    """Interplanar spacing d_hkl (Angstrom) for the (h, k, l) family.

    Computed from the real-space metric tensor G = MᵀM (M = fractional→Cartesian
    matrix) as 1/d² = [h k l] · G⁻¹ · [h k l]ᵀ.
    """
    M = _from_frac_matrix(Box)
    G = M.T @ M
    hkl = np.array([h, k, l], dtype=float)
    inv_d2 = float(hkl @ np.linalg.inv(G) @ hkl)
    if inv_d2 <= 0:
        return float("inf")
    return 1.0 / np.sqrt(inv_d2)


def _unique_rows(arr, tol=1e-10):
    out = []
    for row in arr:
        if not any(np.allclose(row, o, atol=tol) for o in out):
            out.append(row)
    return np.array(out)


def _polygon_area3d(P):
    c0 = P.mean(axis=0)
    acc = np.zeros(3)
    m = len(P)
    for i in range(m):
        acc = acc + np.cross(P[i] - c0, P[(i + 1) % m] - c0)
    return 0.5 * float(np.linalg.norm(acc))


def _intersect_polygon(FromFrac, hkl, s, exclude_boundary):
    """Ordered Cartesian polygon where plane h·x+k·y+l·z = s cuts the unit cube."""
    h, k, l = hkl
    pts = []
    for e0, e1 in _EDGES:
        v0 = _VFRAC[e0]
        v1 = _VFRAC[e1]
        dv = v1 - v0
        denom = h * dv[0] + k * dv[1] + l * dv[2]
        if abs(denom) < 1e-14:
            continue
        t = (s - (h * v0[0] + k * v0[1] + l * v0[2])) / denom
        if -1e-12 <= t <= 1 + 1e-12:
            p = v0 + t * dv
            if np.all(p >= -1e-12) and np.all(p <= 1 + 1e-12):
                pts.append(np.clip(p, 0.0, 1.0))
    if len(pts) < 3:
        return None
    P = _unique_rows(np.array(pts))
    if P.shape[0] < 3:
        return None

    # Order the vertices in-plane by angle around their centroid.
    c0 = P.mean(axis=0)
    nvec = np.array(hkl, dtype=float)
    ref = np.array([1.0, 0.0, 0.0])
    if np.linalg.norm(np.cross(nvec, ref)) < 1e-12:
        ref = np.array([0.0, 1.0, 0.0])
    if np.linalg.norm(np.cross(nvec, ref)) < 1e-12:
        ref = np.array([0.0, 0.0, 1.0])
    u = np.cross(nvec, ref)
    nu = np.linalg.norm(u)
    if nu < 1e-14:
        _, _, Vt = np.linalg.svd(P - c0)
        u, w = Vt[0], Vt[1]
    else:
        u = u / nu
        w = np.cross(nvec, u)
        w = w / np.linalg.norm(w)
    ang = np.arctan2((P - c0) @ w, (P - c0) @ u)
    P = P[np.argsort(ang)]

    if exclude_boundary and _polygon_area3d(P) <= 1e-12:
        return None
    return (FromFrac @ P.T).T  # fractional -> Cartesian


def cut_planes(atoms, Box, planes, whole_molecules=False, reindex=True):
    """Keep only atoms that satisfy ALL given Miller half-spaces (intersection).

    Each plane carves a half-space; intersecting several carves a convex region
    — e.g. 6 side planes (60° apart) cut a **hexagonal column** out of a clay
    layer, optionally capped by 2 basal planes. In fractional coordinates each
    plane is a linear threshold f = h·xf + k·yf + l·zf = s, so the whole cut is
    a logical AND of comparisons (works for any (hkl) and triclinic cell).

    Parameters
    ----------
    atoms : list of dict
        Atom records with 'x', 'y', 'z' (Cartesian, Angstrom).
    Box : sequence
        3/6/9-element cell (see :func:`miller_planes`).
    planes : list of dict
        Each plane: ``{'h','k','l', 'side': 'below'|'above', 'level': 'auto'|float,
        'offset': float}``. 'below' keeps f <= s, 'above' keeps f >= s; 'auto'
        level = midpoint of the structure along that normal; offset shifts the
        plane along its normal in Angstrom (Δs = offset / d_hkl).
    whole_molecules : bool
        Keep/drop whole molecules by centroid (uses 'molid') so molecules aren't
        sliced.
    reindex : bool
        Renumber surviving atoms' 'index' 1..N (default True).

    Returns
    -------
    list of dict
        Surviving atoms (new list; originals untouched).
    """
    if not atoms:
        return []
    if not planes:
        return [dict(a) for a in atoms]

    M = _from_frac_matrix(Box)
    Minv = np.linalg.inv(M)
    cart = np.array([[float(a.get("x", 0.0)), float(a.get("y", 0.0)), float(a.get("z", 0.0))] for a in atoms])
    frac = cart @ Minv.T
    tol = 1e-9  # atoms exactly on a plane (and cos(90°)≈1e-16 noise) stay on the kept side
    molids = np.array([a.get("molid", i) for i, a in enumerate(atoms)]) if whole_molecules else None

    mask = np.ones(len(atoms), dtype=bool)
    for pl in planes:
        h, k, l = int(round(pl["h"])), int(round(pl["k"])), int(round(pl["l"]))
        if h == 0 and k == 0 and l == 0:
            continue
        hkl = np.array([h, k, l], dtype=float)
        f = frac @ hkl
        level = pl.get("level", "auto")
        s = 0.5 * (float(f.min()) + float(f.max())) if isinstance(level, str) else float(level)
        off = pl.get("offset", 0.0)
        if off:
            d = d_spacing(h, k, l, Box)
            if np.isfinite(d) and d > 0:
                s += off / d
        side = pl.get("side", "below")
        if whole_molecules:
            pm = np.zeros(len(atoms), dtype=bool)
            for mid in np.unique(molids):
                sel = molids == mid
                mf = float(f[sel].mean())
                pm[sel] = (mf <= s + tol) if side == "below" else (mf >= s - tol)
            mask &= pm
        else:
            mask &= (f <= s + tol) if side == "below" else (f >= s - tol)

    kept = [dict(a) for a, m in zip(atoms, mask) if m]
    if reindex:
        for i, a in enumerate(kept):
            a["index"] = i + 1
    return kept


def cut_miller(atoms, Box, h, k, l, level="auto", offset=0.0, side="below",
               whole_molecules=False, reindex=True):
    """Keep atoms on one side of a single (h, k, l) Miller plane.

    Convenience wrapper around :func:`cut_planes` for the common single-plane
    case. 'below' keeps the inner side (f <= s), 'above' the outer side. See
    :func:`cut_planes` for cutting a convex region with several planes.
    """
    if int(round(h)) == 0 and int(round(k)) == 0 and int(round(l)) == 0:
        raise ValueError("At least one of h, k, l must be non-zero.")
    return cut_planes(
        atoms, Box,
        [{"h": h, "k": k, "l": l, "side": side, "level": level, "offset": offset}],
        whole_molecules=whole_molecules, reindex=reindex,
    )


def miller_planes(
    h,
    k,
    l,
    Box=(1, 1, 1),
    single_plane=False,
    plane_level="auto",
    offset=0.0,
    exclude_boundary=False,
):
    """Polygon(s) where the (h, k, l) Miller plane(s) intersect the cell.

    Parameters
    ----------
    h, k, l : int
        Miller indices (any sign combination; at least one must be non-zero).
    Box : sequence, optional
        Cell as 3 ``[a, b, c]``, 6 ``[a, b, c, α, β, γ]``, or 9
        ``[lx, ly, lz, 0, 0, xy, 0, xz, yz]`` elements. Default unit cube.
    single_plane : bool, optional
        If False (default), return the **full family** of parallel (hkl) planes
        that intersect the cell. If True, return a single plane (at
        ``plane_level``).
    plane_level : int or 'auto', optional
        For ``single_plane=True``: the integer level n of the plane
        ``h·x + k·y + l·z = n`` (fractional coords). ``'auto'`` (default) picks
        the in-cell level nearest to 1.
    offset : float, optional
        Shift the plane(s) along their normal, in Angstrom. Converted to a
        fractional-level shift Δ = offset / d_hkl, so ``offset = d_spacing(...)``
        moves the plane(s) by exactly one interplanar spacing. Default 0.
    exclude_boundary : bool, optional
        Drop degenerate (zero-area, edge/vertex-only) contacts. Default False.

    Returns
    -------
    list of numpy.ndarray
        One (N×3) array of ordered Cartesian vertices per plane polygon; empty
        if no plane intersects the cell.
    """
    h, k, l = int(round(h)), int(round(k)), int(round(l))
    if h == 0 and k == 0 and l == 0:
        raise ValueError("At least one of h, k, l must be non-zero.")

    FromFrac = _from_frac_matrix(Box)
    hkl = (h, k, l)

    # Convert an Angstrom offset along the normal to a fractional-level shift.
    ds = 0.0
    if offset:
        d = d_spacing(h, k, l, Box)
        ds = offset / d if np.isfinite(d) and d > 0 else 0.0

    svals = _VFRAC @ np.array([h, k, l], dtype=float)
    nmin = int(np.ceil(svals.min() - ds))
    nmax = int(np.floor(svals.max() - ds))

    polys = []
    if single_plane:
        if isinstance(plane_level, str):  # 'auto' -> nearest usable level to 1
            levels = sorted(range(nmin, nmax + 1), key=lambda n: abs(n - 1))
        else:
            levels = [int(round(plane_level))]
        for n in levels:
            poly = _intersect_polygon(FromFrac, hkl, n + ds, exclude_boundary)
            if poly is not None:
                polys.append(poly)
                break
    else:
        for n in range(nmin, nmax + 1):
            poly = _intersect_polygon(FromFrac, hkl, n + ds, exclude_boundary)
            if poly is not None:
                polys.append(poly)
    return polys
