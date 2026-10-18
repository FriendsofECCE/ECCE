"""
Composition analysis for atomipy atom structures.

Provides the `composition` function to report system composition by residue
and atom type — equivalent to MATLAB atom_MATLAB_toolbox/general_functions/composition_atom.m.

Typical usage
-------------
atoms, box = ap.import_gro('system.gro')
comp = ap.composition(atoms)
# Prints a formatted table; also returns a dict for programmatic use.
"""

from __future__ import annotations

import math
from collections import defaultdict, OrderedDict
from itertools import groupby
from typing import Dict, List, Optional, Tuple


def _charge_to_float(value) -> float:
    """Parse a charge that may be numeric or PDB string notation ('2-' -> -2)."""
    if value is None or value == "":
        return 0.0
    try:
        return float(value)
    except (TypeError, ValueError):
        s = str(value).strip()
        if s and s[-1] in "+-":
            sign = -1.0 if s[-1] == "-" else 1.0
            try:
                return sign * float(s[:-1] or "1")
            except ValueError:
                return 0.0
        return 0.0

# ---------------------------------------------------------------------------
# GROMACS molecule name resolution
# ---------------------------------------------------------------------------

#: Resnames treated as water/solvent
_SOLVENT_RESNAMES = {
    'SOL', 'WAT', 'HOH', 'TIP3', 'OPC', 'OPC3', 'SPC', 'SPCE', 'TIP4', 'TIP5',
}

#: Resnames / element names treated as monoatomic ions
_ION_RESNAMES = {
    'ION', 'NA', 'CL', 'K', 'LI', 'CS', 'RB', 'F', 'BR', 'I', 'CA', 'MG', 'ZN',
    'NA+', 'CL-', 'K+', 'CA2+', 'MG2+', 'ZN2+',
}

#: Canonical GROMACS ion names used in min.ff/ions.itp, keyed by element (uppercase)
_ION_ELEMENT_TO_GMXNAME: Dict[str, str] = {
    'NA': 'Na', 'CL': 'Cl', 'K': 'K',  'CA': 'Ca', 'MG': 'Mg',
    'ZN': 'Zn', 'LI': 'Li', 'CS': 'Cs', 'RB': 'Rb', 'F':  'F',
    'BR': 'Br', 'I':  'I',
}


# --- Component classification (resname AND atomtype) -------------------------
#: Ion resnames (extends _ION_RESNAMES with plural + CHARMM/AMBER conventions)
_ION_RESNAMES_EXT = _ION_RESNAMES | {'IONS', 'SOD', 'POT', 'CLA', 'CAL', 'NA+', 'CL-'}
#: Ion atomtypes — only UNAMBIGUOUS forms (charged notation + CHARMM names).
#: Bare element symbols (NA, CL, CA…) are intentionally excluded: they collide
#: with GAFF organic atomtypes (e.g. 'ca' aromatic C, 'cl'/'br'/'i'/'f' halogens).
#: Ions are reliably identified by resname instead (_ION_RESNAMES_EXT).
_ION_ATOMTYPES = {
    'NA+', 'CL-', 'K+', 'LI+', 'CA2+', 'MG2+', 'ZN2+', 'F-', 'BR-', 'I-',
    'SOD', 'POT', 'CLA', 'CAL',
}
#: Atomtype prefixes (case-insensitive) indicating a water molecule
_WATER_ATOMTYPE_PREFIXES = ('ow', 'hw', 'oh2', 'mw')
#: Resnames indicating an organic / ligand / biomolecule
_ORGANIC_RESNAMES = {
    'LIG', 'API', 'MOL', 'UNL', 'UNK', 'DRG', 'INH',
    # Amino acids (standard 3-letter + His protonation variants)
    'ALA', 'ARG', 'ASN', 'ASP', 'CYS', 'GLN', 'GLU', 'GLY', 'HIS', 'ILE',
    'LEU', 'LYS', 'MET', 'PHE', 'PRO', 'SER', 'THR', 'TRP', 'TYR', 'VAL',
    'HID', 'HIE', 'HIP', 'HSD', 'HSE', 'HSP',
    # Common protonation / non-standard / terminal-cap residues
    'CYX', 'CYM', 'ASH', 'GLH', 'LYN', 'ARN', 'HYP', 'ACE', 'NME', 'NHE', 'SEC', 'PYL',
    # DNA nucleotides (PDB v3 / Amber) + 5'/3' terminal forms
    'DA', 'DC', 'DG', 'DT', 'DU',
    'DA5', 'DA3', 'DC5', 'DC3', 'DG5', 'DG3', 'DT5', 'DT3', 'DU5', 'DU3',
    # RNA nucleotides + alt prefixes + 5'/3' terminal forms
    'A', 'C', 'G', 'U', 'RA', 'RC', 'RG', 'RU',
    'A5', 'A3', 'C5', 'C3', 'G5', 'G3', 'U5', 'U3',
    'RA5', 'RA3', 'RC5', 'RC3', 'RG5', 'RG3', 'RU5', 'RU3',
}
#: Resnames indicating a mineral framework
_MINERAL_RESNAMES = {
    'MIN', 'MINERAL', 'MMT', 'PYR', 'KAO', 'CLAY',
    # Zeolite framework codes (first-3-letters of the bundled zeolite presets;
    # 'MFI' is the IZA code for ZSM-5)
    'ABW', 'ANA', 'CHA', 'ETR', 'EUO', 'EZT', 'FAU', 'FER', 'HEU', 'MOR',
    'PHI', 'SOD', 'ZSM', 'MFI',
}
#: Baseline MINFF/CLAYFF mineral atomtypes (extended from the FF library at runtime)
_MINERAL_ATOMTYPES_BASE = {
    'Ob', 'Obos', 'Obts', 'Obss', 'Ohs', 'Op', 'Oh', 'Oas', 'Oahs', 'Oal',
    'Osih', 'Oalh', 'Oalhh', 'Sit', 'Site', 'Alo', 'Alt', 'Ale', 'Mgo', 'Mgh',
    'Mge', 'Cao', 'Cah', 'Lio', 'Nao', 'Feo', 'Fet', 'Fee', 'Fe2', 'Fe3',
    'Tio', 'Mno', 'Cro', 'Zno', 'H',
    'st', 'ao', 'at', 'mgo', 'mgh', 'feo', 'fet', 'cao', 'ob', 'oh', 'ho', 'op', 'ohs',
}
_mineral_atomtypes_cache = None


def _mineral_atomtypes():
    """Comprehensive MINFF/CLAYFF mineral atomtypes — extended from the bundled FF
    library so structural Fe/Ti/etc. are recognised (not just a hard-coded subset).
    Falls back to the baseline set if the library can't be loaded."""
    global _mineral_atomtypes_cache
    if _mineral_atomtypes_cache is not None:
        return _mineral_atomtypes_cache
    types = set(_MINERAL_ATOMTYPES_BASE)
    try:
        from . import ffparams
        data = ffparams.load_json('GMINFF/gminff_all.json')
        for block, bd in (data.get('nonbonded_blocks') or {}).items():
            if 'MINFF' in block.upper() or 'CLAYFF' in block.upper():
                types.update((bd.get('atomtypes') or {}).keys())
    except Exception:
        pass
    # never let an ion/water type masquerade as mineral
    types = {t for t in types if t.upper() not in _ION_ATOMTYPES
             and not t.lower().startswith(_WATER_ATOMTYPE_PREFIXES)}
    _mineral_atomtypes_cache = types
    return types


def classify_atom(atom: dict, mineral_types=None) -> str:
    """Classify an atom as ``'water' | 'ion' | 'organic' | 'mineral' | 'other'``
    using BOTH resname and atomtype.

    Precedence: water → ion → organic-resname → mineral. The organic resname
    (MOL/LIG/amino acids/…) is checked *before* mineral-by-atomtype so GAFF
    atomtypes that collide with CLAYFF lowercase types (e.g. ``oh``/``ho``) are
    not mis-read as mineral. Use resname ``MIN`` for mineral frameworks.
    """
    if mineral_types is None:
        mineral_types = _mineral_atomtypes()
    rn = str(atom.get('resname') or '').strip().upper()
    at = str(atom.get('type') or atom.get('fftype') or '').strip()
    if rn in _SOLVENT_RESNAMES or any(at.lower().startswith(p) for p in _WATER_ATOMTYPE_PREFIXES):
        return 'water'
    if rn in _ION_RESNAMES_EXT or at.upper() in _ION_ATOMTYPES:
        return 'ion'
    if rn in _ORGANIC_RESNAMES:
        return 'organic'
    if at in mineral_types or rn in _MINERAL_RESNAMES:
        return 'mineral'
    return 'other'


def system_component_flags(atoms, has_organic_itp: bool = False) -> Dict[str, bool]:
    """Presence flags {water, ion, mineral, organic} for a system, so a topology
    can activate ONLY the parameter sets it actually contains."""
    mt = _mineral_atomtypes()
    flags = {'water': False, 'ion': False, 'mineral': False, 'organic': bool(has_organic_itp)}
    for a in atoms:
        c = classify_atom(a, mt)
        if c in flags:
            flags[c] = True
    return flags


def _gromacs_mol_name(atom: dict) -> str:
    """Return the canonical GROMACS molecule name for a representative atom.

    Applies the same classification used by min.ff/ions.itp and min.ff/*.itp
    water models so that the returned names can be used directly in the
    ``[ molecules ]`` section of a GROMACS topology file.

    Parameters
    ----------
    atom : dict
        Representative atom dictionary from the molecule/residue group.

    Returns
    -------
    str
        Canonical GROMACS molecule name (e.g. 'SOL', 'Na', 'Cl', 'MIN').
    """
    rn = str(atom.get('resname') or '').strip().upper()

    if rn in _SOLVENT_RESNAMES:
        return 'SOL'

    if rn in _ION_RESNAMES:
        # Resolve the actual ion element when resname is the generic 'ION' or an ion type
        raw = str(atom.get('type') or atom.get('element') or rn).strip().upper()
        clean_raw = raw.rstrip('+-0123456789')
        return _ION_ELEMENT_TO_GMXNAME.get(clean_raw, _ION_ELEMENT_TO_GMXNAME.get(raw, clean_raw.capitalize()[:4]))

    # Non-empty, non-solvent, non-ion resname → return as-is, preserving exact
    # case so the name matches the [ moleculetype ] entry in the .itp file.
    orig = str(atom.get('resname') or '').strip()
    return orig if orig else 'MOL'


# ---------------------------------------------------------------------------
# Core composition function
# ---------------------------------------------------------------------------

def composition(atoms: list, Box=None, verbose: bool = True) -> dict:
    """Analyse and report the composition of an atomipy atom structure.

    Reports composition in three complementary ways that mirror the MATLAB
    ``composition_atom`` function:

    1. **By residue (resname + molid)** — how many distinct molecules/ions
       exist for each residue type, and how many atoms they contain in total.
    2. **By atom type** — count and mean partial charge for every unique
       ``fftype`` / ``type`` in the system.
    3. **System properties** — total mass, volume, and density when ``Box``
       is provided.

    Also computes the ordered molecule sequence (the ``[ molecules ]`` table
    in GROMACS topology format), which can be used to validate or generate
    the ``[ molecules ]`` section of a ``.top`` file.

    Parameters
    ----------
    atoms : list of dict
        List of atom dictionaries.  Each dict must have at least a ``molid``
        field; ``resname``, ``type``/``fftype``, ``charge``, and ``mass``
        are used when present.
    Box : list or array-like, optional
        Simulation cell dimensions in one of the supported formats:
        - 1x3: [lx, ly, lz] for orthogonal boxes
        - 1x6: [a, b, c, alpha, beta, gamma] for cell parameters
        - 1x9: [lx, ly, lz, 0, 0, xy, 0, xz, yz] for GROMACS triclinic format
        When provided, volume (Å³), total mass (amu), and density (g/cm³)
        are calculated and included in the output.
    verbose : bool, optional
        If True (default), print a formatted composition summary to stdout.

    Returns
    -------
    comp : dict
        Dictionary with the following keys:

        ``'resnames'`` : list of str
            Unique residue names, in order of first appearance.
        ``'nresidues'`` : list of int
            Number of distinct molecules (unique molid values) for each
            resname entry.
        ``'natoms'`` : list of int
            Total atom count for each resname entry.
        ``'atom_types'`` : list of str
            Unique atom types (fftype falling back to type), sorted.
        ``'atom_counts'`` : list of int
            Atom count for each entry in ``atom_types``.
        ``'atom_avg_charge'`` : list of float or None
            Mean partial charge per atom for each type; ``None`` if no
            ``charge`` field is present in the atoms.
        ``'total_charge'`` : float
            Sum of all partial charges in the system.
        ``'total_mass'`` : float or None
            Total mass in amu; ``None`` if no ``mass`` field is present.
        ``'volume'`` : float or None
            Cell volume in Å³; ``None`` if ``Box`` was not provided.
        ``'density'`` : float or None
            Density in g/cm³; ``None`` if ``Box`` or mass data are missing.
        ``'mol_sequence'`` : list of tuple[str, int]
            Ordered list of ``(gromacs_mol_name, count)`` pairs, suitable for
            direct use in the ``[ molecules ]`` section of a GROMACS .top
            file.  Consecutive groups with the same name are collapsed.

    Examples
    --------
    atoms, Box = ap.import_gro('mixed_system.gro')
    comp = ap.composition(atoms, Box)
    # Access programmatically:
    for mol_name, count in comp['mol_sequence']:
        print(f'{mol_name:12s} {count}')

    See Also
    --------
    update : Renumber atom indices and molecule IDs.
    molecule : Assign a single molecule ID to an atom group.
    get_structure_stats : Detailed coordination/bond/angle statistics for mineral atoms.
    """
    if not atoms:
        raise ValueError("atoms list cannot be empty")

    has_charge = any('charge' in a for a in atoms)
    has_mass   = any('mass'   in a for a in atoms)

    # ------------------------------------------------------------------
    # 0. Box / volume / density (optional)
    # ------------------------------------------------------------------
    # 1 amu/Å³ = 1.66053886 g/cm³
    _AMU_TO_G_PER_CM3 = 1.66053886

    volume  = None
    density = None
    total_mass = None

    if Box is not None:
        from .cell_utils import normalize_box
        Box_dim, Cell = normalize_box(Box)
        if Cell is not None:
            a, b, c = Cell[0], Cell[1], Cell[2]
            alpha, beta, gamma = Cell[3], Cell[4], Cell[5]
            if abs(alpha - 90) < 1e-6 and abs(beta - 90) < 1e-6 and abs(gamma - 90) < 1e-6:
                volume = a * b * c
            else:
                ar = math.radians(alpha)
                br = math.radians(beta)
                gr = math.radians(gamma)
                volume = a * b * c * math.sqrt(
                    1 - math.cos(ar)**2 - math.cos(br)**2 - math.cos(gr)**2
                    + 2 * math.cos(ar) * math.cos(br) * math.cos(gr)
                )

    if has_mass:
        total_mass = sum(float(a.get('mass') or 0.0) for a in atoms)

    if volume is not None and total_mass is not None and total_mass > 0:
        density = total_mass / volume * _AMU_TO_G_PER_CM3

    # ------------------------------------------------------------------
    # 1. Group atoms by molid → classify each unique (resname, molid) group
    # ------------------------------------------------------------------
    # Gather unique molids in order of first appearance
    seen_molids: OrderedDict = OrderedDict()
    for a in atoms:
        mid = a.get('molid', 0)
        if mid not in seen_molids:
            seen_molids[mid] = a  # keep first atom as representative

    # Build per-resname statistics
    resname_molids: Dict[str, set] = defaultdict(set)
    resname_natoms: Dict[str, int] = defaultdict(int)
    resname_order:  List[str]       = []

    for a in atoms:
        rn = str(a.get('resname') or 'UNK').strip()
        if rn not in resname_natoms:
            resname_order.append(rn)
        resname_natoms[rn] += 1
        resname_molids[rn].add(a.get('molid', 0))

    # ------------------------------------------------------------------
    # 2. Build the GROMACS [ molecules ] sequence from molid boundaries
    #    Group consecutive atoms by (gromacs_mol_name) derived from their molid
    # ------------------------------------------------------------------
    gmx_seq: List[str] = [_gromacs_mol_name(rep) for rep in seen_molids.values()]
    mol_sequence: List[Tuple[str, int]] = [
        (name, sum(1 for _ in group))
        for name, group in groupby(gmx_seq)
    ]

    # ------------------------------------------------------------------
    # 3. Atom-type statistics  (fftype preferred, fall back to type)
    # ------------------------------------------------------------------
    type_counts:   Dict[str, int]   = defaultdict(int)
    type_charges:  Dict[str, list]  = defaultdict(list)

    for a in atoms:
        atype = str(a.get('fftype') or a.get('type') or 'UNK').strip()
        type_counts[atype] += 1
        if has_charge and 'charge' in a:
            type_charges[atype].append(_charge_to_float(a['charge']))

    sorted_types  = sorted(type_counts.keys())
    atom_counts   = [type_counts[t] for t in sorted_types]
    atom_avg_q    = (
        [round(sum(type_charges[t]) / len(type_charges[t]), 6) if type_charges[t] else 0.0
         for t in sorted_types]
        if has_charge else []
    )
    total_charge  = sum(_charge_to_float(a.get('charge')) for a in atoms) if has_charge else 0.0

    # ------------------------------------------------------------------
    # 4. Verbose output
    # ------------------------------------------------------------------
    if verbose:
        n_total = len(atoms)
        print(f"\nSystem composition  ({n_total} atoms total)")

        # --- System properties (volume / mass / density) ---
        if volume is not None or total_mass is not None:
            print("\nSystem properties:")
            if volume is not None:
                print(f"  Volume:      {volume:.2f} Å³")
            if total_mass is not None:
                print(f"  Total mass:  {total_mass:.4f} amu")
            if density is not None:
                print(f"  Density:     {density:.4f} g/cm³")

        # --- By residue ---
        print("\nBy residue:")
        print(f"  {'Resname':<12} {'Molecules':>10} {'Atoms':>8}")
        print("  " + "-" * 32)
        for rn in resname_order:
            print(f"  {rn:<12} {len(resname_molids[rn]):>10} {resname_natoms[rn]:>8}")

        # --- GROMACS [ molecules ] sequence ---
        print("\n[ molecules ] sequence:")
        print(f"  {'Molecule':<16} {'Count':>8}")
        print("  " + "-" * 26)
        for mol_name, cnt in mol_sequence:
            print(f"  {mol_name:<16} {cnt:>8}")

        # --- By atom type ---
        print("\nBy atom type:")
        if has_charge:
            print(f"  {'Type':<12} {'Count':>8}  {'Avg charge':>12}")
            print("  " + "-" * 36)
            for t, cnt, q in zip(sorted_types, atom_counts, atom_avg_q):
                print(f"  {t:<12} {cnt:>8}  {q:>12.6f}")
            print(f"\n  Total charge: {total_charge:.6f} e")
        else:
            print(f"  {'Type':<12} {'Count':>8}")
            print("  " + "-" * 22)
            for t, cnt in zip(sorted_types, atom_counts):
                print(f"  {t:<12} {cnt:>8}")

    return {
        'resnames':        resname_order,
        'nresidues':       [len(resname_molids[rn]) for rn in resname_order],
        'natoms':          [resname_natoms[rn]       for rn in resname_order],
        'atom_types':      sorted_types,
        'atom_counts':     atom_counts,
        'atom_avg_charge': atom_avg_q,
        'total_charge':    total_charge,
        'total_mass':      total_mass,
        'volume':          volume,
        'density':         density,
        'mol_sequence':    mol_sequence,
    }


def get_mol_sequence(atoms: list) -> List[Tuple[str, int]]:
    """Return the ordered GROMACS ``[ molecules ]`` sequence without printing.

    Convenience wrapper around :func:`composition` for use in topology
    writing pipelines.

    Parameters
    ----------
    atoms : list of dict
        List of atom dictionaries with ``molid`` and ``resname`` fields.

    Returns
    -------
    mol_sequence : list of tuple[str, int]
        Ordered ``(gromacs_mol_name, count)`` pairs for the
        ``[ molecules ]`` section of a GROMACS topology file.

    Examples
    --------
    seq = ap.get_mol_sequence(atoms_merged)
    # [('MIN', 1), ('organic', 1), ('SOL', 673), ('Na', 6), ('Cl', 6)]
    """
    comp = composition(atoms, verbose=False)
    return comp['mol_sequence']


def get_mol_sequence_typed(atoms: list) -> List[Tuple[str, int, str]]:
    """Like :func:`get_mol_sequence`, but each entry is ``(name, count, klass)``
    where ``klass`` is the component class ('mineral' | 'organic' | 'ion' |
    'water' | 'other') of a representative atom of that molecule.

    Intended for UI display (e.g. the Topology editor), so a user can see both
    the apparent ``[ molecules ]`` sequence AND how each molecule was classified
    — making a mis-classification (e.g. an organic read as mineral) obvious.
    """
    from collections import OrderedDict
    from itertools import groupby
    mt = _mineral_atomtypes()
    seen: OrderedDict = OrderedDict()
    for a in atoms:
        mid = a.get('molid', 0)
        if mid not in seen:
            seen[mid] = a  # first atom of each molecule = representative
    named = [(_gromacs_mol_name(r), classify_atom(r, mt)) for r in seen.values()]
    out: List[Tuple[str, int, str]] = []
    for (name, klass), group in groupby(named):
        out.append((name, sum(1 for _ in group), klass))
    return out
