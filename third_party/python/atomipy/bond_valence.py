"""
Bond valence sum utilities.


This module parses the IUCr bond valence parameter table (bvparm2020.cif)
and provides helpers to compute bond valence sums (BVS) and the Global
Instability Index (GII) for inorganic structures.

Quick examples:

1) Compute BVS/GII on in-memory atoms (Box can be 3/6/9 length):
   import atomipy as ap
   atoms, Box = ap.import_gro("structure.gro")
   res, gii = ap.compute_bvs(atoms, Box)
   print("GII", gii, "atom1 BVS", res[0]["bvs"])

2) Override oxidation states explicitly (elements/ox_values are paired):
   res, gii = ap.compute_bvs(
       atoms, Box,
       elements=["Al", "Si", "O", "H"],
       ox_values=[3, 4, -2, 1],
   )

3) One-shot from file with CSV output:
   report = ap.conf2bvs("structure.pdb", csv_path="bvs.csv", top_n=5)
   print(report["gii"], report["gii_no_h"], report["formal_charge"])
"""

from __future__ import annotations

import math
import os
import re
import csv
import copy
from typing import Any, Dict, Iterable, List, Optional, Tuple, TypedDict

from .bond_angle import bond_angle
from .element import element
from .cell_utils import Cell2Box_dim
from .import_conf import pdb as import_pdb, gro as import_gro, xyz as import_xyz

# Location of the IUCr parameter table (kept at the repo root by default)
DEFAULT_PARAM_PATH = os.path.join(
    os.path.dirname(os.path.abspath(__file__)),
    "data",
    "bvparm2020.cif",
)
DEFAULT_SHANNON_PATH = os.path.join(
    os.path.dirname(os.path.abspath(__file__)),
    "data",
    "Revised_Shannon_radii.txt",
)


ParamKey = Tuple[str, int, str, int]
ParamValue = Tuple[float, float, str, str]

class ShannonEntry(TypedDict):
    record: int
    crystal_radius: Optional[float]
    ionic_radius: Optional[float]
    spin_state: Optional[str]

def _merge_oxidation_states(
    oxidation_states: Optional[Dict[str, int]] = None,
    elements: Optional[List[str]] = None,
    ox_values: Optional[List[int]] = None,
) -> Dict[str, int]:
    """
    Build an oxidation state map from either a dict or parallel element/ox lists.
    """
    if oxidation_states and not isinstance(oxidation_states, dict):
        raise ValueError("oxidation_states must be a dict mapping element -> oxidation state.")

    ox_map: Dict[str, int] = dict(oxidation_states or {})
    if elements is not None or ox_values is not None:
        if elements is None or ox_values is None:
            raise ValueError("Both elements and ox_values must be provided together.")
        if len(elements) != len(ox_values):
            raise ValueError("elements and ox_values must have the same length.")
        for el, ox in zip(elements, ox_values):
            if el is not None and ox is not None:
                ox_map[el] = ox
    return ox_map


def load_bv_params(path: Optional[str] = None) -> Dict[ParamKey, ParamValue]:
    """
    Parse bond valence parameters from a CIF table.

    The parser expects the column order used in bvparm2020.cif:
    atom1, ox1, atom2, ox2, R0, B, ref_id, details.
    """
    param_path = path or DEFAULT_PARAM_PATH
    if not os.path.exists(param_path):
        raise FileNotFoundError(f"Bond valence table not found: {param_path}")

    params: Dict[ParamKey, ParamValue] = {}
    in_table = False
    with open(param_path, "r") as fh:
        for raw_line in fh:
            line = raw_line.strip()
            if not line or line.startswith("#"):
                continue

            if "_valence_param_details" in line:
                # The next non-empty lines are the parameter rows.
                in_table = True
                continue

            if not in_table:
                continue

            # End the table if a new loop begins
            if line.startswith("loop_"):
                break

            # Lines are whitespace-delimited with a trailing comment chunk.
            tokens = re.split(r"\s+", line, maxsplit=7)
            if len(tokens) < 7:
                continue

            atom1, ox1, atom2, ox2, r0, bval, ref_id = tokens[:7]
            details = tokens[7] if len(tokens) > 7 else ""

            try:
                key = (atom1, int(ox1), atom2, int(ox2))
                value: ParamValue = (float(r0), float(bval), ref_id, details)
            except ValueError:
                continue

            def should_replace(existing: Optional[ParamValue], candidate: ParamValue) -> bool:
                if existing is None:
                    return True
                # For H-containing pairs, prefer R0 closest to ~0.96 Å (typical O-H)
                if atom1 == "H" or atom2 == "H":
                    target = 0.96
                    r0_existing = existing[0]
                    r0_candidate = candidate[0]
                    return abs(r0_candidate - target) < abs(r0_existing - target)
                # Otherwise keep the first encountered value
                return False

            if should_replace(params.get(key), value):
                params[key] = value
            # Store reverse lookup to simplify later matching
            reverse_key = (atom2, int(ox2), atom1, int(ox1))
            if should_replace(params.get(reverse_key), value):
                params[reverse_key] = value

    if not params:
        raise ValueError(f"No bond valence parameters parsed from {param_path}")

    # Force H–O to use the gas/symmetrical parameter (line 826, ref az)
    ho_value: ParamValue = (0.957, 0.35, "az", "From gas and symmetrical bond length")
    params[("H", 1, "O", -2)] = ho_value
    params[("O", -2, "H", 1)] = ho_value
    return params


def load_shannon_radii(path: Optional[str] = None) -> Dict[Tuple[str, int, int], ShannonEntry]:
    """
    Parse the Revised Shannon radii table into a lookup dictionary.

    Parameters
    ----------
    path : str, optional
        Path to the radii table. Defaults to atomipy/data/Revised_Shannon_radii.txt.

    Returns
    -------
    dict
        Mapping (element, oxidation_state, coordination_number) -> radii data.
    """
    radii_path = path or DEFAULT_SHANNON_PATH
    data: Dict[Tuple[str, int, int], ShannonEntry] = {}
    started = False
    with open(radii_path, "r", encoding="utf-8") as fh:
        for raw in fh:
            line = raw.strip()
            if not line:
                continue
            if line.startswith("Rec.#"):
                started = True
                continue
            if not started:
                continue
            parts = [p.strip() for p in line.split(",")]
            if len(parts) < 9:
                continue
            try:
                rec_no = int(parts[0])
                ion = parts[1]
                ox = int(float(parts[2]))
                coord = int(float(parts[4]))
                spin = parts[5] if parts[5] and parts[5] != "-" else None
                crystal = float(parts[6]) if parts[6] else None
                ionic = float(parts[7]) if parts[7] else None
            except (ValueError, TypeError):
                continue
            key = (ion, ox, coord)
            data[key] = {
                "record": rec_no,
                "crystal_radius": crystal,
                "ionic_radius": ionic,
                "spin_state": spin,
            }
    return data


def bond_valence(distance: float, r0: float, b: float = 0.37) -> float:
    """Compute bond valence using s = exp((R0 - R)/B)."""
    return math.exp((r0 - distance) / b)


def _lookup_param(
    params: Dict[ParamKey, ParamValue],
    el1: str,
    ox1: Optional[int],
    el2: str,
    ox2: Optional[int],
) -> Optional[ParamValue]:
    """Try to find the best matching parameter set."""
    # Helper to normalize unknown oxidation to 9 (used in the table)
    def norm(ox: Optional[int]) -> int:
        return 9 if ox is None else ox

    candidates: List[ParamKey] = []
    candidates.append((el1, norm(ox1), el2, norm(ox2)))
    candidates.append((el2, norm(ox2), el1, norm(ox1)))
    # Try without oxidation states if an exact match is missing
    candidates.append((el1, 9, el2, 9))
    candidates.append((el2, 9, el1, 9))

    for key in candidates:
        if key in params:
            return params[key]

    # Fallback: pick the first entry that matches the element pair regardless of oxidation
    for key, value in params.items():
        if (key[0] == el1 and key[2] == el2) or (key[0] == el2 and key[2] == el1):
            return value
    return None


COMMON_OX = {
    "H": 1,
    "Li": 1,
    "Na": 1,
    "K": 1,
    "Mg": 2,
    "Ca": 2,
    "Al": 3,
    "Si": 4,
    "Ti": 4,
    "Fe": 3,
    "Mn": 2,
    "O": -2,
    "F": -1,
    "Cl": -1,
}


def _infer_oxidation(atom: dict, ox_hint: Optional[int]) -> Optional[int]:
    """Infer oxidation state from explicit hint or charge if close to integer."""
    if ox_hint is not None:
        return ox_hint
    if "ox" in atom and atom["ox"] is not None:
        try:
            return int(atom["ox"])
        except (TypeError, ValueError):
            pass
    charge = atom.get("charge")
    if isinstance(charge, (int, float)) and math.isfinite(charge):
        rounded = round(charge)
        if abs(rounded - charge) < 0.25:
            return rounded
    el = atom.get("element")
    return COMMON_OX.get(el) if isinstance(el, str) else None


def _lookup_shannon(
    radii: Optional[Dict[Tuple[str, int, int], ShannonEntry]],
    element: Optional[str],
    ox: Optional[int],
    cn: Optional[int],
) -> Optional[ShannonEntry]:
    if not radii or element is None or ox is None:
        return None
    ox_int = ox
    cn_int = cn
    if cn_int is not None:
        entry = radii.get((element, ox_int, cn_int))
        if entry:
            return entry
    # Fallback: any coordination number match with same element/ox
    for (el, ox_state, _cn), entry in radii.items():
        if el == element and ox_state == ox_int:
            return entry
    return None


def compute_bvs(
    atoms: List[dict],
    Box: Iterable[float],
    params: Optional[Dict[ParamKey, ParamValue]] = None,
    oxidation_states: Optional[Dict[str, int]] = None,
    elements: Optional[List[str]] = None,
    ox_values: Optional[List[int]] = None,
    shannon_radii: Optional[Dict[Tuple[str, int, int], ShannonEntry]] = None,
    default_b: float = 0.37,
    rmaxH: float = 1.2,
    rmaxM: float = 2.45,
) -> Tuple[List[dict], float]:
    """
    Compute bond valence sums and Global Instability Index.

    Returns:
        results: list of per-atom dicts with BVS info.
        gii: Global Instability Index (sqrt(mean(delta^2))).
    """
    if params is None:
        params = load_bv_params()

    oxidation_states = _merge_oxidation_states(oxidation_states, elements, ox_values)
    if shannon_radii is None:
        shannon_radii = load_shannon_radii()

    atoms = element(atoms)

    # Ensure bonds are present with distances.
    atoms_with_bonds, bond_index, _ = bond_angle(
        atoms, Box, rmaxH=rmaxH, rmaxM=rmaxM
    )

    n_atoms = len(atoms_with_bonds)
    accum = [
        {
            "index": i + 1,
            "element": atoms_with_bonds[i].get("element"),
            "resname": atoms_with_bonds[i].get("resname"),
            "expected_ox": None,
            "bvs": 0.0,
            "bonds": [],
            "cn": atoms_with_bonds[i].get("cn"),
        }
        for i in range(n_atoms)
    ]

    # Normalize bond list
    bonds: List[Tuple[int, int, float]] = []
    if bond_index is None:
        bond_index = []
    # Cast to Any to avoid static type errors when calling tolist() on numpy arrays
    bond_index_any: Any = bond_index
    if hasattr(bond_index_any, "tolist"):
        bond_index = bond_index_any.tolist()
    for entry in bond_index:
        if len(entry) < 3:
            continue
        i, j, dist = int(entry[0]), int(entry[1]), float(entry[2])
        # Process each bond once
        if j < i:
            continue
        bonds.append((i, j, dist))

    # Initial guess for expected oxidation states
    expected: List[Optional[int]] = []
    for atom in atoms_with_bonds:
        el = atom.get("element")
        ox_hint = oxidation_states.get(el) if el else None
        expected.append(_infer_oxidation(atom, ox_hint))

    ALLOWED_STATES = {
        "Fe": [2, 3], "Mn": [2, 3, 4], "Ti": [2, 3, 4],
        "V": [2, 3, 4, 5], "Cr": [2, 3, 6], "Cu": [1, 2],
        "Ni": [2, 3], "Co": [2, 3], "Sn": [2, 4], "Pb": [2, 4],
        "O": [-2], "F": [-1], "Cl": [-1], "S": [-2, 4, 6],
        "Al": [3], "Si": [4], "Mg": [2], "Ca": [2], "Na": [1],
        "K": [1], "Li": [1], "H": [1, -1],
    }

    # Build a quick adjacency lookup for the refinement pass
    atom_bonds = [[] for _ in range(n_atoms)]
    for i, j, dist in bonds:
        if i >= n_atoms or j >= n_atoms:
            continue
        atom_bonds[i].append((j, dist))
        atom_bonds[j].append((i, dist))

    # Refine oxidation states based on BVS
    for i in range(n_atoms):
        atom_i = atoms_with_bonds[i]
        el_i = atom_i.get("element") or atom_i.get("type") or "X"
        ox_hint = oxidation_states.get(el_i) if el_i else None

        # Do not overwrite if the user explicitly provided the state
        if ox_hint is not None or "ox" in atom_i:
            continue

        candidates = ALLOWED_STATES.get(el_i)
        if not candidates:
            c_set = set()
            for (el1, ox1, el2, ox2) in params.keys():
                if el1 == el_i and ox1 != 9: c_set.add(ox1)
                if el2 == el_i and ox2 != 9: c_set.add(ox2)
            if c_set:
                candidates = sorted(list(c_set))
            else:
                candidates = [expected[i]]

        best_ox = expected[i]
        best_delta = float('inf')

        for ox_i in candidates:
            bvs_i = 0.0
            for j, dist in atom_bonds[i]:
                atom_j = atoms_with_bonds[j]
                el_j = atom_j.get("element") or atom_j.get("type") or "X"
                ox_j = expected[j]
                
                param = _lookup_param(params, el_i, ox_i, el_j, ox_j)
                if param:
                    bvs_i += bond_valence(dist, param[0], param[1])
                else:
                    # fallback if parameter doesn't exist
                    bvs_i += bond_valence(dist, 0.0, default_b)

            delta = abs(bvs_i - abs(ox_i)) if ox_i is not None else float('inf')
            if delta < best_delta:
                best_delta = delta
                best_ox = ox_i

        expected[i] = best_ox

    # Accumulate bond valences
    for i, j, dist in bonds:
        if i >= n_atoms or j >= n_atoms:
            continue
        atom_i = atoms_with_bonds[i]
        atom_j = atoms_with_bonds[j]
        el1 = atom_i.get("element") or atom_i.get("type") or "X"
        el2 = atom_j.get("element") or atom_j.get("type") or "X"
        ox1 = expected[i]
        ox2 = expected[j]

        param = _lookup_param(params, el1, ox1, el2, ox2)
        if param:
            r0, bval, _, _ = param
        else:
            r0, bval = None, default_b

        if r0 is None:
            continue

        sval = bond_valence(dist, r0, bval)
        accum[i]["bvs"] += sval
        accum[j]["bvs"] += sval
        accum[i]["bonds"].append((j + 1, dist, sval))
        accum[j]["bonds"].append((i + 1, dist, sval))

    # Attach expected oxidation and deltas
    deltas = []
    for idx, res in enumerate(accum):
        res["expected_ox"] = expected[idx]
        exp_val = expected[idx]
        if exp_val is not None:
            target = abs(exp_val)  # BVS should match the magnitude of oxidation
            delta = res["bvs"] - target
            res["delta"] = delta
            deltas.append(delta * delta)
        else:
            res["delta"] = None
        # Attach Shannon radii if available
        cn_raw = res.get("cn")
        if isinstance(cn_raw, (int, float, str)):
            cn_int = int(cn_raw)
        else:
            cn_int = len(res["bonds"])
        res["cn"] = cn_int
        element_val = res.get("element")
        ox_val = expected[idx]
        if isinstance(element_val, str) and ox_val is not None:
            sh_entry = _lookup_shannon(shannon_radii, element_val, ox_val, cn_int)
        else:
            sh_entry = None

        if sh_entry:
            res["shannon_ionic_radius"] = sh_entry.get("ionic_radius")
            res["shannon_crystal_radius"] = sh_entry.get("crystal_radius")
            res["shannon_spin_state"] = sh_entry.get("spin_state")
            res["shannon_record"] = sh_entry.get("record")

    gii = math.sqrt(sum(deltas) / len(deltas)) if deltas else 0.0
    return accum, gii


def summarize_bvs(results: List[dict]) -> dict:
    """
    Build a compact summary useful for logs.
    """
    by_element: Dict[str, List[float]] = {}
    for res in results:
        el = res.get("element") or "X"
        by_element.setdefault(el, []).append(res.get("bvs", 0.0))

    summary = {
        "per_element_avg": {
            el: (sum(vals) / len(vals) if vals else 0.0)
            for el, vals in by_element.items()
        },
        "worst_atoms": sorted(
            [r for r in results if r.get("delta") is not None],
            key=lambda r: abs(r["delta"]),
            reverse=True,
        )[:10],
    }
    return summary


def global_instability_index(results: List[dict]) -> float:
    """
    Calculate Global Instability Index (GII) from computed BVS results.
    """
    deltas = [
        res["delta"] ** 2
        for res in results
        if res.get("delta") is not None
    ]
    return math.sqrt(sum(deltas) / len(deltas)) if deltas else 0.0


def _write_bvs_csv(results: List[dict], csv_path: str) -> None:
    fieldnames = [
        "index",
        "element",
        "resname",
        "bvs",
        "expected_ox",
        "delta",
        "n_bonds",
        "bond_contributions",
    ]
    with open(csv_path, "w", newline="") as fh:
        writer = csv.DictWriter(fh, fieldnames=fieldnames)
        writer.writeheader()
        for row in results:
            bond_contribs = " ".join(f"{val:.3f}" for _, _, val in row.get("bonds", []))
            writer.writerow(
                {
                    "index": row.get("index"),
                    "element": row.get("element"),
                    "resname": row.get("resname"),
                    "bvs": row.get("bvs"),
                    "expected_ox": row.get("expected_ox"),
                    "delta": row.get("delta"),
                    "n_bonds": len(row.get("bonds", [])),
                    "bond_contributions": bond_contribs,
                }
            )


def analyze_bvs(
    atoms: List[dict],
    Box: Iterable[float],
    params: Optional[Dict[ParamKey, ParamValue]] = None,
    oxidation_states: Optional[Dict[str, int]] = None,
    elements: Optional[List[str]] = None,
    ox_values: Optional[List[int]] = None,
    shannon_radii: Optional[Dict[Tuple[str, int, int], ShannonEntry]] = None,
    csv_path: Optional[str] = None,
    top_n: int = 10,
) -> dict:
    """
    High-level helper to compute BVS, GII, GII without hydrogens, and optional CSV output.
    """
    results, gii = compute_bvs(
        atoms,
        Box,
        params=params,
        oxidation_states=oxidation_states,
        elements=elements,
        ox_values=ox_values,
        shannon_radii=shannon_radii,
    )
    summary = summarize_bvs(results)
    gii_no_h = global_instability_index([r for r in results if (r.get("element") or "").upper() != "H"])
    formal_charge = sum(
        ox for ox in (r.get("expected_ox") for r in results) if ox is not None
    )

    if csv_path:
        _write_bvs_csv(results, csv_path)

    return {
        "results": results,
        "summary": summary,
        "gii": gii,
        "gii_no_h": gii_no_h,
        "formal_charge": formal_charge,
        "top_worst": summary.get("worst_atoms", [])[:top_n],
    }


def conf2bvs(
    file_path: str,
    params_path: Optional[str] = None,
    csv_path: Optional[str] = None,
    top_n: int = 10,
    oxidation_states: Optional[Dict[str, int]] = None,
    elements: Optional[List[str]] = None,
    ox_values: Optional[List[int]] = None,
    shannon_radii: Optional[Dict[Tuple[str, int, int], ShannonEntry]] = None,
) -> dict:
    """
    Convenience wrapper to import a structure file (pdb/gro/xyz) and run BVS analysis.
    """
    ext = os.path.splitext(file_path)[1].lower()
    if ext == ".pdb":
        atoms, Cell = import_pdb(file_path)
        if Cell is None:
            raise ValueError("PDB is missing CRYST1 dimensions required for BVS.")
        Box = Cell2Box_dim(Cell)
    elif ext == ".gro":
        atoms, Box = import_gro(file_path)
    elif ext == ".xyz":
        atoms, Cell = import_xyz(file_path)
        if Cell is None:
            raise ValueError("XYZ missing box dimensions in comment line.")
        Box = Cell2Box_dim(Cell)
    else:
        raise ValueError(f"Unsupported file type for BVS analysis: {ext}")

    if Box is None:
        raise ValueError("Structure is missing box/cell dimensions required for BVS.")

    params = load_bv_params(params_path) if params_path else load_bv_params()
    return analyze_bvs(
        atoms,
        Box,
        params=params,
        oxidation_states=oxidation_states,
        elements=elements,
        ox_values=ox_values,
        shannon_radii=shannon_radii,
        csv_path=csv_path,
        top_n=top_n,
    )


def add_hydrogens_bvs(
    atoms: List[dict],
    Box: Iterable[float],
    delta_threshold: float = -0.5,
    max_additions: int = 10,
    bond_length: float = 0.96,
    coordination: int = 2,
) -> List[dict]:
    """
    Automatically add hydrogen atoms to undersaturated oxygen sites based on BVS.

    Parameters
    ----------
    atoms : list of dict
        Structure atoms.
    Box : list
        Box dimensions.
    delta_threshold : float
        Minimum BVS deficit (BVS - expected_ox) to trigger protonation.
    max_additions : int
        Maximum number of hydrogens to add globally.
    bond_length : float
        O-H bond length.
    coordination : int
        Target coordination number for add_H_atom. Default is 2.

    Returns
    -------
    list of dict
        Updated atoms list.
    """
    if max_additions <= 0:
        return atoms

    # Perform BVS analysis
    report = analyze_bvs(atoms, Box)
    results = report.get("results", [])

    candidates = []
    # Identify target oxygens
    for row in results:
        el = (row.get("element") or "").upper()
        delta = row.get("delta")

        # We target Oxygen with significant deficit
        if el != "O" or delta is None or delta >= delta_threshold:
            continue

        idx0 = int(row.get("index", 1)) - 1

        # Check if it already has an H neighbor
        has_h = False
        for neighbor_idx, _, _ in row.get("bonds", []):
            neighbor = atoms[neighbor_idx - 1]
            if (neighbor.get("element") or "").upper() == "H":
                has_h = True
                break

        if not has_h:
            candidates.append((delta, idx0))

    # Sort by most undersaturated first
    candidates.sort(key=lambda x: x[0])

    # Process top candidates
    target_indices = [idx0 for _, idx0 in candidates[:max_additions]]
    if not target_indices:
        return atoms

    from .build import add_H_atom

    current_atoms = copy.deepcopy(atoms)

    for idx0 in target_indices:
        # Use a temporary unique type so add_H_atom only targets this specific oxygen
        original_type = current_atoms[idx0].get("type", "O")
        temp_type = f"__BVS_TARGET_{idx0}__"
        current_atoms[idx0]["type"] = temp_type

        # Add the hydrogen
        current_atoms = add_H_atom(
            current_atoms,
            Box,
            target_type=temp_type,
            h_type="H",
            bond_length=bond_length,
            coordination=coordination,
            max_h_per_atom=1
        )

        # Find the target atom again to restore its type
        for atom in current_atoms:
            if atom.get("type") == temp_type:
                atom["type"] = original_type
                break

    return current_atoms
