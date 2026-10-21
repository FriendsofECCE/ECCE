"""Phyllosilicate structural-distortion analysis.

Tetrahedral rotation angle ``alpha`` for 2:1 / 1:1 clay minerals and micas.

``alpha`` quantifies the *ditrigonal distortion* of the hexagonal siloxane ring in a
tetrahedral sheet: adjacent SiO4 tetrahedra rotate alternately (+/-) about the sheet
normal to relieve the tetrahedral-octahedral size misfit, turning the ideal hexagonal
ring of basal oxygens into a ditrigonal (skewed, three-fold) ring. alpha = 0 deg is the
ideal hexagon; the geometric maximum is 30 deg; smectites/micas typically fall in
~1-15 deg (dioctahedral > trioctahedral).

Definition (Bailey 1984, *Rev. Mineral.* 19; Radoslovich 1961). The canonical measure is

    alpha = (1/2) * mean_i | 120deg - phi_i |

over the six basal O-O-O internal angles ``phi_i`` of the ditrigonal ring. This module
computes the equivalent, ring-perception-free **per-bridging-bond** form: for every
bridging (basal) oxygen ``O`` shared by two tetrahedral cations ``Ta``, ``Tb``, the
in-plane angle between ``Ta->O`` and ``Ta->Tb`` equals the rotation ``alpha`` of that
tetrahedron (in the ideal sheet the bridging O lies on the Ta-Tb line, so the angle is
0; a tetrahedral rotation by alpha carries the O off that line by alpha). Averaging this
angle over both cations of every bridging oxygen recovers Bailey's alpha, and is robust
to thermal disorder in MD frames because it needs only local connectivity, never a
traced six-membered ring.

Basal (bridging) oxygens are identified by connectivity — an O bonded to exactly two
tetrahedral cations — so the analysis does not depend on force-field atom-name spellings
(Ob/Op/...); it only needs the tetrahedral cations (Si by default, plus tetrahedral-Al
etc. by type) and the O positions.

Entry points
------------
tetrahedral_rotation(atoms, Box, ...)        one structure -> dict of results
tetrahedral_rotation_files(files, ...)       many conf*.gro/conf*.pdb -> pooled statistics
"""

import glob as _glob
import os as _os

import numpy as np

from .cell_utils import normalize_box
from .transform import get_cell_vectors

# Tetrahedral cations. Match by element (Si is unambiguously tetrahedral in clays) OR by
# force-field type (to catch tetrahedral substitutions that share an element with an
# octahedral site, e.g. Al -> Alt vs Alo). Override via tet_elements / tet_types.
_TET_ELEMENTS_DEFAULT = {"Si"}
_TET_TYPES_DEFAULT = {"Si", "Sit", "Alt", "Tit", "Fet", "Fee3", "Gat", "Bt", "Pt"}


def _cell_matrix(Box):
    """3x3 cell matrix H (rows = cell vectors a, b, c) from any Box (1x3/1x6/1x9)."""
    _bd, Cell = normalize_box(Box)
    return np.asarray(get_cell_vectors(list(Cell)), dtype=float)


def _mic(delta, H, Hinv):
    """Minimum-image displacement(s) of ``delta`` (..,3) via a fractional wrap.

    Triclinic-safe (correct for the mild tilts of clay/mica cells); reduces to the
    per-axis rule for orthogonal boxes.
    """
    f = delta @ Hinv
    f -= np.round(f)
    return f @ H


def _angle_in_plane(v1, v2, n):
    """Angle (deg) between v1 and v2 after projecting both onto the plane whose normal is
    the unit vector ``n`` (the local sheet normal), or None if degenerate."""
    p1 = v1 - np.dot(v1, n) * n
    p2 = v2 - np.dot(v2, n) * n
    n1 = float(np.linalg.norm(p1))
    n2 = float(np.linalg.norm(p2))
    if n1 == 0.0 or n2 == 0.0:
        return None
    c = float(np.dot(p1, p2) / (n1 * n2))
    return float(np.degrees(np.arccos(max(-1.0, min(1.0, c)))))


def _elements(atoms):
    """Per-atom element symbols (resolving atomipy types like 'Ob'->'O', 'Alt'->'Al')."""
    if all(a.get("element") for a in atoms):
        return [a["element"] for a in atoms]
    from .element import element as _assign
    ae = [dict(a) for a in atoms]
    _assign(ae)
    return [a.get("element") for a in ae]


def _corrugation(coords, gap=2.0):
    """Basal-oxygen corrugation Delta_z (Angstrom): cluster the basal-O sheet-normal
    coordinates into sheets (split on gaps > ``gap``), take max-min within each sheet, and
    return (mean, std across sheets, n_sheets)."""
    c = np.sort(np.asarray(coords, dtype=float))
    if c.size < 3:
        return None, None, 0
    groups = np.split(c, np.where(np.diff(c) > gap)[0] + 1)
    dz = [float(g.max() - g.min()) for g in groups if g.size >= 3]
    if not dz:
        return None, None, 0
    return float(np.mean(dz)), float(np.std(dz)), len(dz)


# Octahedral cations for the octahedral-flattening angle psi (element-based).
_OCT_ELEMENTS_DEFAULT = {"Al", "Mg", "Fe", "Li", "Ti", "Mn", "Ni", "Co", "Cr", "Zn", "Ca"}


def _octahedral_flattening(pos, els, H, Hinv, normal, tet_set, oct_elements=None,
                           oct_cutoff=2.5):
    """Mean octahedral flattening angle psi (deg): cos(psi) = t_oct / (2*<M-O>), where
    t_oct is the octahedral-sheet thickness (separation of the two O triangles along the
    sheet normal) and <M-O> the mean octahedral bond length. Ideal psi = 54.74 deg."""
    oct_elements = set(oct_elements) if oct_elements else set(_OCT_ELEMENTS_DEFAULT)
    o_idx = np.array([i for i in range(len(pos)) if els[i] == "O"])
    if o_idx.size == 0:
        return None, None, 0
    o_pos = pos[o_idx]
    psis = []
    for M in range(len(pos)):
        if els[M] not in oct_elements or M in tet_set:
            continue
        d = _mic(o_pos - pos[M], H, Hinv)
        r = np.linalg.norm(d, axis=1)
        sel = np.where(r < oct_cutoff)[0]
        if sel.size < 5 or sel.size > 7:          # ~octahedral (6; allow 5-7 for disorder)
            continue
        v = d[sel]
        mo = float(np.linalg.norm(v, axis=1).mean())
        proj = np.sort(v @ normal)                # O positions along the sheet normal
        k = sel.size // 2
        t_oct = float(proj[-k:].mean() - proj[:k].mean())   # top vs bottom O triangles
        if mo > 0:
            psis.append(float(np.degrees(np.arccos(min(1.0, max(0.0, t_oct / (2 * mo)))))))
    if not psis:
        return None, None, 0
    a = np.asarray(psis)
    return float(a.mean()), float(a.std()), int(a.size)


def tetrahedral_rotation(atoms, Box, *, tet_types=None, tet_elements=None,
                         basal_types=None, apical_types=None,
                         bond_cutoff=1.9, sheet_axis="z", tet_tet_cutoff=3.6,
                         align_tol=0.5, return_details=False):
    """Tetrahedral rotation angle ``alpha`` (deg) for one clay/mica structure.

    Parameters
    ----------
    atoms : list of dict
        atomipy atom records (need 'x','y','z' and 'type'/'element').
    Box : list
        1x3 / 1x6 [a,b,c,alpha,beta,gamma] / 1x9 GROMACS box.
    tet_types, tet_elements : set of str, optional
        What counts as a tetrahedral cation (defaults: Si by element; Si/Sit/Alt/... by
        type). An atom qualifies if its type OR element is in these sets.
    basal_types, apical_types : set of str, optional
        Force-field type names to force the basal / apical oxygen assignment (e.g.
        ``basal_types={'Ob'}``, ``apical_types={'Op'}``). If None (default) the roles are
        inferred from the 'Ob'/'Op' name prefix, then from connectivity (an O bonded to 2
        tetrahedral cations is basal/bridging, to 1 is apical).
    bond_cutoff : float
        Max cation-O bond length (Angstrom) for tetrahedral-O coordination.
    sheet_axis : {'x','y','z'}
        Fallback sheet normal, used only for a tetrahedron whose apical O is not found.
        Normally the local normal is taken per-tetrahedron from the metal->apical vector,
        so the result is independent of how the slab is oriented / tilted (monoclinic).
    tet_tet_cutoff : float
        Sanity cap on the bridged cation-cation distance (Angstrom).
    align_tol : float
        Minimum |cos| between the basal-triplet plane normal and the metal->apical vector
        for a tetrahedron to be accepted (guards against mis-assigned basal/apical O).
    return_details : bool
        Also return the per-bond angle array and per-(tetrahedron,O) records.

    Returns
    -------
    dict with keys: 'alpha' (mean, deg), 'alpha_std', 'alpha_median', 'n_bonds' (basal
    rotation samples), 'n_tet', 'n_tet_used', 'mean_triplet_alignment', 'unit' ('deg');
    plus 'bond_angles' and 'bonds' if return_details. Returns 'alpha'=None with a 'note'
    when no complete tetrahedra are found (not a tetrahedral sheet, or wrong selection).
    """
    tet_types = set(tet_types) if tet_types is not None else set(_TET_TYPES_DEFAULT)
    tet_elements = set(tet_elements) if tet_elements is not None else set(_TET_ELEMENTS_DEFAULT)
    ax = {"x": 0, "y": 1, "z": 2}.get(str(sheet_axis).lower(), 2)
    global_normal = np.eye(3)[ax]     # fallback sheet normal if a tetrahedron's apex is unclear

    if not atoms:
        return {"alpha": None, "n_bonds": 0, "note": "empty structure", "unit": "deg"}

    els = _elements(atoms)
    pos = np.array([[a["x"], a["y"], a["z"]] for a in atoms], dtype=float)
    H = _cell_matrix(Box)
    Hinv = np.linalg.inv(H)

    def _is_tet(i):
        t = atoms[i].get("type") or atoms[i].get("fftype")
        return (t in tet_types) or (els[i] in tet_elements)

    tet_idx = [i for i in range(len(atoms)) if _is_tet(i)]
    o_idx = [i for i in range(len(atoms)) if els[i] == "O"]
    if not tet_idx or not o_idx:
        return {"alpha": None, "n_bonds": 0, "n_tet": len(tet_idx),
                "note": "no tetrahedral cations and/or oxygens found", "unit": "deg"}

    o_arr = np.array(o_idx)
    o_pos = pos[o_arr]

    # Bond graph (minimum-image). o_to_tet[o] = [(tet, vec tet->o)]; tet_to_o[t] = [(o, vec)].
    o_to_tet, tet_to_o = {}, {t: [] for t in tet_idx}
    for t in tet_idx:
        d = _mic(o_pos - pos[t], H, Hinv)
        r = np.linalg.norm(d, axis=1)
        for k in np.where(r < bond_cutoff)[0]:
            o = int(o_arr[k])
            o_to_tet.setdefault(o, []).append((t, d[k]))
            tet_to_o[t].append((o, d[k]))

    def _role(o):
        """'basal' / 'apical' from atom-type prefix (Ob*/Op*/Omg) with a bridging-count
        fallback: an O bonded to 2 tetrahedral cations is basal (bridging), 1 is apical."""
        t = str(atoms[o].get("type") or "")
        if apical_types and t in apical_types:
            return "apical"
        if basal_types and t in basal_types:
            return "basal"
        if t.startswith("Ob"):
            return "basal"
        if t.startswith("Op") or t.startswith("Oa") or t == "Omg":
            return "apical"
        n = len(o_to_tet.get(o, []))
        return "basal" if n >= 2 else ("apical" if n == 1 else "other")

    angles, bonds, aligns = [], [], []
    apical_tilts, tau_angles, basal_coords = [], [], {}
    n_tet_used = 0
    for M in tet_idx:
        nbrs = tet_to_o[M]
        basal = [(o, v) for (o, v) in nbrs if _role(o) == "basal"]
        apic = [(o, v) for (o, v) in nbrs if _role(o) == "apical"]
        if len(basal) < 3 or not apic:            # need a complete tetrahedron (3 basal + apex)
            continue
        # Local sheet normal = metal -> apical direction (points out of the basal face).
        apex = np.mean([v for (_o, v) in apic], axis=0)
        na = float(np.linalg.norm(apex))
        n_M = apex / na if na > 0 else global_normal.copy()
        # Quality: the basal-triplet plane normal should be ~parallel to metal->apical, i.e.
        # the three basal O really form the face opposite the apex (user-suggested check).
        b3 = [v for (_o, v) in basal[:3]]
        tri = np.cross(b3[1] - b3[0], b3[2] - b3[0])
        ntri = float(np.linalg.norm(tri))
        align = float(abs(np.dot(tri / ntri, n_M))) if ntri > 0 else 0.0
        aligns.append(align)
        if align < align_tol:                     # basal/apical assignment inconsistent -> skip
            continue
        n_tet_used += 1
        # --- companion parameters (same tetrahedron) ---
        # apical tilt: deviation of the metal->apical bond from the global sheet normal.
        apical_tilts.append(float(np.degrees(np.arccos(
            min(1.0, abs(float(np.dot(n_M, global_normal))))))))
        # tau: all O-M-O angles (basal + apical), tetrahedral flattening (ideal 109.47 deg).
        ov = [v for (_o, v) in basal] + [v for (_o, v) in apic]
        for ia in range(len(ov)):
            for ib in range(ia + 1, len(ov)):
                nn = float(np.linalg.norm(ov[ia]) * np.linalg.norm(ov[ib]))
                if nn > 0:
                    tau_angles.append(float(np.degrees(np.arccos(
                        max(-1.0, min(1.0, float(np.dot(ov[ia], ov[ib])) / nn))))))
        # basal-O sheet-normal coordinate (dedup by O index) for the corrugation.
        for (o, _v) in basal:
            basal_coords[o] = float(np.dot(pos[o], global_normal))
        for (o, vMO) in basal:
            others = [(t2, v2) for (t2, v2) in o_to_tet[o] if t2 != M]
            if not others:
                continue
            _Mb, vM2O = others[0]                 # the neighbouring tetrahedral cation via this O
            vMM = vMO - vM2O                       # M -> M' anchored on the shared oxygen
            if np.linalg.norm(vMM) > tet_tet_cutoff:
                continue
            ang = _angle_in_plane(vMO, vMM, n_M)   # this tetrahedron's rotation, in its local plane
            if ang is not None:
                angles.append(ang)
                if return_details:
                    bonds.append({"O": o, "tet": M, "angle": ang, "align": align})

    if not angles:
        return {"alpha": None, "n_bonds": 0, "n_tet": len(tet_idx), "n_tet_used": 0,
                "note": "no valid tetrahedra (need 3 basal + 1 apical O per cation) — not a "
                        "tetrahedral sheet, or adjust bond_cutoff/tet_types/align_tol",
                "unit": "deg"}

    arr = np.asarray(angles, dtype=float)
    tau = np.asarray(tau_angles, dtype=float)
    tilt = np.asarray(apical_tilts, dtype=float)
    dz, dz_std, n_sheets = _corrugation(list(basal_coords.values()))
    psi, psi_std, n_oct = _octahedral_flattening(pos, els, H, Hinv, global_normal, set(tet_idx))
    out = {
        # --- tetrahedral rotation (ditrigonal distortion) ---
        "alpha": float(arr.mean()),
        "alpha_std": float(arr.std()),
        "alpha_median": float(np.median(arr)),
        # --- companion distortion parameters ---
        "apical_tilt": float(tilt.mean()) if tilt.size else None,     # T->apical vs sheet normal (deg)
        "apical_tilt_std": float(tilt.std()) if tilt.size else None,
        "tau": float(tau.mean()) if tau.size else None,               # mean O-T-O angle (ideal 109.47)
        "tau_std": float(tau.std()) if tau.size else None,
        "dz_corrugation": dz,                                          # basal-O plane roughness (Angstrom)
        "dz_corrugation_std": dz_std,                                  # spread of corrugation across sheets
        "n_sheets": n_sheets,
        "psi": psi,                                                   # octahedral flattening (ideal 54.74)
        "psi_std": psi_std,
        "n_oct": n_oct,
        # --- bookkeeping ---
        "n_bonds": int(arr.size),
        "n_tet": len(tet_idx),
        "n_tet_used": int(n_tet_used),
        "mean_triplet_alignment": float(np.mean(aligns)) if aligns else None,
        "unit": "deg",
    }
    if return_details:
        out["bond_angles"] = arr
        out["bonds"] = bonds
    return out


def tetrahedral_rotation_files(files, *, tet_types=None, tet_elements=None,
                               bond_cutoff=1.9, sheet_axis="z", tet_tet_cutoff=3.6,
                               on_file=None):
    """Aggregate ``alpha`` over many structure files for good statistics.

    Intended for a set of frames dumped as conf*.gro / conf*.pdb (e.g. an MD trajectory
    exported frame-by-frame): compute alpha per file, then report both the file-to-file
    distribution (how alpha "breathes") and the pooled per-bond distribution.

    Parameters
    ----------
    files : str or list of str
        A glob pattern (e.g. ``"run/conf*.gro"``) or an explicit list of paths.
    on_file : callable, optional
        ``on_file(path, per_file_dict)`` progress callback.

    Returns
    -------
    dict with:
      'alpha_mean'  : mean of the per-file alpha (deg)
      'alpha_std'   : std of the per-file alpha across files (the fluctuation)
      'alpha_sem'   : standard error of the per-file mean
      'n_files'     : files successfully analyzed
      'pooled_mean' : mean over ALL bridging-bond angles from all files
      'pooled_std'  : std of that pooled distribution
      'per_file'    : list of {'file','alpha','n_bonds'}  (alpha=None if a file had none)
      'pooled_angles' : ndarray of every bond angle (for histogramming)
    """
    from .import_conf import auto as _import_auto

    if isinstance(files, str):
        paths = sorted(_glob.glob(files))
    else:
        paths = list(files)
    if not paths:
        return {"alpha_mean": None, "n_files": 0, "per_file": [],
                "note": "no files matched", "unit": "deg"}

    per_file = []
    per_file_alpha = []
    pooled = []
    for p in paths:
        rec = {"file": _os.path.basename(p), "alpha": None, "n_bonds": 0}
        try:
            atoms, Box = _import_auto(p)
            res = tetrahedral_rotation(
                atoms, Box, tet_types=tet_types, tet_elements=tet_elements,
                bond_cutoff=bond_cutoff, sheet_axis=sheet_axis,
                tet_tet_cutoff=tet_tet_cutoff, return_details=True)
            rec["alpha"] = res.get("alpha")
            rec["n_bonds"] = res.get("n_bonds", 0)
            if res.get("alpha") is not None:
                per_file_alpha.append(res["alpha"])
                pooled.append(res["bond_angles"])
        except Exception as e:               # keep going over a bad frame
            rec["error"] = str(e)
        per_file.append(rec)
        if on_file:
            on_file(p, rec)

    if not per_file_alpha:
        return {"alpha_mean": None, "n_files": len(paths), "per_file": per_file,
                "note": "no alpha computed from any file", "unit": "deg"}

    fa = np.asarray(per_file_alpha, dtype=float)
    pooled_arr = np.concatenate(pooled) if pooled else np.array([])
    n = fa.size
    return {
        "alpha_mean": float(fa.mean()),
        "alpha_std": float(fa.std()),
        "alpha_sem": float(fa.std() / np.sqrt(n)) if n > 1 else 0.0,
        "n_files": len(per_file_alpha),
        "n_files_seen": len(paths),
        "pooled_mean": float(pooled_arr.mean()) if pooled_arr.size else None,
        "pooled_std": float(pooled_arr.std()) if pooled_arr.size else None,
        "per_file": per_file,
        "pooled_angles": pooled_arr,
        "unit": "deg",
    }
