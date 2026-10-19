"""
atomipy.write_topology — writers that consume a `Topology` (the hub).

  write_json   canonical interchange artifact
  write_itp    GROMACS (inline per-instance params — the MINFF-native path)
  write_data   LAMMPS .data (unique type per unique term via typing.extract_types)
  write_psf    CHARMM .psf (+ generated .prm)
  to_atoms_box bridge back to atomipy's (atoms, Box) state
"""
from __future__ import annotations

import json
import os
from typing import Dict, List, Optional

from .topology import functional_forms as ff
from .topology import typing as ttyping
from .topology import units as U
from .topology import validate as tvalidate
from .topology.model import Topology
from ._provenance import provenance_string

# Parameter emission order per (category, form) for fixed-column file formats.
_PARAM_ORDER = {
    ("bond", "harmonic"): ["b0", "k"],
    ("angle", "harmonic"): ["theta0", "k"],
    ("angle", "urey-bradley"): ["theta0", "k", "r0", "k_ub"],
    ("dihedral", "periodic"): ["phi0", "k", "n"],
    ("dihedral", "ryckaert-bellemans"): [f"c{i}" for i in range(6)],
    ("dihedral", "fourier"): ["phi0", "k", "n"],
    ("improper", "harmonic"): ["xi0", "k"],
    ("improper", "periodic"): ["phi0", "k", "n"],
}


def _ordered(category: str, form: str, params: Dict[str, float]) -> List[float]:
    order = _PARAM_ORDER.get((category, form)) or list(params.keys())
    return [params[k] for k in order if k in params]


# ---------------------------------------------------------------------------
# JSON (canonical)
# ---------------------------------------------------------------------------
def _json_default(o):
    """Coerce numpy scalars/arrays (int32, float64, ndarray, …) to native types
    so json.dump doesn't choke on values coming from numpy-backed atom data."""
    if hasattr(o, "item"):          # numpy scalar (int32/float64/bool_)
        return o.item()
    if hasattr(o, "tolist"):        # numpy ndarray
        return o.tolist()
    raise TypeError(f"Object of type {o.__class__.__name__} is not JSON serializable")


def write_json(topology: Topology, file_path: str, *, indent: int = 2,
               validate: bool = True) -> str:
    if validate:
        tvalidate.validate(topology, raise_on_error=True)
    data = topology.to_dict()
    data["_generator"] = provenance_string()
    with open(file_path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=indent, default=_json_default)
    return file_path


def write_xml(topology: Topology, file_path: str, *, validate: bool = True) -> str:
    """Optional OpenFF-flavoured XML artifact (explicit per-instance, NOT SMIRNOFF)."""
    from .topology.xml_io import to_xml_string
    if validate:
        tvalidate.validate(topology, raise_on_error=True)
    data = topology.to_dict()
    data["_generator"] = provenance_string()
    with open(file_path, "w", encoding="utf-8") as f:
        f.write(to_xml_string(data))
    return file_path


# ---------------------------------------------------------------------------
# atoms / Box adapter
# ---------------------------------------------------------------------------
def to_atoms_box(topology: Topology):
    """Bridge back to atomipy's (atoms, Box). Positions nm -> Å; Box -> 1x6 cell
    (Å + degrees), matching `import_auto`."""
    atoms = []
    for a in topology.atoms:
        d = {
            "index": a.id + 1,
            "type": a.type,
            "fftype": a.type,
            "element": a.element,
            "resname": a.residue_name,
            "molid": a.molecule_id,
            "charge": a.charge if a.charge is not None else "",
            "mass": a.mass,
        }
        if a.position is not None:
            d["x"], d["y"], d["z"] = (U.nm_to_angstrom(p) for p in a.position)
        d.update(a.extra)
        atoms.append(d)
    box = topology.box.cell_params() if topology.box is not None else None
    if box is not None:
        box = [U.nm_to_angstrom(box[0]), U.nm_to_angstrom(box[1]),
               U.nm_to_angstrom(box[2]), box[3], box[4], box[5]]
    return atoms, box


# ---------------------------------------------------------------------------
# GROMACS .itp / .top  (inline per-instance params — MINFF-native)
# ---------------------------------------------------------------------------
def write_itp(topology: Topology, file_path: str, *, molecule_name: Optional[str] = None,
              nrexcl: int = 1, validate: bool = True, include_defaults: bool = True,
              explicit_bonds: bool = True, explicit_angles: bool = True) -> str:
    if validate:
        tvalidate.validate(topology, raise_on_error=True)
    g = "gromacs"
    name = molecule_name or topology.meta.name or "MOL"
    lines: List[str] = []
    ap = lines.append

    ap(f"; {provenance_string()} — write_topology GROMACS .itp (inline per-instance params)")
    
    if include_defaults:
        d = topology.defaults
        ap("\n[ defaults ]")
        ap("; nbfunc  comb-rule  gen-pairs  fudgeLJ  fudgeQQ")
        ap(f"{d.nb_func:<8d} {d.comb_rule:<10d} {d.gen_pairs:<10s} {d.fudgeLJ:<8g} {d.fudgeQQ:g}")

        if topology.atom_types:
            ap("\n[ atomtypes ]")
            ap(";name  at.num   mass     charge  ptype     sigma         epsilon")
            for t in topology.atom_types:
                sig = t.lj.get("sigma", 0.0)
                eps = t.lj.get("epsilon", 0.0)
                an = t.atomic_number if t.atomic_number is not None else 0
                ap(f"{t.name:<6s} {an:>4d} {(t.mass or 0.0):>10.5f} {(t.charge or 0.0):>9.5f} "
                   f"{t.ptype:>3s} {sig:>14.6e} {eps:>14.6e}")

    ap("\n[ moleculetype ]")
    ap("; name   nrexcl")
    ap(f"{name:<8s} {nrexcl}")

    ap("\n[ atoms ]")
    ap(";   nr   type  resnr  residue  atom   cgnr     charge        mass")
    for a in topology.atoms:
        resn = a.residue_name or "MOL"
        aname = a.name or a.type or "X"
        ap(f"{a.id + 1:>6d} {(a.type or 'X'):>6s} {a.residue_id:>6d} {resn:>8s} "
           f"{aname:>6s} {a.charge_group:>6d} "
           f"{(a.charge if a.charge is not None else 0.0):>12.6f} {(a.mass or 0.0):>11.5f}")

    def emit_bonded(header, terms, category, idx_attr, explicit=True):
        if not terms:
            return
        ap(f"\n[ {header} ]")
        for t in terms:
            idxs = [getattr(t, x) + 1 for x in idx_attr]
            funct = ff.gromacs_funct(category, t.form)
            p = ff.to_backend(category, t.form, t.params, g)
            vals = _ordered(category, t.form, p)
            idx_s = " ".join(f"{x:>6d}" for x in idxs)
            
            if explicit:
                val_s = " ".join(f"{v:>14.6e}" for v in vals)
                ap(f"{idx_s} {funct:>4d} {val_s}")
            else:
                val_s = " ".join(f"{v:>14.6e}" for v in vals)
                ap(f"{idx_s} {funct:>4d} ; {val_s}")

    emit_bonded("bonds", topology.bonds, "bond", ("i", "j"), explicit=explicit_bonds)
    if topology.pairs:
        ap("\n[ pairs ]")
        for pr in topology.pairs:
            ap(f"{pr.i + 1:>6d} {pr.j + 1:>6d} {1:>4d}")
    emit_bonded("angles", topology.angles, "angle", ("i", "j", "k"), explicit=explicit_angles)
    emit_bonded("dihedrals", topology.dihedrals, "dihedral", ("i", "j", "k", "l"))
    emit_bonded("dihedrals", topology.impropers, "improper", ("i", "j", "k", "l"))

    if topology.exclusions:
        ap("\n[ exclusions ]")
        for e in topology.exclusions:
            ap(f"{e.i + 1} " + " ".join(str(x + 1) for x in e.excluded))

    with open(file_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")
    return file_path


def write_top(topology: Topology, file_path: str, *, molecule_name: Optional[str] = None,
              split_system: bool = False, water_model: str = "spce",
              nrexcl: int = 1, validate: bool = True) -> str:
    """
    Write a complete GROMACS system topology (.top) file.
    
    If split_system=True, it will partition the atoms into mineral vs solvent/ions,
    write the mineral out as a separate _mineral.itp file, and include it in the .top.
    Otherwise, it writes all [ moleculetype ] details inline.
    """
    if validate:
        tvalidate.validate(topology, raise_on_error=True)
    
    name = molecule_name or topology.meta.name or "System"
    
    # Simple heuristic to identify solvent/ions for splitting
    def is_solvent(a):
        res = (a.residue_name or "").upper()
        typ = (a.type or "").upper()
        return res in ('SOL', 'WAT', 'HOH', 'TIP3', 'OPC', 'SPC', 'SPCE', 'ION', 
                       'NA', 'CL', 'K', 'CA', 'MG') or typ in ('OW', 'HW', 'HW1', 'HW2', 'MW')

    has_solvent = any(is_solvent(a) for a in topology.atoms)
    
    if split_system and has_solvent:
        # Split atoms into mineral and solvent
        mineral_atoms = [a for a in topology.atoms if not is_solvent(a)]
        solvent_atoms = [a for a in topology.atoms if is_solvent(a)]
        
        # We need a copy of the topology for the mineral.itp
        import copy
        min_top = copy.deepcopy(topology)
        min_top.atoms = mineral_atoms
        # Filter bonds/angles to only those within the mineral
        min_ids = {a.id for a in mineral_atoms}
        min_top.bonds = [b for b in min_top.bonds if b.i in min_ids and b.j in min_ids]
        min_top.angles = [a for a in min_top.angles if a.i in min_ids and a.j in min_ids and a.k in min_ids]
        min_top.dihedrals = [d for d in min_top.dihedrals if d.i in min_ids and d.j in min_ids and d.k in min_ids and d.l in min_ids]
        min_top.impropers = [i for i in min_top.impropers if i.i in min_ids and i.j in min_ids and i.k in min_ids and i.l in min_ids]
        
        # Write mineral.itp (without defaults!)
        dir_name = os.path.dirname(file_path)
        base_name = os.path.basename(file_path)
        mineral_itp_name = base_name.replace('.top', '_mineral.itp') if file_path.endswith('.top') else base_name + '_mineral.itp'
        mineral_itp_path = os.path.join(dir_name, mineral_itp_name) if dir_name else mineral_itp_name
        
        write_itp(min_top, mineral_itp_path, molecule_name=molecule_name, nrexcl=nrexcl, validate=False, include_defaults=False)
        
        lines = []
        ap = lines.append
        ap(f"; {provenance_string()} — write_topology GROMACS .top (split_system)")
        
        d = topology.defaults
        ap("\n[ defaults ]")
        ap("; nbfunc  comb-rule  gen-pairs  fudgeLJ  fudgeQQ")
        ap(f"{d.nb_func:<8d} {d.comb_rule:<10d} {d.gen_pairs:<10s} {d.fudgeLJ:<8g} {d.fudgeQQ:g}")

        if topology.atom_types:
            ap("\n[ atomtypes ]")
            for t in topology.atom_types:
                sig = t.lj.get("sigma", 0.0)
                eps = t.lj.get("epsilon", 0.0)
                an = t.atomic_number if t.atomic_number is not None else 0
                ap(f"{t.name:<6s} {an:>4d} {(t.mass or 0.0):>10.5f} {(t.charge or 0.0):>9.5f} "
                   f"{t.ptype:>3s} {sig:>14.6e} {eps:>14.6e}")

        ap(f"\n#include \"{mineral_itp_name}\"")
        
        # Count solvent molecules
        water_count = 0
        ion_counts = {}
        for a in solvent_atoms:
            res = (a.residue_name or "").upper()
            if res in ('SOL', 'WAT', 'HOH', 'TIP3', 'OPC', 'SPC', 'SPCE'):
                if (a.type or "").upper() in ("OW", "O"):
                    water_count += 1
            elif res in ('ION', 'NA', 'CL', 'K', 'CA', 'MG'):
                ion_name = res if res != 'ION' else a.element or a.type
                ion_counts[ion_name] = ion_counts.get(ion_name, 0) + 1
        
        if water_count > 0:
            ap(f"#include \"{water_model}.itp\"")
        if ion_counts:
            ap("#include \"ions.itp\"")
            
        ap("\n[ system ]")
        ap(f"{name}")
        ap("\n[ molecules ]")
        ap(f"{molecule_name or 'MOL'} 1")
        if water_count > 0:
            ap(f"SOL {water_count}")
        for ion, count in ion_counts.items():
            ap(f"{ion} {count}")
            
        with open(file_path, "w", encoding="utf-8") as f:
            f.write("\n".join(lines) + "\n")
        return file_path
        
    else:
        # Non-split system: just write defaults, then the moleculetype inline, then system
        # We can reuse write_itp logic for the molecule body, then append system.
        lines = []
        ap = lines.append
        ap(f"; {provenance_string()} — write_topology GROMACS .top")
        
        d = topology.defaults
        ap("\n[ defaults ]")
        ap("; nbfunc  comb-rule  gen-pairs  fudgeLJ  fudgeQQ")
        ap(f"{d.nb_func:<8d} {d.comb_rule:<10d} {d.gen_pairs:<10s} {d.fudgeLJ:<8g} {d.fudgeQQ:g}")

        if topology.atom_types:
            ap("\n[ atomtypes ]")
            for t in topology.atom_types:
                sig = t.lj.get("sigma", 0.0)
                eps = t.lj.get("epsilon", 0.0)
                an = t.atomic_number if t.atomic_number is not None else 0
                ap(f"{t.name:<6s} {an:>4d} {(t.mass or 0.0):>10.5f} {(t.charge or 0.0):>9.5f} "
                   f"{t.ptype:>3s} {sig:>14.6e} {eps:>14.6e}")
        
        # Write the molecule body using a temporary string or by duplicating logic
        # For simplicity we duplicate the moleculetype emission logic here, since it's short.
        ap("\n[ moleculetype ]")
        ap(f"{name:<8s} {nrexcl}")

        ap("\n[ atoms ]")
        for a in topology.atoms:
            resn = a.residue_name or "MOL"
            aname = a.name or a.type or "X"
            ap(f"{a.id + 1:>6d} {(a.type or 'X'):>6s} {a.residue_id:>6d} {resn:>8s} "
               f"{aname:>6s} {a.charge_group:>6d} "
               f"{(a.charge if a.charge is not None else 0.0):>12.6f} {(a.mass or 0.0):>11.5f}")

        def emit_bonded(header, terms, category, idx_attr):
            if not terms:
                return
            ap(f"\n[ {header} ]")
            for t in terms:
                idxs = [getattr(t, x) + 1 for x in idx_attr]
                funct = ff.gromacs_funct(category, t.form)
                p = ff.to_backend(category, t.form, t.params, "gromacs")
                vals = _ordered(category, t.form, p)
                idx_s = " ".join(f"{x:>6d}" for x in idxs)
                val_s = " ".join(f"{v:>14.6e}" for v in vals)
                ap(f"{idx_s} {funct:>4d} {val_s}")

        emit_bonded("bonds", topology.bonds, "bond", ("i", "j"))
        emit_bonded("angles", topology.angles, "angle", ("i", "j", "k"))
        emit_bonded("dihedrals", topology.dihedrals, "dihedral", ("i", "j", "k", "l"))
        emit_bonded("dihedrals", topology.impropers, "improper", ("i", "j", "k", "l"))

        ap("\n[ system ]")
        ap(f"{name}")
        ap("\n[ molecules ]")
        ap(f"{name} 1")
        
        with open(file_path, "w", encoding="utf-8") as f:
            f.write("\n".join(lines) + "\n")
        return file_path


# ---------------------------------------------------------------------------
# LAMMPS .data  (per-term unique typing via extract_types)
# ---------------------------------------------------------------------------
def write_data(topology: Topology, file_path: str, *, atom_style: str = "full",
               units: str = "real", emit_input_snippet: Optional[str] = None,
               tol: float = 1e-9, validate: bool = True) -> str:
    if validate:
        tvalidate.validate(topology, require_positions=True, raise_on_error=True)
    if topology.box is None:
        raise tvalidate.TopologyError(
            "write_data requires topology.box (LAMMPS .data needs a simulation box; "
            "set it from the companion .gro / atoms Box).")
    sysu = U.get_system(units)

    # Per-term type tables (unique per unique term — the key inversion vs legacy lmp()).
    ttyping.extract_types(topology, tol=tol)

    # Atom types -> LAMMPS integer type ids (ordered by first appearance).
    atype_order: List[str] = []
    for a in topology.atoms:
        key = a.type or "X"
        if key not in atype_order:
            atype_order.append(key)
    atype_id = {name: i + 1 for i, name in enumerate(atype_order)}
    tmap = topology.atom_type_map()

    def L(x):  # nm -> length unit
        return sysu.from_canonical(x, U.LENGTH)

    bt = topology.derived_types.get("bonds", [])
    at = topology.derived_types.get("angles", [])
    dt = topology.derived_types.get("dihedrals", [])
    it = topology.derived_types.get("impropers", [])

    out: List[str] = []
    w = out.append
    w(f"{provenance_string()} — write_topology LAMMPS .data (per-instance-unique types)\n")
    w(f"{len(topology.atoms)} atoms")
    if topology.bonds: w(f"{len(topology.bonds)} bonds")
    if topology.angles: w(f"{len(topology.angles)} angles")
    if topology.dihedrals: w(f"{len(topology.dihedrals)} dihedrals")
    if topology.impropers: w(f"{len(topology.impropers)} impropers")
    w("")
    w(f"{len(atype_order)} atom types")
    if bt: w(f"{len(bt)} bond types")
    if at: w(f"{len(at)} angle types")
    if dt: w(f"{len(dt)} dihedral types")
    if it: w(f"{len(it)} improper types")
    w("")

    if topology.box is not None:
        (xlo, xhi), (ylo, yhi), (zlo, zhi), (xy, xz, yz) = topology.box.lammps_bounds()
        w(f"{L(xlo):.6f} {L(xhi):.6f} xlo xhi")
        w(f"{L(ylo):.6f} {L(yhi):.6f} ylo yhi")
        w(f"{L(zlo):.6f} {L(zhi):.6f} zlo zhi")
        if topology.box.is_triclinic():
            w(f"{L(xy):.6f} {L(xz):.6f} {L(yz):.6f} xy xz yz")
    w("")

    w("Masses\n")
    for name in atype_order:
        m = (tmap[name].mass if name in tmap and tmap[name].mass else
             next((a.mass for a in topology.atoms if (a.type or 'X') == name and a.mass), 0.0))
        w(f"{atype_id[name]} {m:.5f}  # {name}")
    w("")

    # Pair Coeffs (epsilon sigma) in backend units
    if any(name in tmap and tmap[name].lj for name in atype_order):
        w("Pair Coeffs\n")
        for name in atype_order:
            lj = tmap[name].lj if name in tmap else {}
            eps = sysu.from_canonical(lj.get("epsilon", 0.0), U.ENERGY)
            sig = sysu.from_canonical(lj.get("sigma", 0.0), U.LENGTH)
            w(f"{atype_id[name]} {eps:.6f} {sig:.6f}  # {name}")
        w("")

    def coeffs(title, table, category):
        if not table:
            return
        w(f"{title}\n")
        for e in table:
            p = ff.to_backend(category, e["form"], e["params"], sysu)
            vals = _ordered(category, e["form"], p)
            w(f"{e['id']} " + " ".join(f"{v:.6f}" for v in vals)
              + f"  # {e['form']}")
        w("")

    coeffs("Bond Coeffs", bt, "bond")
    coeffs("Angle Coeffs", at, "angle")
    coeffs("Dihedral Coeffs", dt, "dihedral")
    coeffs("Improper Coeffs", it, "improper")

    w("Atoms  # full\n")
    for a in topology.atoms:
        x, y, z = (L(p) for p in (a.position or [0.0, 0.0, 0.0]))
        # Pad the leading integer columns and right-align the charge so the
        # charge values end at the same position (matching the .itp/.psf and
        # legacy .data formatting).
        w(f"{a.id + 1:<8}{a.molecule_id:<8}{atype_id[a.type or 'X']:<8}"
          f"{(a.charge or 0.0):>12.6f}  {x:.6f} {y:.6f} {z:.6f}")
    w("")

    def terms(title, term_list, idx_attr):
        if not term_list:
            return
        w(f"{title}\n")
        for n, t in enumerate(term_list, 1):
            tid = int(t.type_ref.split("_")[-1])
            idxs = " ".join(str(getattr(t, x) + 1) for x in idx_attr)
            w(f"{n} {tid} {idxs}")
        w("")

    terms("Bonds", topology.bonds, ("i", "j"))
    terms("Angles", topology.angles, ("i", "j", "k"))
    terms("Dihedrals", topology.dihedrals, ("i", "j", "k", "l"))
    terms("Impropers", topology.impropers, ("i", "j", "k", "l"))

    with open(file_path, "w", encoding="utf-8") as f:
        f.write("\n".join(out))

    if emit_input_snippet:
        _write_lammps_input(topology, emit_input_snippet, units, atom_style)
    return file_path


def _write_lammps_input(topology, path, units, atom_style):
    styles = {}
    for cat, attr in (("bond", "bonds"), ("angle", "angles"),
                      ("dihedral", "dihedrals"), ("improper", "impropers")):
        forms = {t.form for t in getattr(topology, attr)}
        if forms:
            styles[cat] = ff.lammps_style(cat, sorted(forms)[0])
    with open(path, "w", encoding="utf-8") as f:
        f.write(f"# {provenance_string()} — LAMMPS settings\nunits {units}\natom_style {atom_style}\n")
        f.write("pair_style lj/cut/coul/long 10.0\n")
        for cat, style in styles.items():
            f.write(f"{cat}_style {style}\n")


# ---------------------------------------------------------------------------
# CHARMM .psf + .prm
# ---------------------------------------------------------------------------
def write_psf(topology: Topology, file_path: str, *, prm_out: Optional[str] = None,
              segid: str = "MIN", validate: bool = True) -> str:
    if validate:
        tvalidate.validate(topology, raise_on_error=True)

    def section(title, items, per_line, fmt):
        s = [f"{len(items):>8d} !{title}"]
        row = []
        for it in items:
            row.append(fmt(it))
            if len(row) == per_line:
                s.append("".join(row)); row = []
        if row:
            s.append("".join(row))
        s.append("")
        return s

    lines = ["PSF EXT", "", "%8d !NTITLE" % 1,
             f"* {provenance_string()}", ""]
    # !NATOM
    lines.append(f"{len(topology.atoms):>8d} !NATOM")
    for a in topology.atoms:
        lines.append(
            f"{a.id + 1:>10d} {segid:<8s} {a.residue_id:<8d} {a.residue_name:<8s} "
            f"{(a.name or a.type):<8s} {(a.type or 'X'):<6s} "
            f"{(a.charge or 0.0):>10.6f} {(a.mass or 0.0):>13.4f}{0:>12d}")
    lines.append("")
    lines += section("NBOND: bonds", [(b.i + 1, b.j + 1) for b in topology.bonds],
                     4, lambda t: f"{t[0]:>10d}{t[1]:>10d}")
    lines += section("NTHETA: angles", [(a.i + 1, a.j + 1, a.k + 1) for a in topology.angles],
                     3, lambda t: f"{t[0]:>10d}{t[1]:>10d}{t[2]:>10d}")
    lines += section("NPHI: dihedrals",
                     [(d.i + 1, d.j + 1, d.k + 1, d.l + 1) for d in topology.dihedrals],
                     2, lambda t: f"{t[0]:>10d}{t[1]:>10d}{t[2]:>10d}{t[3]:>10d}")
    lines += section("NIMPHI: impropers",
                     [(d.i + 1, d.j + 1, d.k + 1, d.l + 1) for d in topology.impropers],
                     2, lambda t: f"{t[0]:>10d}{t[1]:>10d}{t[2]:>10d}{t[3]:>10d}")

    with open(file_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")

    if prm_out:
        _write_prm(topology, prm_out)
    return file_path


def _write_prm(topology: Topology, path: str):
    """Companion CHARMM .prm. Per-site uniqueness via site_label->unique type
    names so same-FF-type sites with different params stay distinct."""
    c = "charmm"
    site_type = ttyping.extract_atom_types_by_site(topology)
    lines = [f"* {provenance_string()} — CHARMM parameter file", "*", "", "BONDS"]
    seen = set()
    for b in topology.bonds:
        ti, tj = site_type[b.i], site_type[b.j]
        p = ff.to_backend("bond", b.form, b.params, c)
        key = (ti, tj, round(p.get("k", 0), 4), round(p.get("b0", 0), 4))
        if key in seen:
            continue
        seen.add(key)
        lines.append(f"{ti:<8s} {tj:<8s} {p.get('k', 0):>10.4f} {p.get('b0', 0):>10.4f}")
    lines += ["", "ANGLES"]
    seen.clear()
    for a in topology.angles:
        ti, tj, tk = site_type[a.i], site_type[a.j], site_type[a.k]
        p = ff.to_backend("angle", a.form, a.params, c)
        key = (ti, tj, tk, round(p.get("k", 0), 4), round(p.get("theta0", 0), 4))
        if key in seen:
            continue
        seen.add(key)
        lines.append(f"{ti:<8s} {tj:<8s} {tk:<8s} {p.get('k', 0):>10.4f} {p.get('theta0', 0):>10.4f}")
    lines += ["", "NONBONDED"]
    tmap = topology.atom_type_map()
    done = set()
    for a in topology.atoms:
        st = site_type[a.id]
        if st in done:
            continue
        done.add(st)
        lj = tmap[a.type].lj if a.type in tmap else {}
        eps = ff.to_backend("nonbonded", "lj", {"epsilon": lj.get("epsilon", 0.0),
                                                "sigma": lj.get("sigma", 0.0)}, c)
        rmin2 = eps.get("sigma", 0.0) * (2 ** (1 / 6)) / 2.0
        lines.append(f"{st:<8s} 0.0 {-abs(eps.get('epsilon', 0.0)):>10.4f} {rmin2:>10.4f}")
    lines += ["", "END"]
    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")
