"""
Semantic validation for `Topology` (§8). Run on read and before write.
Raises `TopologyError` with an actionable message, or returns the issue list.
"""
from __future__ import annotations

import os
from typing import List, Optional

from . import functional_forms as ff


class TopologyError(ValueError):
    pass


def schema_path() -> str:
    return os.path.join(os.path.dirname(__file__), "schema", "topology.schema.json")


def validate_against_schema(topology) -> List[str]:
    """Optional JSON-Schema validation. Uses ``jsonschema`` if installed
    (import-guarded so the core stays zero-dependency); otherwise no-op."""
    try:
        import json
        import jsonschema  # type: ignore
    except Exception:
        return []
    with open(schema_path(), "r", encoding="utf-8") as f:
        schema = json.load(f)
    errors = sorted(jsonschema.Draft7Validator(schema).iter_errors(topology.to_dict()),
                    key=lambda e: list(e.path))
    return [f"{list(e.path)}: {e.message}" for e in errors]


def validate(topology, *, require_positions: bool = False,
             expected_total_charge: Optional[float] = None,
             charge_tol: float = 1e-3, raise_on_error: bool = True) -> List[str]:
    issues: List[str] = []
    n = topology.n_atoms
    ids = {a.id for a in topology.atoms}

    # 1. atom ids contiguous 0..n-1
    if ids and ids != set(range(n)):
        issues.append(f"atom ids are not a contiguous 0..{n-1} range")

    # 2. atom-type refs resolve
    type_names = {t.name for t in topology.atom_types}
    if type_names:
        for a in topology.atoms:
            if a.type is not None and a.type not in type_names:
                issues.append(f"atom {a.id}: type {a.type!r} not in atom_types")

    # 3. mass / element present
    for a in topology.atoms:
        if a.mass is None and not (a.type and topology.atom_type_map().get(a.type) and
                                   topology.atom_type_map()[a.type].mass is not None):
            issues.append(f"atom {a.id}: no mass (per-site or via type)")
        if require_positions and a.position is None:
            issues.append(f"atom {a.id}: missing position (required for this writer)")

    # 4. bonded index ranges + registered forms
    def check_term(cat, term, idxs):
        for x in idxs:
            if x not in ids:
                issues.append(f"{cat}: atom index {x} out of range")
        if not ff.is_registered(cat, term.form):
            issues.append(f"{cat}: unregistered form {term.form!r}")

    for b in topology.bonds:
        check_term("bond", b, (b.i, b.j))
    for a in topology.angles:
        check_term("angle", a, (a.i, a.j, a.k))
    for d in topology.dihedrals:
        check_term("dihedral", d, (d.i, d.j, d.k, d.l))
    for im in topology.impropers:
        check_term("improper", im, (im.i, im.j, im.k, im.l))

    # 5. box well-formed
    if topology.box is not None:
        m = topology.box.matrix
        if len(m) != 3 or any(len(r) != 3 for r in m):
            issues.append("box matrix must be 3x3")
        elif any(m[d][d] <= 0 for d in range(3)):
            issues.append("box has a non-positive diagonal length")

    # 6. optional charge total
    if expected_total_charge is not None:
        tot = sum((a.charge or 0.0) for a in topology.atoms)
        if abs(tot - expected_total_charge) > charge_tol:
            issues.append(f"total charge {tot:.4f} != expected {expected_total_charge:.4f}")

    if issues and raise_on_error:
        raise TopologyError("Topology validation failed:\n  - " + "\n  - ".join(issues))
    return issues


def lammps_tilt_ok(topology) -> List[str]:
    """LAMMPS restricted-triclinic constraint: |xy| <= lx/2, |xz| <= lx/2,
    |yz| <= ly/2. Returns warnings (does not raise)."""
    warns: List[str] = []
    if topology.box is None:
        return warns
    (xlo, xhi), (ylo, yhi), _, (xy, xz, yz) = topology.box.lammps_bounds()
    lx = xhi - xlo
    ly = yhi - ylo
    if abs(xy) > lx / 2 + 1e-9:
        warns.append(f"LAMMPS tilt |xy|={abs(xy):.3f} > lx/2={lx/2:.3f} (may need cell reduction)")
    if abs(xz) > lx / 2 + 1e-9:
        warns.append(f"LAMMPS tilt |xz|={abs(xz):.3f} > lx/2={lx/2:.3f}")
    if abs(yz) > ly / 2 + 1e-9:
        warns.append(f"LAMMPS tilt |yz|={abs(yz):.3f} > ly/2={ly/2:.3f}")
    return warns
