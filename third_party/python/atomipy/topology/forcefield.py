"""
Force-field parameter library + apply layer.

A `ForceField` holds typed parameters (LJ/mass/charge per atom-type, plus
bond/angle/dihedral type tables) read from EITHER source:

  - GROMACS `ffnonbonded.itp` + `ffbonded.itp` (via the preprocessor), or
  - the bundled `ffparams/*.json` (which pre-expands the #ifdef variants).

`apply(topology)` then fills `AtomType.lj/mass/charge` and resolves each bonded
term's per-instance `params` by atom-type tuple. So:
  structure-only Topology + ForceField  ->  fully-parameterized Topology
with `.itp` and `.json` interchangeable as the parameter source.
"""
from __future__ import annotations

import json
import os
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Sequence, Tuple

from . import functional_forms as ff
from . import gmx_preprocessor as gmx
from . import elements as elem

# canonical param order (mirrors import_topology)
_ORDER = {
    ("bond", "harmonic"): ["b0", "k"],
    ("angle", "harmonic"): ["theta0", "k"],
    ("angle", "urey-bradley"): ["theta0", "k", "r0", "k_ub"],
    ("dihedral", "periodic"): ["phi0", "k", "n"],
    ("improper", "harmonic"): ["xi0", "k"],
}


def _f(x, default=None):
    try:
        return float(x) if x not in (None, "") else default
    except (TypeError, ValueError):
        return default


@dataclass
class ForceField:
    name: str = ""
    # name -> {mass, charge, sigma, epsilon, ptype, atomic_number, element}
    atomtypes: Dict[str, dict] = field(default_factory=dict)
    bondtypes: Dict[frozenset, Tuple[str, dict]] = field(default_factory=dict)
    angletypes: Dict[tuple, Tuple[str, dict]] = field(default_factory=dict)
    dihedraltypes: Dict[tuple, Tuple[str, dict]] = field(default_factory=dict)

    # --- keys -------------------------------------------------------------
    @staticmethod
    def _bkey(t1, t2):
        return frozenset((t1, t2))

    @staticmethod
    def _akey(t1, t2, t3):
        return (t2, frozenset((t1, t3)))

    # --- apply ------------------------------------------------------------
    def apply(self, topology, *, set_atom_charge: bool = False,
              set_atom_mass: bool = True, overwrite: bool = False):
        """Parameterize *topology* in place: AtomType LJ/mass/charge + per-instance
        bond/angle params resolved by atom type. Returns the topology."""
        from .model import AtomType  # local import to avoid cycle

        used = [a.type for a in topology.atoms if a.type]
        have = {t.name: t for t in topology.atom_types}
        for tname in dict.fromkeys(used):           # preserve order, unique
            at = self.atomtypes.get(tname)
            if not at:
                continue
            if tname not in have or overwrite:
                obj = AtomType(
                    name=tname, mass=at.get("mass"), charge=at.get("charge"),
                    ptype=at.get("ptype", "A"),
                    atomic_number=at.get("atomic_number"),
                    element=at.get("element"),
                    lj={"sigma": at.get("sigma", 0.0), "epsilon": at.get("epsilon", 0.0)})
                if tname in have:
                    topology.atom_types[topology.atom_types.index(have[tname])] = obj
                else:
                    topology.atom_types.append(obj)
                have[tname] = obj

        for a in topology.atoms:
            at = self.atomtypes.get(a.type)
            if not at:
                continue
            if set_atom_mass and (a.mass is None or overwrite):
                a.mass = at.get("mass")
            if set_atom_charge and (a.charge is None or overwrite):
                a.charge = at.get("charge")
            if a.element is None:
                a.element = at.get("element") or (
                    None if at.get("atomic_number") is None else None)

        tof = {a.id: a.type for a in topology.atoms}
        for b in topology.bonds:
            if b.params and not overwrite:
                continue
            hit = self.bondtypes.get(self._bkey(tof.get(b.i), tof.get(b.j)))
            if hit:
                b.form, b.params = hit[0], dict(hit[1])
        for a in topology.angles:
            if a.params and not overwrite:
                continue
            hit = self.angletypes.get(self._akey(tof.get(a.i), tof.get(a.j), tof.get(a.k)))
            if hit:
                a.form, a.params = hit[0], dict(hit[1])
        for d in topology.dihedrals:
            if d.params and not overwrite:
                continue
            hit = self.dihedraltypes.get((tof.get(d.i), tof.get(d.j), tof.get(d.k), tof.get(d.l)))
            if hit:
                d.form, d.params = hit[0], dict(hit[1])
        return topology


# ---------------------------------------------------------------------------
# Readers
# ---------------------------------------------------------------------------
def from_itp(paths: Sequence[str] | str, *, defines=None, include_dirs=None,
             name: str = "") -> ForceField:
    """Read FF params from one or more GROMACS .itp files (e.g. ffnonbonded.itp,
    ffbonded.itp). Sections may be split across files or pulled in via #include."""
    if isinstance(paths, str):
        paths = [paths]
    fff = ForceField(name=name or os.path.basename(paths[0]))
    for p in paths:
        lines = gmx.preprocess(p, defines=defines, include_dirs=include_dirs)
        for sec, body in gmx.sections(lines):
            if sec == "atomtypes":
                for ln in body:
                    _itp_atomtype(ln, fff)
            elif sec == "bondtypes":
                for ln in body:
                    _itp_bonded(ln, fff, "bond", 2)
            elif sec == "angletypes":
                for ln in body:
                    _itp_bonded(ln, fff, "angle", 3)
            elif sec == "dihedraltypes":
                for ln in body:
                    _itp_bonded(ln, fff, "dihedral", 4, allow_two_type=True)
    return fff


def _itp_atomtype(ln, fff: ForceField):
    t = ln.split()
    if len(t) < 5:
        return
    p_idx = next((i for i, tok in enumerate(t) if tok in ("A", "S", "V", "D")), None)
    if p_idx is None or p_idx < 2:
        return
    name = t[0]
    fff.atomtypes[name] = {
        "mass": _f(t[p_idx - 2]), "charge": _f(t[p_idx - 1]), "ptype": t[p_idx],
        "sigma": _f(t[p_idx + 1], 0.0) if len(t) > p_idx + 1 else 0.0,
        "epsilon": _f(t[p_idx + 2], 0.0) if len(t) > p_idx + 2 else 0.0,
        "atomic_number": elem.atomic_number_of(name),
        "element": None,
    }


def _itp_bonded(ln, fff: ForceField, category, natoms, allow_two_type=False):
    t = ln.split()
    if len(t) < natoms + 1:
        # dihedraltypes can use 2 central types
        if allow_two_type and len(t) >= 3:
            natoms = 2
        else:
            return
    try:
        funct = int(t[natoms])
    except ValueError:
        return
    cat = category
    if category == "dihedral" and funct in (2, 4):
        cat = "improper"
    try:
        form = ff.form_from_funct(cat, funct)
    except KeyError:
        return
    order = _ORDER.get((cat, form), [])
    raw = [_f(x) for x in t[natoms + 1:]]
    pb = {k: raw[i] for i, k in enumerate(order) if i < len(raw) and raw[i] is not None}
    params = ff.from_backend(cat, form, pb, "gromacs")
    if "n" in params:
        params["n"] = int(round(params["n"]))
    types = t[:natoms]
    if cat == "bond":
        fff.bondtypes[ForceField._bkey(types[0], types[1])] = (form, params)
    elif cat == "angle":
        fff.angletypes[ForceField._akey(types[0], types[1], types[2])] = (form, params)
    else:
        fff.dihedraltypes[tuple(types)] = (form, params)


def from_json(path: str, *, variant: Optional[str] = None, name: str = "") -> ForceField:
    """Read FF params from a bundled ffparams/*.json. `variant` selects a
    nonbonded block (e.g. 'GMINFF_k500'); bond/angle types are shared top-level."""
    full = path
    if not os.path.isabs(path) and not os.path.exists(path):
        full = os.path.join(os.path.dirname(os.path.dirname(__file__)), "ffparams", path)
    with open(full, "r", encoding="utf-8") as f:
        d = json.load(f)
    fff = ForceField(name=name or os.path.basename(path))

    blocks = d.get("nonbonded_blocks", {})
    atblock = {}
    if variant and variant in blocks:
        atblock = dict(blocks[variant].get("atomtypes", {}))
    elif blocks:
        # default: first block
        atblock = dict(next(iter(blocks.values())).get("atomtypes", {}))
    # common atomtypes (water/ions) always present
    common = d.get("common_atomtypes", {})
    if isinstance(common, dict):
        merged = dict(common)
        merged.update(atblock)
        atblock = merged
    for nm, a in atblock.items():
        fff.atomtypes[nm] = {
            "mass": a.get("mass"), "charge": a.get("charge"),
            "ptype": a.get("ptype", "A"),
            "sigma": a.get("sigma", 0.0), "epsilon": a.get("epsilon", 0.0),
            "atomic_number": elem.atomic_number_of(nm), "element": None,
        }

    for bt in d.get("bondtypes", []):
        t1, t2 = bt["atoms"][0], bt["atoms"][1]
        form = ff.form_from_funct("bond", int(bt.get("func", 1)))
        params = ff.from_backend("bond", form,
                                 {"b0": _f(bt.get("length")), "k": _f(bt.get("k"))}, "gromacs")
        fff.bondtypes[ForceField._bkey(t1, t2)] = (form, params)
    for at in d.get("angletypes", []):
        t1, t2, t3 = at["atoms"][0], at["atoms"][1], at["atoms"][2]
        form = ff.form_from_funct("angle", int(at.get("func", 1)))
        params = ff.from_backend("angle", form,
                                 {"theta0": _f(at.get("theta0")), "k": _f(at.get("k"))}, "gromacs")
        fff.angletypes[ForceField._akey(t1, t2, t3)] = (form, params)
    return fff


def apply_forcefield(topology, forcefield: ForceField, **kw):
    return forcefield.apply(topology, **kw)
