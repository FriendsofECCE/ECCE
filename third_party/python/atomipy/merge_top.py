"""
N-way GROMACS topology merger for atomipy.

Merges any number of topology components (mineral, organic, or mixed) into a
single self-contained GROMACS .top + .gro pair that OpenMM (or GROMACS) can
load directly.

Key design constraints
-----------------------
* Each component carries: a list of atomipy atom-dicts, an itp-dict (as
  returned by import_itp / import_gaff_top), and a box vector.
* Atom types across components must not collide.  GAFF types (c3, hc, oh, ...)
  and MINFF/CLAYFF types (Ob, Sit, Alo, H, Oh, ...) never overlap, so the
  merge is purely additive.
* The merged [ defaults ] uses comb-rule 2 (Lorentz-Berthelot) — the GAFF
  convention.  MINFF and CLAYFF also run acceptably with comb-rule 2.
* Ions (Na, Cl, ...) are NOT inlined into the merged [ atomtypes ]; they are
  expected to be referenced via the existing min.ff/ions.itp include in the
  mineral forcefield, which is already handled by write_top.py / openmm_interface.py.

Public API
----------
merge_top(*components, output_box=None)
    Each component: {'atoms': [...], 'itp': {...}, 'box': [...]}
    Returns (atoms_merged, itp_merged, box_merged).

merge_top_files(*pairs, out_top, out_gro, defines=None, box=None)
    Convenience wrapper: alternating (top_path, gro_path) or component dicts.
    Reads each pair with import_gaff_top + import_conf, merges, writes files.

write_merged_top(atoms_merged, itp_merged, box_merged, out_top, out_gro,
                 mineral_ff='minff', minff_variant='GMINFF_k500',
                 water_model='spce', ion_model='SPCE_HFE_LM',
                 molecule_name='System')
    Writes the final self-contained .top and .gro files.
"""

from __future__ import annotations

import copy
import os
import re
from collections import defaultdict
from datetime import datetime
from typing import Any, Dict, List, Optional, Sequence, Tuple

from ._provenance import provenance_string

# atomipy-native atom dict type alias for documentation
AtomList = List[Dict[str, Any]]
ITPDict  = Dict[str, Any]


def _charge_to_float(value) -> float:
    """Robustly parse a charge that may be a float or PDB-style string.

    Handles numeric values, '', None, and PDB trailing-sign notation like
    '2-' (=-2), '1+' (=+1). Returns 0.0 for anything unparseable.
    """
    if value is None or value == "":
        return 0.0
    try:
        return float(value)
    except (TypeError, ValueError):
        s = str(value).strip()
        if s and s[-1] in "+-":               # PDB notation: '2-', '1+'
            sign = -1.0 if s[-1] == "-" else 1.0
            try:
                return sign * float(s[:-1] or "1")
            except ValueError:
                return 0.0
        return 0.0


def _ion_key(name: str) -> str:
    """Normalise an ion name for charge-tolerant matching.

    Upper-cases and strips a trailing charge so the charged and uncharged
    spellings collapse to the same key: 'Na'/'Na+'/'NA+' -> 'NA',
    'Ca'/'Ca2+' -> 'CA', 'Cl'/'Cl-' -> 'CL', 'Fe2+' -> 'FE'. (Different
    oxidation states of the same element share a key — callers prefer an exact
    match first and only use the key when it is unambiguous.)
    """
    return re.sub(r'\d*[+-]+$', '', str(name).strip().upper())


# ---------------------------------------------------------------------------
# Internal helpers
# ---------------------------------------------------------------------------

def _box_volume(box):
    """Return approximate scalar volume in Å³ for comparison."""
    if box is None:
        return 0.0
    b = list(box)
    if len(b) >= 3:
        return float(b[0]) * float(b[1]) * float(b[2])
    return 0.0


def _max_box(*boxes):
    """Return the component-wise maximum of all box vectors (Å)."""
    valid = [b for b in boxes if b is not None and len(b) >= 3]
    if not valid:
        return [50.0, 50.0, 50.0]
    # Use the longest box length among all valid boxes
    max_len = max(len(b) for b in valid)
    result = [0.0] * max_len
    
    for i in range(max_len):
        vals = [float(b[i]) for b in valid if i < len(b)]
        if vals:
            result[i] = max(vals)
    return result


def _parse_ion_moleculetypes(path: str) -> dict:
    """Parse ions.itp -> {define: {molname: atomtype}}.

    Each [ moleculetype ] under a top-level #ifdef maps its molecule name to the
    atomtype its single [ atoms ] entry references.
    """
    out: dict = {}
    define = None
    depth = 0  # nested POSRES_ION etc.
    try:
        with open(path, encoding='utf-8') as fh:
            lines = [l.rstrip('\n') for l in fh]
    except OSError:
        return out
    i = 0
    while i < len(lines):
        s = lines[i].strip()
        if s.startswith('#ifdef'):
            tok = s.split()[1] if len(s.split()) > 1 else ''
            if define is None and tok != 'POSRES_ION':
                define = tok
            else:
                depth += 1
        elif s.startswith('#endif'):
            if depth > 0:
                depth -= 1
            else:
                define = None
        elif s == '[ moleculetype ]' and define:
            # next non-comment, non-blank line is "molname nrexcl"
            j = i + 1
            while j < len(lines) and (not lines[j].strip() or lines[j].strip().startswith(';')):
                j += 1
            molname = lines[j].split()[0] if j < len(lines) and lines[j].split() else None
            # find the [ atoms ] block and its first data row's atomtype (col 2)
            atype = None
            k = j + 1
            while k < len(lines) and lines[k].strip() != '[ moleculetype ]' and not lines[k].strip().startswith('#endif'):
                if lines[k].strip() == '[ atoms ]':
                    m = k + 1
                    while m < len(lines) and (not lines[m].strip() or lines[m].strip().startswith(';')):
                        m += 1
                    cols = lines[m].split() if m < len(lines) else []
                    if len(cols) >= 2:
                        atype = cols[1]
                    break
                k += 1
            if molname and atype:
                out.setdefault(define, {})[molname] = atype
        i += 1
    return out


def _parse_atomtypes_by_define(path: str) -> dict:
    """Parse ffnonbonded.itp -> {define: set(atomtype names)} for #ifdef blocks."""
    out: dict = {}
    define = None
    try:
        with open(path, encoding='utf-8') as fh:
            in_atomtypes = False
            for line in fh:
                s = line.strip()
                if s.startswith('#ifdef'):
                    tok = s.split()[1] if len(s.split()) > 1 else ''
                    if tok != 'POSRES_ION':
                        define = tok
                elif s.startswith('#endif'):
                    define = None
                elif s.startswith('['):
                    in_atomtypes = (s == '[ atomtypes ]')
                elif define and s and not s.startswith(';'):
                    name = s.split()[0]
                    out.setdefault(define, set()).add(name)
    except OSError:
        pass
    return out


def _resolve_ion_define(ion_model: str, water_model: str, ion_molnames=None) -> str:
    """Return an ion-model #define from the min.ff library that actually provides
    the ions present in the system.

    Ion sets are water-model specific AND not every block contains every ion
    (e.g. ffnonbonded's SPCE_IOD_LM lists only trivalent ions; monovalent Na+/Cl-
    live under all_IOD_LM / OPC3_IOD_LM / *_HFE_LM …). So a define is only usable
    if, for each ion in the system, ions.itp defines its [ moleculetype ] AND
    ffnonbonded defines the atomtype that moleculetype references — both under
    that same #ifdef. Preference: the requested set, common 3-site waters, then
    'all', then JC.
    """
    if not ion_model:
        return ion_model
    base = os.path.join(os.path.dirname(__file__), 'ffparams', 'min.ff')
    moltypes = _parse_ion_moleculetypes(os.path.join(base, 'ions.itp'))
    attypes  = _parse_atomtypes_by_define(os.path.join(base, 'ffnonbonded.itp'))
    all_ion_keys = {_ion_key(m) for block in moltypes.values() for m in block}
    # Charge-tolerant: a system 'Na+' still matches the 'Na' moleculetype, etc.
    needed = {_ion_key(n) for n in (ion_molnames or []) if _ion_key(n) in all_ion_keys}

    def _covers(define: str) -> bool:
        mt = moltypes.get(define, {})       # {molname: referenced atomtype}
        at = attypes.get(define, set())     # ffnonbonded atomtypes under this define
        keys = {_ion_key(m): a for m, a in mt.items()}
        if needed:
            return all(k in keys and keys[k] in at for k in needed)
        # No explicit ion list: require at least one monovalent atomtype present.
        return any(a in at for a in ('Na+', 'Cl-', 'K+'))

    if _covers(ion_model):
        return ion_model
    ionset = ion_model.split('_', 1)[1] if '_' in ion_model else ion_model
    candidates = (
        [f'{w}_{ionset}' for w in ('all', 'SPCE', 'TIP3P', 'OPC3', 'TIP4PEW', 'OPC', 'TIP3PFB', 'TIP4PFB')]
        + sorted(d for d in moltypes if d.endswith('_' + ionset))
        + [f'{w}_JC' for w in (water_model.upper(), 'SPCE', 'SPC', 'TIP3P')]
        + sorted(moltypes)
    )
    seen = set()
    for cand in candidates:
        if cand in seen:
            continue
        seen.add(cand)
        if _covers(cand):
            if cand != ion_model:
                print(f"write_merged_top: ion set '{ion_model}' does not provide all "
                      f"requested ions for this water model; using '{cand}' instead.")
            return cand
    return ion_model


def _remap_ion_molnames(mol_counts, ion_model):
    """Rewrite ion names in the [ molecules ] list to the exact [ moleculetype ]
    name present in the chosen ions.itp block, tolerating charged/uncharged
    spelling (so a system 'Na+' maps to the 'Na' moleculetype, or 'Fe' to
    'Fe2+'). GROMACS/OpenMM require an exact [ molecules ] ↔ [ moleculetype ]
    name match, so this prevents "Unknown molecule type" for either spelling.
    """
    if not ion_model:
        return mol_counts
    base = os.path.join(os.path.dirname(__file__), 'ffparams', 'min.ff')
    block = _parse_ion_moleculetypes(os.path.join(base, 'ions.itp')).get(ion_model, {})
    if not block:
        return mol_counts
    by_key = defaultdict(list)
    for m in block:
        by_key[_ion_key(m)].append(m)
    out = []
    for name, count in mol_counts:
        if name in block:                       # exact match — keep
            out.append((name, count))
            continue
        cands = by_key.get(_ion_key(name))
        if cands and len(cands) == 1:           # unambiguous charge-tolerant match
            out.append((cands[0], count))
        else:                                   # not an ion in this block, or ambiguous
            out.append((name, count))
    return out


def _normalize_itp(itp: Optional[ITPDict]) -> ITPDict:
    """Return a safe copy of itp, defaulting missing sections to empty dicts."""
    if itp is None:
        itp = {}
    base = {
        'atomtypes':    {},
        'moleculetype': {},
        'atoms':        {},
        'bonds':        {},
        'pairs':        {},
        'angles':       {},
        'dihedrals':    {},
        'dihedrals2':   {},
        'impropers':    {},
    }
    base.update(copy.deepcopy(itp))
    return base


def _itp_col_len(section: dict) -> int:
    """Return the number of rows in an itp section dict."""
    if not section:
        return 0
    first_key = next(iter(section), None)
    if first_key is None:
        return 0
    val = section[first_key]
    return len(val) if isinstance(val, list) else 0


def _itp_row(section: dict, i: int) -> dict:
    """Return row i as a dict."""
    return {k: v[i] for k, v in section.items() if isinstance(v, list)}


def _append_itp_row(section: dict, row: dict):
    """Append a row (dict) to a section dict, initialising missing lists."""
    for k, v in row.items():
        if k not in section:
            section[k] = []
        section[k].append(v)


def _merge_atomtypes(itp_a: ITPDict, itp_b: ITPDict) -> dict:
    """
    Merge two [ atomtypes ] sections.
    On collision (same name) the first definition wins and a warning is printed.
    """
    merged: dict = {}
    seen:   dict = {}  # name -> column values dict

    for src in (itp_a.get('atomtypes', {}), itp_b.get('atomtypes', {})):
        n = _itp_col_len(src)
        if n == 0:
            continue
        # detect name column
        name_col = 'name' if 'name' in src else ('type' if 'type' in src else None)
        if name_col is None:
            continue
        for i in range(n):
            row = _itp_row(src, i)
            name = str(row.get(name_col, '')).strip()
            if not name:
                continue
            if name in seen:
                # Check if parameters differ — warn only if they do
                existing = seen[name]
                sigma_key = 'sigma' if 'sigma' in row else 'v'
                eps_key   = 'epsilon' if 'epsilon' in row else 'w'
                if (abs(float(existing.get(sigma_key, 0)) - float(row.get(sigma_key, 0))) > 1e-8 or
                        abs(float(existing.get(eps_key, 0)) - float(row.get(eps_key, 0))) > 1e-8):
                    print(f"merge_top: WARNING — duplicate atomtype '{name}' with different "
                          f"parameters; keeping first definition.")
                continue  # skip duplicate regardless
            seen[name] = row
            _append_itp_row(merged, row)

    return merged


# ---------------------------------------------------------------------------
# Index re-mapping helpers
# ---------------------------------------------------------------------------

def _shift_index_col(section: dict, col: str, offset: int) -> dict:
    """Add `offset` to every value in column `col` (in-place copy)."""
    if col in section and offset:
        new_vals = []
        for v in section[col]:
            try:
                new_vals.append(int(float(v)) + offset)
            except (ValueError, TypeError):
                new_vals.append(v)
        section[col] = new_vals
    return section


def _shift_bond_section(section: dict, offset: int) -> dict:
    """Shift both atom-index columns in a bonds/pairs section."""
    for col in ('ai', 'aj'):
        _shift_index_col(section, col, offset)
    return section


def _shift_angle_section(section: dict, offset: int) -> dict:
    """Shift three atom-index columns in an angles section."""
    for col in ('ai', 'aj', 'ak'):
        _shift_index_col(section, col, offset)
    return section


def _shift_dihedral_section(section: dict, offset: int) -> dict:
    """Shift four atom-index columns in a dihedrals/impropers section."""
    for col in ('ai', 'aj', 'ak', 'al'):
        _shift_index_col(section, col, offset)
    return section


def _concat_sections(sec_a: dict, sec_b: dict) -> dict:
    """Concatenate two itp section dicts column-wise."""
    merged = copy.deepcopy(sec_a)
    n_a = _itp_col_len(sec_a)
    n_b = _itp_col_len(sec_b)
    if n_b == 0:
        return merged
    # Ensure all columns from sec_b exist in merged
    all_keys = set(merged.keys()) | set(sec_b.keys())
    for k in all_keys:
        if k not in merged:
            merged[k] = ['' ] * n_a
        if k not in sec_b:
            merged[k].extend(['' ] * n_b)
        else:
            merged[k].extend(copy.deepcopy(sec_b[k]))
    return merged


def _build_mineral_itp(atoms_in: list, box: Optional[list]) -> dict:
    """Dynamically construct an itp topology dict for mineral components."""
    from .bond_angle import bond_angle
    
    # Fallback box dimension if none is provided
    box_vec = list(box) if box is not None and len(box) >= 3 else [50.0, 50.0, 50.0]
    
    try:
        updated_atoms, Bond_index, Angle_index = bond_angle(
            atoms_in, box_vec, rmaxH=1.2, rmaxM=2.45,
            same_element_bonds=False, same_molecule_only=True
        )
    except Exception:
        updated_atoms = atoms_in
        Bond_index = []
        Angle_index = []
    # ------------------------------------------------------------------
    # Filter bonds/angles to only those with parameters in min.ff/ffbonded.itp
    # MINFF/CLAYFF is fundamentally non-bonded for the bulk framework:
    #   - Alo-Ob, Alo-Op, Sit-Ob, Sit-Op etc. are NOT bonded in this FF
    #   - Only O-H, Alo-Oalh/Oalhh, and Sit-Osih have explicit bond params
    # ------------------------------------------------------------------
    BONDED_PAIRS = {
        frozenset(['Oh',   'H']),
        frozenset(['Ob',   'H']),
        frozenset(['Ohmg', 'H']),
        frozenset(['Oalh', 'H']),
        frozenset(['Oalhh','H']),
        frozenset(['Osih', 'H']),
        frozenset(['Alo',  'Oalh']),
        frozenset(['Alo',  'Oalhh']),
        frozenset(['Sit',  'Osih']),
    }
    def _atom_fftype(idx):
        return str(updated_atoms[idx].get('fftype', updated_atoms[idx].get('type', ''))).strip()

    filtered_bonds = []
    if Bond_index is not None and len(Bond_index) > 0:
        for b in Bond_index:
            ai, aj = int(b[0]), int(b[1])
            pair = frozenset([_atom_fftype(ai), _atom_fftype(aj)])
            if pair in BONDED_PAIRS:
                filtered_bonds.append(b)

    # Build a local-index map: 0-based array position → 1-based local index
    # bond_angle() stores 0-based array positions in Bond_index / Angle_index.
    # A GROMACS [ moleculetype ] must number its own atoms LOCALLY from 1 — its [ bonds ]/
    # [ angles ]/[ position_restraints ] all reference those local nr. Use the atom's
    # POSITION in this component (1..N), NOT its global atom['index']: when the mineral is
    # split into per-layer moleculetypes the atoms keep the whole-system indices (e.g.
    # 41..80 for layer 2), which would make that block start at 41 and break grompp.
    local_idx = list(range(1, len(updated_atoms) + 1))     # local nr, 1-based per moleculetype

    # Classify every scanned angle (bond_angle rows are [i, j, k, angle_deg], 0-based)
    # via atomipy's shared angle model (write_top.angle_parameters) — the SAME logic
    # write_top.itp() uses, no duplicated criteria:
    #   'moh'   M-O-H  -> standard θ0/k stored explicitly (c0, c1)
    #   'metal' O-M-O / M-O-M -> explicit scanned θ0 (c0); the Ka force constant
    #           (c1) is filled at write time from the chosen angle Ka.
    from .write_top import angle_parameters

    def _is_h(ff):     return ff[:1].upper() == 'H'
    def _is_o(ff):     return ff[:1] == 'O'
    def _is_metal(ff): return bool(ff) and not _is_o(ff) and not _is_h(ff) and ff != 'Fs'

    ang_ai, ang_aj, ang_ak, ang_c0, ang_c1, ang_cat = [], [], [], [], [], []
    n_metal = n_moh = 0
    if Angle_index is not None and len(Angle_index) > 0:
        for an in Angle_index:
            ai, aj, ak = int(an[0]), int(an[1]), int(an[2])
            t1, t2, t3 = _atom_fftype(ai), _atom_fftype(aj), _atom_fftype(ak)
            angle_val = float(an[3]) if len(an) > 3 else 0.0
            theta0, ktheta, cat = angle_parameters(t1, t2, t3, angle_val, KANGLE=0.0)
            tl = (t1, t2, t3)
            if cat == 'moh':
                if not (any(_is_metal(t) for t in tl) and any(_is_o(t) for t in tl)):
                    continue                                  # genuine M-O-H only
                c0, c1 = f'{theta0:.2f}', f'{ktheta:.3f}'     # standard, fixed
                n_moh += 1
            elif cat == 'metal':
                if not (any(_is_metal(t) for t in tl) and all(_is_o(t) or _is_metal(t) for t in tl)):
                    continue                                  # genuine O-M-O / M-O-M only
                c0, c1 = f'{theta0:.2f}', ''                  # c1 (Ka) filled at write time
                n_metal += 1
            else:
                continue                                      # water/other not in mineral itp
            ang_ai.append(local_idx[ai]); ang_aj.append(local_idx[aj]); ang_ak.append(local_idx[ak])
            ang_c0.append(c0); ang_c1.append(c1); ang_cat.append(cat)

    # Name the built mineral moleculetype after the atoms' resname (so a user can
    # name a mineral, e.g. PYRO/KAOL) rather than hard-coding 'MIN'. Defaults to
    # 'MIN'. merge_top() still dedups collisions (MIN, MIN_1, ...) and syncs the
    # [ molecules ] entry + resnames to match.
    _mt_name = 'MIN'
    if updated_atoms:
        _rn0 = str(updated_atoms[0].get('resname') or '').strip()
        if _rn0:
            _mt_name = _rn0
    # Residue numbers must be LOCAL to the moleculetype (start at 1), not the whole-system
    # molid. Map this component's molids to 1..K in first-seen order, so a single slab (one
    # molid) becomes all resnr 1 regardless of its molid in the .gro/.pdb; an (un-split)
    # multi-molid mineral gets a clean local 1,2,3,...
    _mol_local = {}
    _local_resnr = []
    for a in updated_atoms:
        m = a.get('molid', 1)
        if m not in _mol_local:
            _mol_local[m] = len(_mol_local) + 1
        _local_resnr.append(_mol_local[m])
    itp = {
        # nrexcl=2 for inorganic mineral moleculetypes (MINFF/CLAYFF exclude 1-2 neighbours;
        # nonbonded metal-O interactions must NOT be excluded at 3 bonds). Organics use 3.
        'moleculetype': {'moleculetype': [_mt_name], 'nrexcl': [2]},
        'atoms': {
            'nr':      local_idx,
            'type':    [a.get('fftype', a.get('type', 'X')) for a in updated_atoms],
            'resnr':   _local_resnr,
            'residue': [a.get('resname', 'MIN') for a in updated_atoms],
            'atom':    [a.get('type', 'X') for a in updated_atoms],
            'cgnr':    local_idx,
            'charge':  [float(a.get('charge') or 0.0) for a in updated_atoms],
            'mass':    [float(a.get('mass') or 0.0) for a in updated_atoms]
        },
        'bonds': {
            'ai':    [local_idx[int(b[0])] for b in filtered_bonds],
            'aj':    [local_idx[int(b[1])] for b in filtered_bonds],
            'funct': [1] * len(filtered_bonds),
            'c0':    [''] * len(filtered_bonds),
            'c1':    [''] * len(filtered_bonds)
        } if filtered_bonds else {},
        'angles': {
            'ai':       ang_ai,
            'aj':       ang_aj,
            'ak':       ang_ak,
            'funct':    [1] * len(ang_ai),
            'c0':       ang_c0,                # θ0 (deg): scanned for metal, standard for M-O-H
            'c1':       ang_c1,                # k: standard for M-O-H; '' for metal (Ka at write)
            'category': ang_cat,               # 'metal' or 'moh'
        } if ang_ai else {}
    }
    print(f"  _build_mineral_itp: {len(filtered_bonds)} bonded pairs, "
          f"{n_metal} metal (O-M-O/M-O-M) + {n_moh} M-O-H angles available")
    return itp



# ---------------------------------------------------------------------------
# Core merge function
# ---------------------------------------------------------------------------

def _split_mineral_components_by_resname(components):
    """Split any built-from-scratch mineral component (``itp`` is None) that contains
    MORE THAN ONE distinct residue name into one sub-component per residue name, so each
    becomes its own ``[ moleculetype ]`` in the output .top (e.g. three stacked clay
    layers the user named MIN1/MIN2/MIN3 -> MIN1 1 / MIN2 1 / MIN3 1 in ``[ molecules ]``).

    A single-resname mineral, or any pre-built (``itp`` provided) component, passes through
    unchanged, so the default behaviour is identical. Splitting is keyed on RESNAME (not
    molid) on purpose: resnames are coarse and user-controlled, so a structure with per-atom
    molids (e.g. from an external PDB) still yields ONE moleculetype instead of exploding
    into thousands. Atoms of each resname are kept in first-seen order, so each group stays
    contiguous in the merged atom list — a requirement for a valid GROMACS [ molecules ]
    sequence. Bonds are rebuilt per group by _build_mineral_itp, so give distinct resnames
    only to parts that are genuinely separate molecules (stacked layers have no inter-layer
    bonds, so this is safe).
    """
    out = []
    for c in components:
        atoms = c.get('atoms', [])
        if c.get('itp') is not None or not atoms:
            out.append(c)
            continue
        order = []
        groups = {}
        for a in atoms:
            rn = str(a.get('resname', 'MIN') or 'MIN')
            if rn not in groups:
                groups[rn] = []
                order.append(rn)
            groups[rn].append(a)
        if len(order) <= 1:
            out.append(c)
            continue
        for rn in order:
            out.append({'atoms': groups[rn], 'itp': None, 'box': c.get('box')})
    return out


def merge_top(
    *components,
    output_box: Optional[list] = None,
) -> Tuple[AtomList, ITPDict, list]:
    """
    Merge any number of topology components into a single atomipy representation.

    Parameters
    ----------
    *components : dict
        Each dict must have keys:
            'atoms'  : list of atomipy atom-dicts (x, y, z, type, fftype, charge, ...)
            'itp'    : itp-dict from import_itp / import_gaff_top (may be None for minerals)
            'box'    : list [a, b, c] or [a, b, c, alpha, beta, gamma] in Å
    output_box : list, optional
        Override the merged box. If None, uses component-wise maximum.

    Returns
    -------
    atoms_merged : list of atom dicts with updated 'index' and 'molid' fields
    itp_merged   : merged itp-dict with unified atomtypes + renumbered topology
    box_merged   : list [a, b, c] (or full 6-vector) in Å
    """
    if not components:
        raise ValueError("merge_top: at least one component is required")

    # A single mineral component carrying several distinct residue names (e.g. stacked
    # clay layers named MIN1/MIN2/MIN3) is expanded into one component per resname, so each
    # gets its own [ moleculetype ]. No-op for single-resname minerals.
    components = _split_mineral_components_by_resname(components)

    # ------------------------------------------------------------------
    # 1. Determine merged box
    # ------------------------------------------------------------------
    all_boxes = [c.get('box') for c in components]
    if output_box is not None:
        box_merged = list(output_box)
    else:
        box_merged = _max_box(*all_boxes)

    # ------------------------------------------------------------------
    # 2. Merge [ atomtypes ]
    # ------------------------------------------------------------------
    # Build a fresh combined itp
    itp_list = []
    original_itps = []
    for c in components:
        raw_itp = c.get('itp')
        if raw_itp is None:
            raw_itp = _build_mineral_itp(c.get('atoms', []), c.get('box'))
        itp_list.append(_normalize_itp(raw_itp))
        original_itps.append(raw_itp)
    merged_atomtypes = {}
    seen_names: set = set()

    for itp in itp_list:
        src = itp.get('atomtypes', {})
        n = _itp_col_len(src)
        if n == 0:
            continue
        name_col = 'name' if 'name' in src else ('type' if 'type' in src else None)
        if name_col is None:
            continue
        for i in range(n):
            row = _itp_row(src, i)
            name = str(row.get(name_col, '')).strip()
            if not name or name in seen_names:
                continue
            seen_names.add(name)
            _append_itp_row(merged_atomtypes, row)

    # ------------------------------------------------------------------
    # 3. Merge atoms + topology sections with index renumbering
    # ------------------------------------------------------------------
    atoms_merged: AtomList = []
    atom_offset = 0          # running 1-based offset
    molid_offset = 0         # running moleculetype offset

    # Sections that need renumbering
    merged_atoms_sec  = {}
    merged_bonds      = {}
    merged_pairs      = {}
    merged_angles     = {}
    merged_dihedrals  = {}
    merged_dihedrals2 = {}
    merged_impropers  = {}
    merged_moltype    = {}   # will contain all moleculetype blocks concatenated

    component_labels: List[str] = []  # for [ molecules ] table

    for comp_idx, (comp, itp) in enumerate(zip(components, itp_list)):
        atoms_in = comp.get('atoms', [])
        n_atoms  = len(atoms_in)
        if n_atoms == 0:
            continue

        # Determine a unique molecule name for this component
        mol_name = None
        mt = itp.get('moleculetype', {})
        if mt and 'moleculetype' in mt and mt['moleculetype']:
            mol_name = str(mt['moleculetype'][0]).strip()
        if not mol_name:
            mol_name = 'MOL'
        # Ensure uniqueness
        base_name = mol_name
        suffix = 1
        while mol_name in component_labels:
            mol_name = f"{base_name}_{suffix}"
            suffix += 1
            
        # Synchronize ITP to match the newly assigned unique name
        if 'moleculetype' not in itp:
            itp['moleculetype'] = {'moleculetype': [mol_name], 'nrexcl': [3]}
        elif itp['moleculetype'].get('moleculetype'):
            itp['moleculetype']['moleculetype'][0] = mol_name
            
        component_labels.append(mol_name)

        # Resnames that should NOT be overridden — water and ions carry their own
        # classification resnames which _gromacs_mol_name() relies on.
        _skip_resname_sync = {
            'SOL', 'WAT', 'HOH', 'TIP3', 'OPC', 'OPC3', 'SPC', 'SPCE', 'TIP4', 'TIP5',
            'ION', 'NA', 'CL', 'K', 'LI', 'CS', 'RB', 'F', 'BR', 'I', 'CA', 'MG', 'ZN',
            'Na', 'Cl', 'Ca', 'Mg', 'Zn', 'Li', 'Cs', 'Rb',
        }
        sync_resname = mol_name if mol_name.upper() not in {r.upper() for r in _skip_resname_sync} else None

        # Re-index atoms
        for local_i, atom in enumerate(atoms_in):
            new_atom = dict(atom)
            new_atom['index']  = atom_offset + local_i + 1
            new_atom['molid']  = molid_offset + int(atom.get('molid', 1))
            # Synchronize resname to the itp moleculetype name so that
            # get_mol_sequence() correctly identifies the GROMACS molecule name.
            # ACPYPE assigns 'MOL'/'LIG' in the .gro but names the moleculetype
            # 'organic' in the .itp — we need the itp name in [ molecules ].
            if sync_resname is not None:
                new_atom['resname'] = sync_resname
            # Tag with forcefield component for debugging
            if 'component' not in new_atom:
                new_atom['component'] = comp_idx
            atoms_merged.append(new_atom)

        # Renumber itp [ atoms ] section
        atoms_sec = copy.deepcopy(itp.get('atoms', {}))
        if atoms_sec:
            _shift_index_col(atoms_sec, 'nr',   atom_offset)
            _shift_index_col(atoms_sec, 'resnr', molid_offset)
            merged_atoms_sec = _concat_sections(merged_atoms_sec, atoms_sec)

        # Renumber bonds
        bonds = copy.deepcopy(itp.get('bonds', {}))
        if bonds:
            _shift_bond_section(bonds, atom_offset)
            merged_bonds = _concat_sections(merged_bonds, bonds)

        # Renumber pairs
        pairs = copy.deepcopy(itp.get('pairs', {}))
        if pairs:
            _shift_bond_section(pairs, atom_offset)
            merged_pairs = _concat_sections(merged_pairs, pairs)

        # Renumber angles
        angles = copy.deepcopy(itp.get('angles', {}))
        if angles:
            _shift_angle_section(angles, atom_offset)
            merged_angles = _concat_sections(merged_angles, angles)

        # Renumber dihedrals (proper)
        dihedrals = copy.deepcopy(itp.get('dihedrals', {}))
        if dihedrals:
            _shift_dihedral_section(dihedrals, atom_offset)
            merged_dihedrals = _concat_sections(merged_dihedrals, dihedrals)

        # Renumber dihedrals (improper, block 2)
        dihedrals2 = copy.deepcopy(itp.get('dihedrals2', {}))
        if dihedrals2:
            _shift_dihedral_section(dihedrals2, atom_offset)
            merged_dihedrals2 = _concat_sections(merged_dihedrals2, dihedrals2)

        # Impropers
        impropers = copy.deepcopy(itp.get('impropers', {}))
        if impropers:
            _shift_dihedral_section(impropers, atom_offset)
            merged_impropers = _concat_sections(merged_impropers, impropers)

        # Accumulate moleculetype entry
        nrexcl = 3
        if mt and 'nrexcl' in mt and mt['nrexcl']:
            try:
                nrexcl = int(mt['nrexcl'][0])
            except (ValueError, TypeError):
                pass
        _append_itp_row(merged_moltype, {'moleculetype': mol_name, 'nrexcl': nrexcl})

        # Advance offsets
        # molid offset: advance by number of unique molids in this component
        if atoms_in:
            molids = [int(a.get('molid', 1)) for a in atoms_in]
            molid_offset += max(molids) - min(molids) + 1
        atom_offset += n_atoms

    # ------------------------------------------------------------------
    # 4. Assemble merged itp
    # ------------------------------------------------------------------
    itp_merged: ITPDict = {
        'atomtypes':    merged_atomtypes,
        'moleculetype': merged_moltype,
        'atoms':        merged_atoms_sec,
        'bonds':        merged_bonds,
        'pairs':        merged_pairs,
        'angles':       merged_angles,
        'dihedrals':    merged_dihedrals,
        'dihedrals2':   merged_dihedrals2,
        'impropers':    merged_impropers,
        '_component_labels': component_labels,  # for write_merged_top
        '_original_itps': original_itps,
    }

    return atoms_merged, itp_merged, box_merged


# ---------------------------------------------------------------------------
# File-level writer
# ---------------------------------------------------------------------------

def write_merged_top(
    atoms_merged: AtomList,
    itp_merged:   ITPDict,
    box_merged:   list,
    out_top:      str,
    out_gro:      str,
    *,
    # Mineral force field include
    mineral_ff:    str  = 'minff',
    minff_variant: str  = 'GMINFF_k500',
    water_model:   str  = 'spce',
    ion_model:     str  = 'SPCE_HFE_LM',
    molecule_name: str  = 'Mixed System',
    # Organic itp files to #include (written by ACPYPE, relative to out_top dir)
    organic_itps:  Optional[Sequence[str]] = None,
    # Angle force constant Ka (kJ/mol/rad²) for mineral O-M-O / M-O-M angles.
    # None  -> emit NO [ angles ] at all (CLAYFF default, MINFF "No angles").
    # 0/250/500/1500 -> write explicit angles: scanned θ0 for metal angles at
    # this Ka, and the standard θ0/k for M-O-H. The GMINFF_k nonbonded block is
    # unaffected (written separately).
    angle_ka:  Optional[float] = 500.0,
    # Optional explicit [ molecules ] override: a sequence of (name, count) pairs.
    # When given, it REPLACES the auto-derived sequence (get_mol_sequence) — the
    # user states the molecule order/counts directly. The caller is responsible
    # for consistency (Σ count×moleculetype_size == atom count, and matching
    # order). Names must match defined moleculetypes / #included .itp molnames.
    mol_counts_override: Optional[Sequence[Tuple[str, int]]] = None,
    # Position-restraint force constant (kJ/mol/nm²) written into each mineral .itp behind
    # a shared '#ifdef POSRES'. Every mineral atom is restrained, so a single -DPOSRES at
    # grompp freezes all mineral slabs. Set None to omit the block entirely.
    posres_fc: Optional[float] = 1000.0,
) -> None:
    """
    Write a self-contained GROMACS .top file and a .gro coordinate file for a
    merged system produced by merge_top().

    The .top structure is:
      [ defaults ]        — comb-rule 2, gen-pairs no (fudge params apply only to explicit
                            [ pairs ] entries; GAFF organic itp has explicit [ pairs ],
                            MINFF has none — so no conflict)
      #include min.ff/ffnonbonded.itp  (mineral/water/ion atomtypes, no [ defaults ])
      #include min.ff/ffbonded.itp     (O-H + edge bond/angle types, mineral only)
      #include organic_GMX.itp ...    (GAFF atomtypes, inline)
      [ moleculetype ] / [ atoms ] / [ bonds ] / [ angles ] / ...  per component
      [ system ] / [ molecules ]

    Parameters
    ----------
    out_top        : path for the output .top file
    out_gro        : path for the output .gro file
    mineral_ff     : 'minff', 'clayff', or 'gminff' — selects the #ifdef macro
    minff_variant  : preprocessor define for the mineral FF variant
    water_model    : water model include (spce, opc3, tip3p, ...)
    ion_model      : ion parametrisation set (SPCE_HFE_LM, OPC3_IOD_LM, ...)
    organic_itps   : list of organic .itp filenames to #include (relative paths)
    """
    from .write_conf import gro  as write_gro_fn
    from .cell_utils  import Cell2Box_dim

    total_charge = sum(_charge_to_float(a.get('charge')) for a in atoms_merged)

    # Detect what's present in the system via the shared classifier (resname AND
    # atomtype; mineral set derived from the FF library). This decides which
    # parameter blocks the topology activates, so only what's used is included.
    from .composition import system_component_flags
    _flags = system_component_flags(atoms_merged, has_organic_itp=bool(organic_itps))
    has_water   = _flags['water']
    has_ions    = _flags['ion']
    has_mineral = _flags['mineral']
    has_organic = _flags['organic']

    component_labels = itp_merged.get('_component_labels', [])

    # ------------------------------------------------------------------
    # Count molecules for [ molecules ] section using composition.get_mol_sequence,
    # which correctly handles generic 'ION' resnames, water, minerals, and organic
    # molecules — all keyed by molid boundaries.
    from .composition import get_mol_sequence
    if mol_counts_override:
        # User-specified [ molecules ] sequence (e.g. from the Topology editor).
        mol_counts: List[Tuple[str, int]] = [(str(n), int(c)) for n, c in mol_counts_override]
        _auto = get_mol_sequence(atoms_merged)
        _auto_atoms = sum(c for _, c in _auto)  # informational only
        print(f"write_merged_top: using caller-supplied [ molecules ] override "
              f"({len(mol_counts)} entries, {sum(c for _, c in mol_counts)} molecules) "
              f"in place of the auto-detected {len(_auto)} entries.")
    else:
        mol_counts: List[Tuple[str, int]] = get_mol_sequence(atoms_merged)

        # Reconcile the mineral [ molecules ] counts with the actual mineral moleculetypes.
        # The mineral is written as ONE moleculetype per component (each a single instance
        # containing all its atoms), but get_mol_sequence counts by molid — so a multi-molid
        # single-resname mineral (e.g. stacked clay layers with "Reset MolID" off) is
        # miscounted (MIN 3) against a one-moleculetype MIN, making the topology expect
        # N x more mineral atoms than the .gro has (grompp: coordinate/topology mismatch).
        # Rebuild those entries as one-per-moleculetype so counts and atoms agree.
        if has_mineral:
            _min_mts, _seen_mt = [], set()
            for _itp in itp_merged.get('_original_itps', []) or []:
                if not _itp or _itp.get('_source_itp'):
                    continue
                _mt = _itp.get('moleculetype', {})
                if not _mt or 'moleculetype' not in _mt:
                    continue
                _nm, _b, _s = _mt['moleculetype'][0], _mt['moleculetype'][0], 1
                while _nm in _seen_mt:
                    _nm = f"{_b}_{_s}"; _s += 1
                _seen_mt.add(_nm)
                _min_mts.append(_nm)
            if _min_mts:
                _min_match = set(_min_mts) | {n.split('_')[0] for n in _min_mts}
                _reco, _done = [], False
                for _n, _c in mol_counts:
                    if _n in _min_match:
                        if not _done:
                            _reco.extend((nm, 1) for nm in _min_mts)
                            _done = True
                    else:
                        _reco.append((_n, _c))
                if not _done:
                    _reco = [(nm, 1) for nm in _min_mts] + _reco
                mol_counts = _reco

    # ------------------------------------------------------------------
    # Write .top
    # ------------------------------------------------------------------
    with open(out_top, 'w', encoding='utf-8') as f:
        f.write(f'; {provenance_string()} — merge_top mixed system\n')
        f.write(f'; Total charge: {total_charge:.6f}\n')
        f.write(f'; Components: {", ".join(component_labels)}\n\n')

        # [ defaults ] — GAFF convention (works for MINFF/CLAYFF too)
        f.write('[ defaults ]\n')
        f.write('; nbfunc   comb-rule   gen-pairs   fudgeLJ   fudgeQQ\n')
        f.write('  1        2           yes         0.5       0.8333333333\n\n')

        # Mineral/water/ion FF atomtypes — use ffnonbonded.itp (no [ defaults ] inside)
        # forcefield.itp is NOT used: it wraps ffnonbonded+ffbonded but also adds [ defaults ],
        # which would create a duplicate [ defaults ] block and override our gen-pairs=yes above.
        if has_mineral or has_water or has_ions:
            # Activate only the atomtype blocks the system actually uses, so a
            # MINFF-free / ion-free topology never pulls in unused parameter sets.
            if has_mineral:
                f.write(f'#define {minff_variant}\n')
            if has_water:
                # Activate the water-model atomtype block (#ifdef OPC3 / SPCE /
                # TIP4PEW / ...) in ffnonbonded.itp. The water .itp only
                # *references* OW_xxx/HW_xxx — those [ atomtypes ] are defined
                # behind this #ifdef, so without the define OpenMM/GROMACS raises
                # KeyError: 'OW_opc3'. (The ion #define provides ions only.)
                _water_define = water_model.lower().replace('/', '').replace('-', '').upper()
                f.write(f'#define {_water_define}\n')
            if ion_model and has_ions:
                ion_model = _resolve_ion_define(
                    ion_model, water_model,
                    ion_molnames=[name for name, _ in mol_counts])
                f.write(f'#define {ion_model}\n')
            f.write('#include "min.ff/ffnonbonded.itp"\n')  # atomtypes only, no [ defaults ]
            if has_mineral:
                f.write('#include "min.ff/ffbonded.itp"\n')  # O-H + edge bond/angle types
            f.write('\n')

        # Mineral moleculetype(s): one self-contained .itp per slab, #included like the
        # water/ion topologies (local 1-based indexing + a shared '#ifdef POSRES' block).
        if has_mineral:
            _mineral_itp_files = _write_mineral_itp_files(
                out_top, itp_merged, angle_ka=angle_ka, posres_fc=posres_fc)
            if _mineral_itp_files:
                f.write('; Mineral/s\n')
                for _fn, _ in _mineral_itp_files:
                    f.write(f'#include "{_fn}"\n')
                f.write('\n')

        # Organic itp includes (GAFF atomtypes + molecule topology, incl. explicit [ pairs ])
        if organic_itps:
            f.write('; Organic molecule topologies\n')
            for oitp in organic_itps:
                f.write(f'#include "{os.path.basename(oitp)}"\n')
            f.write('\n')

        # Water model
        if has_water:
            wm = water_model.lower().replace('/', '').replace('-', '')
            wm_map = {
                # 3-site
                'spce': 'spce', 'spc': 'spc', 'tip3p': 'tip3p', 'opc3': 'opc3',
                'tip3pfb': 'tip3p-fb',
                # 4-site (each has its own M-site itp — must NOT collapse to a 3-site model)
                'opc': 'opc', 'tip4p': 'tip4p', 'tip4pew': 'tip4pew', 'tip4pfb': 'tip4p-fb',
            }
            wm_file = wm_map.get(wm, 'spce')
            f.write(f'; Water model\n')
            f.write(f'#include "min.ff/{wm_file}.itp"\n\n')

        # Ions
        if has_ions:
            f.write('; Ions\n')
            f.write('#include "min.ff/ions.itp"\n\n')

        # (Mineral moleculetypes are written to separate #include'd .itp files above.)

        # [ system ] and [ molecules ]
        f.write('[ system ]\n')
        f.write(f'{molecule_name}\n\n')
        f.write('[ molecules ]\n')
        f.write('; Compound        nmols\n')
        # Match ion names to the chosen ion block's moleculetype spelling
        # (charge-tolerant: Na/Na+, Ca/Ca2+, …).
        # A caller override is written verbatim (the user's exact molnames); the
        # automatic ion-name remap only applies to the auto-derived sequence.
        if mol_counts_override:
            out_mol_counts = mol_counts
        else:
            out_mol_counts = _remap_ion_molnames(mol_counts, ion_model) if has_ions else mol_counts
        for mol_name, count in out_mol_counts:
            f.write(f'{mol_name:<18} {count}\n')
        f.write('\n')

    # ------------------------------------------------------------------
    # Write .gro
    # ------------------------------------------------------------------
    try:
        write_gro_fn(atoms_merged, box_merged, out_gro)
    except Exception:
        # Fallback: write minimal GRO manually
        _write_gro_fallback(atoms_merged, box_merged, out_gro)

    print(f'write_merged_top: wrote {out_top} ({len(atoms_merged)} atoms, '
          f'{len(component_labels)} component(s))')


def _write_one_mineral_moleculetype(f, itp, molname, angle_ka=500.0, posres_fc=1000.0):
    """Write a single mineral [ moleculetype ] (+ [ atoms ]/[ bonds ]/[ angles ] and, if
    posres_fc is set, a shared '#ifdef POSRES [ position_restraints ]' restraining every
    atom) to file object ``f``. All indices are the itp's own local 1-based nr."""
    mt = itp.get('moleculetype', {})
    nrexcl = mt['nrexcl'][0] if 'nrexcl' in mt else 3

    f.write('[ moleculetype ]\n')
    f.write('; name             nrexcl\n')
    f.write(f'{molname:<18} {nrexcl}\n\n')

    atoms_sec = itp.get('atoms', {})
    n_atoms = 0
    if atoms_sec and 'nr' in atoms_sec:
        f.write('[ atoms ]\n')
        f.write('; nr  type  resnr  residue  atom  cgnr  charge  mass\n')
        n_atoms = len(atoms_sec['nr'])
        for i in range(n_atoms):
            _chg = float(atoms_sec['charge'][i] or 0.0)
            _mass = float(atoms_sec['mass'][i] or 0.0)
            f.write(f"{atoms_sec['nr'][i]:>6} {str(atoms_sec['type'][i] or 'X'):<10} "
                    f"{atoms_sec['resnr'][i]:>5} {str(atoms_sec['residue'][i] or 'MIN'):<8} "
                    f"{str(atoms_sec['atom'][i] or 'X'):<8} {atoms_sec['cgnr'][i]:>5} "
                    f"{_chg:>12.6f} {_mass:>12.6f}\n")
        f.write('\n')

    bonds = itp.get('bonds', {})
    if bonds and 'ai' in bonds:
        f.write('[ bonds ]\n')
        f.write('; ai   aj   funct\n')
        for i in range(len(bonds['ai'])):
            f.write(f"{bonds['ai'][i]:>5} {bonds['aj'][i]:>5} {bonds['funct'][i]:>5}\n")
        f.write('\n')

    angles = itp.get('angles', {})
    if angle_ka is not None and angles and 'ai' in angles:
        n = len(angles['ai'])
        cats = angles.get('category', ['metal'] * n)
        c0s  = angles.get('c0', [''] * n)
        c1s  = angles.get('c1', [''] * n)
        f.write('[ angles ]\n')
        f.write('; ai   aj   ak   funct      th0          cth\n')
        for i in range(n):
            th0 = c0s[i] if i < len(c0s) else ''
            # Metal O-M-O / M-O-M get the chosen Ka; M-O-H keep their standard k.
            kth = f'{float(angle_ka):.3f}' if cats[i] == 'metal' else (c1s[i] if i < len(c1s) else '')
            if th0 != '' and kth != '':
                f.write(f"{angles['ai'][i]:>5} {angles['aj'][i]:>5} {angles['ak'][i]:>5} "
                        f"{angles['funct'][i]:>5} {th0:>10} {kth:>12}\n")
            else:
                f.write(f"{angles['ai'][i]:>5} {angles['aj'][i]:>5} {angles['ak'][i]:>5} "
                        f"{angles['funct'][i]:>5}\n")
        f.write('\n')

    # Shared POSRES block: restrain EVERY atom of this slab. Guarded by the single 'POSRES'
    # define, so grompp -DPOSRES freezes all mineral slabs at once (no per-slab defines).
    if posres_fc and n_atoms > 0:
        _fc = f'{float(posres_fc):.1f}'
        f.write('#ifdef POSRES\n')
        f.write('[ position_restraints ]\n')
        f.write('; ai  funct      fcx        fcy        fcz\n')
        for i in range(n_atoms):
            f.write(f"{atoms_sec['nr'][i]:>6}     1 {_fc:>10} {_fc:>10} {_fc:>10}\n")
        f.write('#endif\n\n')


def _write_mineral_itp_files(out_top, itp_merged, angle_ka=500.0, posres_fc=1000.0):
    """Write each mineral [ moleculetype ] to its own ``minN.itp`` next to ``out_top`` and
    return a list of (filename, molname) to #include from the .top — mirroring the water/ion
    layout. Names are deduped exactly as merge_top() deduped the [ molecules ] entries
    (MIN, MIN_1, ...), so [ molecules ] entries still resolve to a defined moleculetype.
    """
    original_itps = itp_merged.get('_original_itps', [])
    if not original_itps:
        return []
    out_dir = os.path.dirname(os.path.abspath(out_top)) or '.'
    _seen_mineral_names: set = set()
    files = []
    n = 0
    for itp in original_itps:
        if itp is None or itp.get('_source_itp'):
            continue
        mt = itp.get('moleculetype', {})
        if not mt or 'moleculetype' not in mt:
            continue
        molname = mt['moleculetype'][0]
        _base, _suf = molname, 1
        while molname in _seen_mineral_names:
            molname = f"{_base}_{_suf}"
            _suf += 1
        _seen_mineral_names.add(molname)
        n += 1
        fname = f"min{n}.itp"
        with open(os.path.join(out_dir, fname), 'w', encoding='utf-8') as mf:
            mf.write(f'; {provenance_string()} — mineral moleculetype {molname}\n\n')
            _write_one_mineral_moleculetype(mf, itp, molname, angle_ka=angle_ka, posres_fc=posres_fc)
        files.append((fname, molname))
    return files


def _write_gro_fallback(atoms: AtomList, box: list, out_gro: str) -> None:
    """Write a minimal GRO file when write_conf.gro is unavailable."""
    with open(out_gro, 'w', encoding='utf-8') as f:
        f.write(f'{provenance_string()} — mixed system\n')
        f.write(f'{len(atoms)}\n')
        for atom in atoms:
            molid   = int(atom.get('molid', 1))
            resname = str(atom.get('resname', 'MOL'))[:5]
            name    = str(atom.get('type', atom.get('name', 'X')))[:5]
            idx     = int(atom.get('index', 1))
            x = float(atom.get('x', 0.0)) / 10.0  # Å → nm
            y = float(atom.get('y', 0.0)) / 10.0
            z = float(atom.get('z', 0.0)) / 10.0
            f.write(f'{molid%100000:5d}{resname:<5}{name:>5}{idx%100000:5d}'
                    f'{x:8.3f}{y:8.3f}{z:8.3f}\n')
        # Box line (nm)
        b = list(box)
        bx = float(b[0]) / 10.0 if len(b) > 0 else 5.0
        by = float(b[1]) / 10.0 if len(b) > 1 else 5.0
        bz = float(b[2]) / 10.0 if len(b) > 2 else 5.0
        f.write(f'{bx:10.5f}{by:10.5f}{bz:10.5f}\n')


# ---------------------------------------------------------------------------
# Convenience file-level wrapper
# ---------------------------------------------------------------------------

def merge_top_files(
    *top_gro_pairs: str,
    out_top: str,
    out_gro: str,
    defines: Optional[list] = None,
    box: Optional[list] = None,
    mineral_ff: str = 'minff',
    minff_variant: str = 'GMINFF_k500',
    water_model: str = 'spce',
    ion_model: str = 'SPCE_HFE_LM',
    angle_ka: Optional[float] = 500.0,
) -> Tuple[AtomList, ITPDict, list]:
    """
    Read alternating (top_path, gro_path) pairs, merge them all, write files.

    Example
    -------
    merge_top_files(
        'mineral.top', 'mineral.gro',
        'organic_GMX.top', 'organic_GMX.gro',
        out_top='mixed.top', out_gro='mixed.gro',
    )
    """
    from .import_top import import_gaff_top, import_gro_coords

    if len(top_gro_pairs) % 2 != 0:
        raise ValueError("merge_top_files: arguments must be alternating top_path, gro_path pairs")

    components = []
    organic_itps = []
    for i in range(0, len(top_gro_pairs), 2):
        top_path = top_gro_pairs[i]
        gro_path = top_gro_pairs[i + 1]
        atoms, itp = import_gaff_top(top_path)
        import_gro_coords(gro_path, atoms)
        # Detect box from gro
        comp_box = _box_from_gro(gro_path)
        components.append({'atoms': atoms, 'itp': itp, 'box': comp_box})
        # Collect organic itp paths for #include
        if itp.get('_source_itp'):
            organic_itps.append(itp['_source_itp'])

    atoms_merged, itp_merged, box_merged = merge_top(*components, output_box=box)
    write_merged_top(
        atoms_merged, itp_merged, box_merged,
        out_top, out_gro,
        mineral_ff=mineral_ff,
        minff_variant=minff_variant,
        water_model=water_model,
        ion_model=ion_model,
        organic_itps=organic_itps or None,
        angle_ka=angle_ka,
    )
    return atoms_merged, itp_merged, box_merged


def _box_from_gro(gro_path: str) -> list:
    """Read the box line from a .gro file and return [a, b, c] in Å."""
    try:
        with open(gro_path, encoding='utf-8') as fh:
            lines = fh.readlines()
        parts = lines[-1].split()
        if len(parts) >= 3:
            a = float(parts[0]) * 10.0
            b = float(parts[1]) * 10.0
            c = float(parts[2]) * 10.0
            if a > 0 and b > 0 and c > 0:
                return [a, b, c]
    except Exception:
        pass
    return [50.0, 50.0, 50.0]
