"""
Type extraction / expansion (§16).

`extract_types` is the per-instance-first inverse of the legacy LAMMPS
"group-by-atom-type-tuple and average" logic: it groups bonded terms by their
*parameters* so per-site-unique terms become per-type-unique entries (≈ one type
per term for MINFF). `expand_types` inlines a type table back onto the terms.
"""
from __future__ import annotations

from typing import Any, Dict, List

_CATEGORIES = {
    "bonds": "bond",
    "angles": "angle",
    "dihedrals": "dihedral",
    "impropers": "improper",
}


def _canonical_key(form: str, params: Dict[str, Any], tol: float):
    """Quantize numeric params to a tolerance grid so float noise doesn't split a
    type; keep non-numeric params (multiplicity n, flags) exact."""
    def q(v):
        if isinstance(v, bool):
            return v
        if isinstance(v, (int, float)):
            return round(v / tol) if tol else float(v)
        return v
    return (form, tuple(sorted((k, q(v)) for k, v in params.items())))


def extract_types(topology, *, tol: float = 1e-9) -> Dict[str, List[dict]]:
    """Mint a minimal type table per category by grouping identical
    (form, params) terms. Sets ``term.type_ref`` and fills
    ``topology.derived_types``. Returns the derived tables."""
    for attr, cat in _CATEGORIES.items():
        terms = getattr(topology, attr)
        table: Dict[Any, dict] = {}
        order: List[Any] = []
        for t in terms:
            key = _canonical_key(t.form, t.params, tol)
            if key not in table:
                table[key] = {"id": len(order) + 1, "form": t.form,
                              "params": dict(t.params)}
                order.append(key)
            t.type_ref = f"{cat}_{table[key]['id']}"
        topology.derived_types[attr] = [table[k] for k in order]
    return topology.derived_types


def expand_types(topology) -> None:
    """Inverse: inline each term's params from its type table (used after reading
    a typed format). No-op for terms that already carry inline params."""
    for attr, cat in _CATEGORIES.items():
        table = topology.derived_types.get(attr) or []
        by_id = {f"{cat}_{e['id']}": e for e in table}
        for t in getattr(topology, attr):
            if (not t.params) and t.type_ref and t.type_ref in by_id:
                t.params = dict(by_id[t.type_ref]["params"])
                if not t.form:
                    t.form = by_id[t.type_ref]["form"]


def extract_atom_types_by_site(topology) -> Dict[int, str]:
    """Force per-site-unique LJ types: mint one atom-type name per distinct
    ``site_label`` (the §3.4 trick for CHARMM/LAMMPS). Returns atom.id ->
    minted type name. Only needed when σ/ε must differ per site — rare for
    MINFF, where nonbonded is per-type and charge is already per-atom."""
    mapping: Dict[int, str] = {}
    for a in topology.atoms:
        mapping[a.id] = a.site_label or a.type or f"T{a.id}"
    return mapping
