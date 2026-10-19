"""
atomipy.import_topology — readers that return a `Topology` (the hub).

  read_json     canonical interchange artifact
  read_itp      GROMACS (+ minimal preprocessor, see topology.gmx_preprocessor)
  read_data     LAMMPS .data
  read_psf      CHARMM .psf (connectivity; params from optional .prm)
  from_atoms_box  bridge atomipy's (atoms, Box) state into a Topology
"""
from __future__ import annotations

import json
from typing import Dict, List, Optional

from .topology import elements as elem
from .topology import functional_forms as ff
from .topology import gmx_preprocessor as gmx
from .topology import typing as ttyping
from .topology import units as U
from .topology import validate as tvalidate
from .topology.model import (
    Angle, Atom, AtomType, Bond, Box, Defaults, Dihedral, Improper, Meta,
    Molecule, Pair, Topology,
)


def _f(x, default=None):
    """Float or default. atomipy charge can be '' -> default."""
    try:
        if x is None or x == "":
            return default
        return float(x)
    except (TypeError, ValueError):
        return default


# ---------------------------------------------------------------------------
# atoms / Box  ->  Topology
# ---------------------------------------------------------------------------
def _box_from_atomipy(box) -> Optional[Box]:
    """atomipy Box (Å): 3 (orthogonal), 6 (a,b,c,α,β,γ), or 9 (Box_dim) -> nm."""
    if box is None:
        return None
    v = [float(x) for x in box]
    if len(v) == 3:
        return Box.from_box_dim([U.angstrom_to_nm(v[0]), U.angstrom_to_nm(v[1]),
                                 U.angstrom_to_nm(v[2])])
    if len(v) == 6:
        return Box.from_cell(U.angstrom_to_nm(v[0]), U.angstrom_to_nm(v[1]),
                             U.angstrom_to_nm(v[2]), v[3], v[4], v[5])
    if len(v) == 9:
        bd = [U.angstrom_to_nm(v[0]), U.angstrom_to_nm(v[1]), U.angstrom_to_nm(v[2]),
              0, 0, U.angstrom_to_nm(v[5]), 0, U.angstrom_to_nm(v[7]), U.angstrom_to_nm(v[8])]
        return Box.from_box_dim(bd)
    raise ValueError(f"Unsupported atomipy Box length {len(v)}")


def from_atoms_box(atoms, box) -> Topology:
    """Bridge atomipy's in-memory state. Positions Å->nm; mass filled from
    element when absent. Bonded parameters are NOT in `atoms` (they live in the
    .itp / FF); read_itp produces a fully-parameterized Topology."""
    top = Topology(meta=Meta(source_format="atoms_box"))
    top.box = _box_from_atomipy(box)
    counters: Dict[str, int] = {}
    for pos, a in enumerate(atoms):
        el = a.get("element") or None
        tname = a.get("type") or a.get("fftype")
        counters[tname] = counters.get(tname, 0) + 1
        mass = _f(a.get("mass")) or elem.mass_of(el)
        position = None
        if a.get("x") is not None:
            position = [U.angstrom_to_nm(_f(a.get("x"), 0.0)),
                        U.angstrom_to_nm(_f(a.get("y"), 0.0)),
                        U.angstrom_to_nm(_f(a.get("z"), 0.0))]
        top.atoms.append(Atom(
            id=pos,
            type=tname,
            site_label=f"{tname}_{counters[tname]}" if tname else None,
            name=a.get("fftype") or tname or "X",
            element=el,
            atomic_number=elem.atomic_number_of(el),
            mass=mass,
            charge=_f(a.get("charge")),
            residue_id=int(a.get("molid", 1) or 1),
            residue_name=a.get("resname", "MIN"),
            molecule_id=int(a.get("molid", 1) or 1),
            position=position,
            extra={k: a[k] for k in ("occupancy", "temp_factor") if k in a},
        ))
    return top


# ---------------------------------------------------------------------------
# JSON
# ---------------------------------------------------------------------------
def read_forcefield_itp(paths, *, defines=None, include_dirs=None):
    """Read a ForceField from GROMACS ffnonbonded.itp / ffbonded.itp (or any
    .itp with [atomtypes]/[bondtypes]/[angletypes])."""
    from .topology import forcefield as _ff
    return _ff.from_itp(paths, defines=defines, include_dirs=include_dirs)


def read_forcefield_json(path, *, variant=None):
    """Read a ForceField from a bundled ffparams/*.json (variant selects a
    nonbonded block, e.g. 'GMINFF_k500')."""
    from .topology import forcefield as _ff
    return _ff.from_json(path, variant=variant)


def apply_forcefield(topology, forcefield, **kw):
    """Fill AtomType LJ/mass/charge and resolve per-instance bond/angle params."""
    from .topology import forcefield as _ff
    return _ff.apply_forcefield(topology, forcefield, **kw)


def read_json(file_path: str, *, validate: bool = True) -> Topology:
    with open(file_path, "r", encoding="utf-8") as f:
        top = Topology.from_dict(json.load(f))
    if validate:
        tvalidate.validate(top, raise_on_error=True)
    return top


def read_xml(file_path: str, *, validate: bool = True) -> Topology:
    from .topology.xml_io import from_xml_string
    with open(file_path, "r", encoding="utf-8") as f:
        top = Topology.from_dict(from_xml_string(f.read()))
    if validate:
        tvalidate.validate(top, raise_on_error=True)
    return top


# ---------------------------------------------------------------------------
# GROMACS .itp / .top
# ---------------------------------------------------------------------------
_DIHEDRAL_IMPROPER_FUNCTS = {2, 4}


def read_itp(file_path: str, *, defines=None, include_dirs=None,
             validate: bool = True) -> Topology:
    lines = gmx.preprocess(file_path, defines=defines, include_dirs=include_dirs)
    secs = gmx.sections(lines)
    top = Topology(meta=Meta(source_format="itp", source_file=file_path))
    # typed parameter tables (used to resolve terms that lack inline params)
    typed = {"bond": {}, "angle": {}}

    for name, body in secs:
        if name == "bondtypes":
            for ln in body:
                t = ln.split()
                if len(t) >= 5:
                    form = ff.form_from_funct("bond", int(t[2]))
                    order = _PARAM_ORDER.get(("bond", form), [])
                    pb = {k: _f(t[3 + i]) for i, k in enumerate(order) if 3 + i < len(t)}
                    typed["bond"][frozenset((t[0], t[1]))] = (
                        form, ff.from_backend("bond", form, pb, "gromacs"))
            continue
        if name == "angletypes":
            for ln in body:
                t = ln.split()
                if len(t) >= 6:
                    form = ff.form_from_funct("angle", int(t[3]))
                    order = _PARAM_ORDER.get(("angle", form), [])
                    pb = {k: _f(t[4 + i]) for i, k in enumerate(order) if 4 + i < len(t)}
                    typed["angle"][(t[1], frozenset((t[0], t[2])))] = (
                        form, ff.from_backend("angle", form, pb, "gromacs"))
            continue
        if name == "defaults":
            t = body[0].split()
            top.defaults = Defaults(
                nb_func=int(t[0]), comb_rule=int(t[1]),
                gen_pairs=(t[2] if len(t) > 2 else "no"),
                fudgeLJ=_f(t[3], 1.0) if len(t) > 3 else 1.0,
                fudgeQQ=_f(t[4], 1.0) if len(t) > 4 else 1.0)
        elif name == "atomtypes":
            for ln in body:
                at = _parse_atomtype(ln)
                if at:
                    top.atom_types.append(at)
        elif name == "moleculetype":
            top.meta.name = body[0].split()[0]
        elif name == "atoms":
            for ln in body:
                t = ln.split()
                if len(t) < 6:
                    continue
                top.atoms.append(Atom(
                    id=int(t[0]) - 1, type=t[1], residue_id=int(t[2]),
                    residue_name=t[3], name=t[4], charge_group=int(t[5]),
                    charge=_f(t[6]) if len(t) > 6 else None,
                    mass=_f(t[7]) if len(t) > 7 else None))
        elif name == "bonds":
            for ln in body:
                _parse_bonded(ln, "bond", 2, top)
        elif name == "angles":
            for ln in body:
                _parse_bonded(ln, "angle", 3, top)
        elif name == "dihedrals":
            for ln in body:
                _parse_bonded(ln, "dihedral", 4, top)
        elif name == "pairs":
            for ln in body:
                t = ln.split()
                if len(t) >= 2:
                    top.pairs.append(Pair(i=int(t[0]) - 1, j=int(t[1]) - 1))

    # resolve terms that lacked inline params from the typed tables (by atom type)
    if typed["bond"] or typed["angle"]:
        tof = {a.id: a.type for a in top.atoms}
        for b in top.bonds:
            if not b.params:
                hit = typed["bond"].get(frozenset((tof.get(b.i), tof.get(b.j))))
                if hit:
                    b.form, b.params = hit[0], dict(hit[1])
        for a in top.angles:
            if not a.params:
                hit = typed["angle"].get((tof.get(a.j), frozenset((tof.get(a.i), tof.get(a.k)))))
                if hit:
                    a.form, a.params = hit[0], dict(hit[1])

    # fill atom masses/elements from atom_types where missing
    tmap = top.atom_type_map()
    for a in top.atoms:
        if a.mass is None and a.type in tmap:
            a.mass = tmap[a.type].mass
        if a.element is None and a.type in tmap:
            a.element = tmap[a.type].element
    if validate:
        tvalidate.validate(top, raise_on_error=True)
    return top


def _parse_atomtype(ln: str) -> Optional[AtomType]:
    t = ln.split()
    if len(t) < 5:
        return None
    # locate ptype column ('A','S','V','D')
    p_idx = next((i for i, tok in enumerate(t) if tok in ("A", "S", "V", "D")), None)
    if p_idx is None or p_idx < 2:
        return None
    name = t[0]
    mass = _f(t[p_idx - 2]); charge = _f(t[p_idx - 1])
    sigma = _f(t[p_idx + 1]) if len(t) > p_idx + 1 else 0.0
    eps = _f(t[p_idx + 2]) if len(t) > p_idx + 2 else 0.0
    an = elem.atomic_number_of(name)
    return AtomType(name=name, atomic_number=an, mass=mass, charge=charge,
                    ptype=t[p_idx], lj={"sigma": sigma or 0.0, "epsilon": eps or 0.0})


_PARAM_ORDER = {
    ("bond", "harmonic"): ["b0", "k"],
    ("angle", "harmonic"): ["theta0", "k"],
    ("angle", "urey-bradley"): ["theta0", "k", "r0", "k_ub"],
    ("dihedral", "periodic"): ["phi0", "k", "n"],
    ("dihedral", "ryckaert-bellemans"): [f"c{i}" for i in range(6)],
    ("improper", "harmonic"): ["xi0", "k"],
    ("improper", "periodic"): ["phi0", "k", "n"],
}


def _parse_bonded(ln: str, category: str, natoms: int, top: Topology):
    t = ln.split()
    if len(t) < natoms + 1:
        return
    idxs = [int(t[i]) - 1 for i in range(natoms)]
    funct = int(t[natoms])
    raw = [_f(x) for x in t[natoms + 1:]]

    cat = category
    if category == "dihedral" and funct in _DIHEDRAL_IMPROPER_FUNCTS:
        cat = "improper"
    form = ff.form_from_funct(cat, funct)
    order = _PARAM_ORDER.get((cat, form), [])
    params_backend = {k: raw[i] for i, k in enumerate(order) if i < len(raw) and raw[i] is not None}
    params = ff.from_backend(cat, form, params_backend, "gromacs") if params_backend else {}
    if "n" in params:
        params["n"] = int(round(params["n"]))

    if cat == "bond":
        top.bonds.append(Bond(i=idxs[0], j=idxs[1], form=form, params=params))
    elif cat == "angle":
        top.angles.append(Angle(i=idxs[0], j=idxs[1], k=idxs[2], form=form, params=params))
    elif cat == "improper":
        top.impropers.append(Improper(i=idxs[0], j=idxs[1], k=idxs[2], l=idxs[3],
                                      form=form, params=params))
    else:
        top.dihedrals.append(Dihedral(i=idxs[0], j=idxs[1], k=idxs[2], l=idxs[3],
                                      form=form, params=params))


# ---------------------------------------------------------------------------
# LAMMPS .data
# ---------------------------------------------------------------------------
_DATA_PARAM_ORDER = _PARAM_ORDER


def read_data(file_path: str, *, units: str = "real", validate: bool = True) -> Topology:
    with open(file_path, "r", encoding="utf-8") as f:
        raw = [l.split("#")[0].rstrip() for l in f]
    sysu = U.get_system(units)
    top = Topology(meta=Meta(source_format="lammps_data", source_file=file_path))

    # locate section headers
    SECTIONS = ("Masses", "Pair Coeffs", "Bond Coeffs", "Angle Coeffs",
                "Dihedral Coeffs", "Improper Coeffs", "Atoms", "Velocities",
                "Bonds", "Angles", "Dihedrals", "Impropers")
    blocks: Dict[str, List[str]] = {}
    cur = None
    box = {"x": (0, 0), "y": (0, 0), "z": (0, 0), "tilt": (0, 0, 0)}
    for ln in raw:
        s = ln.strip()
        if not s:
            continue
        if s in SECTIONS:
            cur = s; blocks[cur] = []; continue
        low = s.lower()
        if low.endswith("xlo xhi"):
            p = s.split(); box["x"] = (float(p[0]), float(p[1])); continue
        if low.endswith("ylo yhi"):
            p = s.split(); box["y"] = (float(p[0]), float(p[1])); continue
        if low.endswith("zlo zhi"):
            p = s.split(); box["z"] = (float(p[0]), float(p[1])); continue
        if low.endswith("xy xz yz"):
            p = s.split(); box["tilt"] = (float(p[0]), float(p[1]), float(p[2])); continue
        if cur:
            blocks[cur].append(s)

    # box (Å -> nm)
    def Lc(x):
        return sysu.to_canonical(x, U.LENGTH)
    lx = Lc(box["x"][1] - box["x"][0]); ly = Lc(box["y"][1] - box["y"][0]); lz = Lc(box["z"][1] - box["z"][0])
    xy, xz, yz = (Lc(t) for t in box["tilt"])
    top.box = Box.from_box_dim([lx, ly, lz, 0, 0, xy, 0, xz, yz])

    masses = {int(l.split()[0]): float(l.split()[1]) for l in blocks.get("Masses", [])}
    pair = {}
    for l in blocks.get("Pair Coeffs", []):
        p = l.split(); pair[int(p[0])] = (float(p[1]), float(p[2]))  # eps, sigma (backend)
    for tid in sorted(masses):
        eps, sig = pair.get(tid, (0.0, 0.0))
        top.atom_types.append(AtomType(
            name=f"type{tid}", mass=masses[tid],
            lj={"epsilon": sysu.to_canonical(eps, U.ENERGY),
                "sigma": sysu.to_canonical(sig, U.LENGTH)}))
    typename = {t: f"type{t}" for t in masses}

    # coeff tables -> derived_types (canonical)
    def parse_coeffs(block, category, form_default):
        out = []
        for l in blocks.get(block, []):
            p = l.split()
            tid = int(p[0]); vals = [float(x) for x in p[1:]]
            form = form_default
            order = _DATA_PARAM_ORDER.get((category, form), [])
            pb = {k: vals[i] for i, k in enumerate(order) if i < len(vals)}
            out.append({"id": tid, "form": form,
                        "params": ff.from_backend(category, form, pb, sysu)})
        return out

    top.derived_types["bonds"] = parse_coeffs("Bond Coeffs", "bond", "harmonic")
    top.derived_types["angles"] = parse_coeffs("Angle Coeffs", "angle", "harmonic")

    for l in blocks.get("Atoms", []):
        p = l.split()
        aid = int(p[0]); mol = int(p[1]); typ = int(p[2]); q = float(p[3])
        x, y, z = (sysu.to_canonical(float(v), U.LENGTH) for v in p[4:7])
        top.atoms.append(Atom(id=aid - 1, type=typename[typ], molecule_id=mol,
                              residue_id=mol, charge=q, mass=masses.get(typ),
                              position=[x, y, z], residue_name="MIN"))
    top.atoms.sort(key=lambda a: a.id)

    for l in blocks.get("Bonds", []):
        p = l.split()
        top.bonds.append(Bond(i=int(p[2]) - 1, j=int(p[3]) - 1,
                              type_ref=f"bond_{int(p[1])}"))
    for l in blocks.get("Angles", []):
        p = l.split()
        top.angles.append(Angle(i=int(p[2]) - 1, j=int(p[3]) - 1, k=int(p[4]) - 1,
                                type_ref=f"angle_{int(p[1])}"))

    ttyping.expand_types(top)   # inline params from coeff tables
    if validate:
        tvalidate.validate(top, raise_on_error=True)
    return top


# ---------------------------------------------------------------------------
# CHARMM .psf (connectivity; params from optional .prm not yet parsed)
# ---------------------------------------------------------------------------
def read_psf(file_path: str, *, prm: Optional[str] = None, validate: bool = True) -> Topology:
    with open(file_path, "r", encoding="utf-8") as f:
        toks = f.read().split("\n")
    top = Topology(meta=Meta(source_format="psf", source_file=file_path))

    def find(tag):
        for i, l in enumerate(toks):
            if tag in l:
                return i, int(l.split()[0])
        return None, 0

    i, natom = find("!NATOM")
    if i is not None:
        for l in toks[i + 1:i + 1 + natom]:
            p = l.split()
            if len(p) < 8:
                continue
            top.atoms.append(Atom(id=int(p[0]) - 1, residue_id=int(p[2]),
                                  residue_name=p[3], name=p[4], type=p[5],
                                  charge=_f(p[6]), mass=_f(p[7])))

    def read_conn(tag, n):
        idx, count = find(tag)
        if idx is None:
            return []
        flat: List[int] = []
        for l in toks[idx + 1:]:
            s = l.split()
            if not s or not s[0].lstrip("-").isdigit():
                break
            flat += [int(x) for x in s]
            if len(flat) >= count * n:
                break
        return [tuple(flat[k:k + n]) for k in range(0, count * n, n)]

    for (a, b) in read_conn("!NBOND", 2):
        top.bonds.append(Bond(i=a - 1, j=b - 1))
    for tpl in read_conn("!NTHETA", 3):
        top.angles.append(Angle(i=tpl[0] - 1, j=tpl[1] - 1, k=tpl[2] - 1))
    for tpl in read_conn("!NPHI", 4):
        top.dihedrals.append(Dihedral(i=tpl[0] - 1, j=tpl[1] - 1, k=tpl[2] - 1, l=tpl[3] - 1))
    for tpl in read_conn("!NIMPHI", 4):
        top.impropers.append(Improper(i=tpl[0] - 1, j=tpl[1] - 1, k=tpl[2] - 1, l=tpl[3] - 1))

    if validate:
        tvalidate.validate(top, raise_on_error=False)  # connectivity-only may lack params
    return top
