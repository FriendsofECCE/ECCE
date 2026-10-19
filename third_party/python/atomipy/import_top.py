"""
Import GROMACS .itp topology files.

This module mirrors the MATLAB `import_itp.m` utility and parses sections into
plain Python dictionaries (column name -> list of values) to make subsequent
editing straightforward.

Examples
--------
from atomipy import import_itp_topology
data = import_itp_topology("topology.itp")
data["atomtypes"]["name"][:3]  # ['opls_135', 'opls_136', 'opls_137']
"""

import os
from typing import Any, Dict, List, Optional, Sequence, Set, Tuple, Union

# NOTE: element.element() expects atom *dicts*; we use a local string helper here.

Number = Union[int, float]


def _itp_col_len(section: dict) -> int:
    """Return the number of rows in an itp section dict (column-list format)."""
    if not isinstance(section, dict) or not section:
        return 0
    first_key = next(iter(section), None)
    if first_key is None:
        return 0
    val = section[first_key]
    return len(val) if isinstance(val, list) else 0


def _strip_comment(line: str) -> str:
    """Remove inline comments starting with ';' or '#' and trim whitespace."""
    for marker in (";", "#"):
        if marker in line:
            line = line.split(marker, 1)[0]
    return line.strip()


def _to_number(value: str) -> Union[str, Number]:
    """Convert a string to int/float when possible, otherwise return the string."""
    try:
        number = float(value)
    except (TypeError, ValueError):
        return value

    # Preserve floating representation (avoid casting to int so we keep decimal precision,
    # especially for charges read from .itp files).
    return number


def _safe_int(val, default_val):
    """Safely convert any numeric-like string to int."""
    try:
        return int(float(str(val)))
    except (ValueError, TypeError):
        return default_val


def _safe_float(val, default_val):
    """Safely convert any numeric-like string to float."""
    try:
        return float(str(val))
    except (ValueError, TypeError):
        return default_val


def _parse_table(
    lines: Sequence[str],
    columns: Sequence[str],
    numeric_columns: Optional[Union[Sequence[str], Set[str]]] = None,
) -> Dict[str, List[Union[str, Number]]]:
    """Parse whitespace-separated table lines into a column dictionary."""
    numeric_set = set(numeric_columns or [])
    rows: List[List[Union[str, Number]]] = []

    for raw in lines:
        tokens = _strip_comment(raw).split()
        if not tokens:
            continue

        padded = tokens + [""] * (len(columns) - len(tokens))
        row: List[Union[str, Number]] = []
        for name, token in zip(columns, padded):
            if name in numeric_set and token != "":
                row.append(_to_number(token))
            else:
                row.append(token)
        rows.append(row)

    # Transpose into dict-of-lists
    column_data: Dict[str, List[Union[str, Number]]] = {name: [] for name in columns}
    for row in rows:
        for name, value in zip(columns, row):
            column_data[name].append(value)
    return column_data


def _choose_atomtype_columns(first_line: str) -> Tuple[List[str], List[str]]:
    """Select atomtype column labels based on the first data row."""
    tokens = _strip_comment(first_line).split()
    # Two common layouts:
    # 1) type name atnum charge ptype v w
    # 2) name atnum mass charge ptype sigma epsilon
    if len(tokens) >= 7 and tokens[2].replace(".", "", 1).isdigit():
        return (
            ["type", "name", "atnum", "charge", "ptype", "v", "w"],
            ["atnum", "charge", "v", "w"],
        )
    return (
        ["name", "atnum", "mass", "charge", "ptype", "sigma", "epsilon"],
        ["atnum", "mass", "charge", "sigma", "epsilon"],
    )


def import_itp(filename: str, itp_prev: Optional[Dict[str, Union[Dict, str]]] = None) -> Dict[str, Union[Dict, str]]:
    """
    Import a GROMACS .itp file into a dictionary of section dictionaries.

    Parameters
    ----------
    filename : str
        Path to the .itp file.
    itp_prev : dict, optional
        Previous itp dictionary to merge into the result (fields in `itp_prev`
        take precedence).

    Returns
    -------
    dict
        Mapping of section name to column dictionary (for tabular sections) or
        raw string (for ``enddata``).

    Examples
    --------
    data = import_itp("molecule.itp")
    list(data.keys())[:3]  # ['defaults', 'atomtypes', 'atoms']
    merged = import_itp("child.itp", itp_prev=data)  # merge with previous
    """
    if not os.path.isfile(filename):
        raise FileNotFoundError(f"Could not find itp file: {filename}")

    with open(filename, "r", encoding="utf-8") as fh:
        lines = [line.rstrip("\n") for line in fh]

    section_lines: Dict[str, List[str]] = {}
    current_section: Optional[str] = None
    section_counts: Dict[str, int] = {}

    for line in lines:
        stripped = line.strip()
        if not stripped or stripped.startswith(("#", ";")):
            continue
        if stripped.startswith("[") and stripped.endswith("]"):
            name = stripped.strip("[]").strip()
            count = section_counts.get(name, 0) + 1
            section_counts[name] = count
            if count > 1:
                name = f"{name}{count}"
            current_section = name
            section_lines[current_section] = []
            continue
        if current_section:
            section_lines[current_section].append(line)

    itp: Dict[str, Union[Dict, str]] = {}

    # moleculetype
    if "moleculetype" in section_lines:
        cols = ["moleculetype", "nrexcl"]
        itp["moleculetype"] = _parse_table(section_lines["moleculetype"], cols, ["nrexcl"])

    # atoms
    if "atoms" in section_lines:
        atom_cols = [
            "nr",
            "type",
            "resnr",
            "residue",
            "atom",
            "cgnr",
            "charge",
            "mass",
            "typeB",
            "chargeB",
            "massB",
            "comment",
        ]
        numeric = {"nr", "resnr", "cgnr", "charge", "mass", "chargeB", "massB"}
        itp["atoms"] = _parse_table(section_lines["atoms"], atom_cols, numeric)

    # atomtypes
    if "atomtypes" in section_lines:
        first_data = next((ln for ln in section_lines["atomtypes"] if _strip_comment(ln)), "")
        atomtype_cols, numeric_cols = _choose_atomtype_columns(first_data)
        itp["atomtypes"] = _parse_table(section_lines["atomtypes"], atomtype_cols, numeric_cols)
        # Normalize: expose 'type' consistently when only 'name' exists
        if "type" not in itp["atomtypes"] and "name" in itp["atomtypes"]:
            itp["atomtypes"]["type"] = list(itp["atomtypes"]["name"])

    # bondtypes / bonds
    bond_cols = ["ai", "aj", "funct", "c0", "c1", "c2", "c3"]
    bond_numeric = set(bond_cols)
    if "bondtypes" in section_lines:
        itp["bondtypes"] = _parse_table(section_lines["bondtypes"], bond_cols, bond_numeric)
    if "bonds" in section_lines:
        itp["bonds"] = _parse_table(section_lines["bonds"], bond_cols, bond_numeric)

    # angletypes / angles
    angle_cols = ["ai", "aj", "ak", "funct", "c0", "c1", "c2", "c3"]
    angle_numeric = set(angle_cols)
    if "angletypes" in section_lines:
        itp["angletypes"] = _parse_table(section_lines["angletypes"], angle_cols, angle_numeric)
    if "angles" in section_lines:
        itp["angles"] = _parse_table(section_lines["angles"], angle_cols, angle_numeric)

    # pairtypes / pairs
    pair_cols = ["ai", "aj", "funct", "c0", "c1", "c2", "c3"]
    pair_numeric = set(pair_cols)
    if "pairtypes" in section_lines:
        itp["pairtypes"] = _parse_table(section_lines["pairtypes"], pair_cols, pair_numeric)
    if "pairs" in section_lines:
        itp["pairs"] = _parse_table(section_lines["pairs"], pair_cols, pair_numeric)

    # exclusions
    if "exclusions" in section_lines:
        exc_cols = ["ai", "aj", "ak", "funct"]
        itp["exclusions"] = _parse_table(section_lines["exclusions"], exc_cols, exc_cols)

    # dihedral blocks
    dihedral_cols = ["ai", "aj", "ak", "al", "funct", "c0", "c1", "c2", "c3", "c4", "c5"]
    if "dihedrals" in section_lines:
        itp["dihedrals"] = _parse_table(section_lines["dihedrals"], dihedral_cols, dihedral_cols)
    if "dihedrals2" in section_lines:
        itp["dihedrals2"] = _parse_table(section_lines["dihedrals2"], dihedral_cols, dihedral_cols)
    if "dihedraltypes" in section_lines:
        itp["dihedraltypes"] = _parse_table(section_lines["dihedraltypes"], dihedral_cols, dihedral_cols)
    if "dihedraltypes2" in section_lines:
        itp["dihedraltypes2"] = _parse_table(section_lines["dihedraltypes2"], dihedral_cols, dihedral_cols)

    # impropers
    if "impropers" in section_lines:
        itp["impropers"] = _parse_table(section_lines["impropers"], dihedral_cols, dihedral_cols)

    # position restraints and trailing text
    if "position_restraints" in section_lines:
        itp["position_restraints"] = _parse_table(
            section_lines["position_restraints"],
            ["ai", "aj", "ak", "al", "funct", "c0", "c1", "c2", "c3", "c4", "c5"],
            dihedral_cols,
        )

    # Preserve raw tail if present
    if "enddata" in section_lines:
        itp["enddata"] = "\n".join(section_lines["enddata"])

    if itp_prev:
        itp.update(itp_prev)

    return itp


# ---------------------------------------------------------------------------
# GAFF / ACPYPE topology import
# ---------------------------------------------------------------------------


def _element_from_str(name: str) -> str:
    """Guess chemical element symbol from an atom type or atom name string.
    Handles GAFF types (c3, hc, oh, ...) and MINFF/CLAYFF types (Sit, Ob, ...)."""
    if not name:
        return ''
    s = name.strip().lower()
    # Two-letter mineral / ion checks first (order matters)
    if s.startswith('sit') or s.startswith('si'): return 'Si'
    if s.startswith('ale') or s.startswith('alt') or s.startswith('al'): return 'Al'
    if s.startswith('fee') or s.startswith('fet') or s.startswith('fe'): return 'Fe'
    if s.startswith('mg'): return 'Mg'
    if s.startswith('mn'): return 'Mn'
    if s.startswith('ti'): return 'Ti'
    if s.startswith('li'): return 'Li'
    if s.startswith('na'): return 'Na'
    if s.startswith('sod'): return 'Na'
    if s.startswith('ca'): return 'Ca'
    if s.startswith('cal'): return 'Ca'
    if s.startswith('cl'): return 'Cl'
    if s.startswith('cla'): return 'Cl'
    if s.startswith('pot'): return 'K'
    if s.startswith('k'):   return 'K'
    # Single-letter + GAFF organic types
    if s.startswith('c'):  return 'C'
    if s.startswith('n'):  return 'N'
    if s.startswith('o') or s.startswith('ow') or s.startswith('ob'): return 'O'
    if s.startswith('p'):  return 'P'
    if s.startswith('s'):  return 'S'
    if s.startswith('f'):  return 'F'
    if s.startswith('h'):  return 'H'
    # Fallback: capitalize the raw name
    return name.strip().capitalize()


def import_gaff_top(
    top_path: str,
    itp_path: Optional[str] = None,
) -> tuple:
    """
    Import an ACPYPE-generated GROMACS topology into atomipy-native format.

    ACPYPE produces two files:
      organic_GMX.top  — thin wrapper: [ defaults ], #include "organic_GMX.itp", [ system ], [ molecules ]
      organic_GMX.itp  — all FF data: [ atomtypes ], [ moleculetype ], [ atoms ], [ bonds ], [ pairs ],
                                       [ angles ], [ dihedrals ], [ dihedrals2 ], [ impropers ]

    This function reads both and returns:
      atoms   — list of atomipy atom-dicts (index, type, fftype, resname, molid, charge, mass)
                NOTE: x, y, z are set to 0.0 — call import_gro_coords() to fill in coordinates.
      itp     — itp-dict with all FF sections, plus '_source_itp' key for the .itp file path.

    Parameters
    ----------
    top_path : str   Path to organic_GMX.top
    itp_path : str   Path to organic_GMX.itp (auto-detected from top if omitted)

    Returns
    -------
    atoms : list of dicts
    itp   : dict
    """
    if not os.path.isfile(top_path):
        raise FileNotFoundError(f"import_gaff_top: .top not found: {top_path}")

    # Auto-detect the .itp path from the #include line in the .top
    if itp_path is None:
        top_dir = os.path.dirname(os.path.abspath(top_path))
        with open(top_path, 'r', encoding='utf-8') as fh:
            for line in fh:
                raw = line.strip()
                # Use the raw line — _strip_comment removes '#' so #include becomes invisible
                if raw.startswith('#include'):
                    # Extract quoted filename: #include "organic_GMX.itp"
                    parts = raw.split('"')
                    if len(parts) >= 2:
                        inc_name = parts[1]
                        candidate = os.path.join(top_dir, inc_name)
                        if os.path.isfile(candidate) and candidate.endswith('.itp'):
                            itp_path = candidate
                            break

    # Parse the itp (or the top itself if no separate itp)
    src_path = itp_path if (itp_path and os.path.isfile(itp_path)) else top_path
    itp = import_itp(src_path)

    # If the .itp didn't have atomtypes, try parsing the top directly for [ defaults ]
    if not itp.get('atomtypes'):
        itp_from_top = import_itp(top_path)
        if itp_from_top.get('atomtypes'):
            itp['atomtypes'] = itp_from_top['atomtypes']

    itp['_source_itp'] = src_path

    # Build atomipy atoms list from [ atoms ] section
    atoms_sec = itp.get('atoms', {})
    if not isinstance(atoms_sec, dict):
        atoms_sec = {}
    n = _itp_col_len(atoms_sec)

    if n == 0:
        raise ValueError(
            f"No atoms found in GROMACS topology/itp at: {src_path}. "
            f"Please ensure the file exists and contains a valid [ atoms ] section."
        )

    # Helper to safely fetch a value from a section column
    def _col(col_name, i, default=''):
        lst = atoms_sec.get(col_name, [])
        if i < len(lst):
            val = lst[i]
            if val is not None and val != '':
                return val
        return default

    atoms: List[Dict[str, Any]] = []
    for i in range(n):
        nr      = _safe_int(_col('nr', i, ''), i + 1)
        atype   = str(_col('type', i, 'X')).strip()
        resnr   = _safe_int(_col('resnr', i, ''), 1)
        
        # Proper fallback: default to empty string, fallback to resname/MOL if empty
        resname = (str(_col('residue', i, '')).strip() or 
                   str(_col('resname', i, '')).strip() or 
                   'MOL')
                   
        atname  = str(_col('atom', i, '')).strip() or atype
        charge  = _safe_float(_col('charge', i, ''), 0.0)
        mass    = _safe_float(_col('mass', i, ''), 0.0)

        # Infer element symbol from atom type string
        el = _element_from_str(atype) or _element_from_str(atname) or ''

        atom = {
            'index':   nr,
            'type':    atype,
            'fftype':  atype,
            'name':    atname,
            'resname': resname,
            'molid':   resnr,
            'charge':  charge,
            'mass':    mass,
            'x':       0.0,
            'y':       0.0,
            'z':       0.0,
            'element': el,
            'neigh':   [],
            'bonds':   [],
            'angles':  [],
        }
        atoms.append(atom)

    return atoms, itp


def import_gro_coords(gro_path: str, atoms: Optional[list] = None) -> list:
    """
    Read coordinates from a GROMACS .gro file and optionally fill them into
    an existing atomipy atom list.

    Parameters
    ----------
    gro_path : str
        Path to the .gro coordinate file.
    atoms : list of dicts, optional
        If provided, fills x/y/z (in Å) into the existing dicts in-place.
        If None, returns a new minimal atom list (no FF info).

    Returns
    -------
    atoms : list of dicts  (the same list that was passed in, or a new one)
    """
    from .import_conf import gro as _read_gro
    gro_atoms, gro_box = _read_gro(gro_path)

    if atoms is None:
        return gro_atoms

    # Fill coordinates into the provided atoms list
    for i, ga in enumerate(gro_atoms):
        if i < len(atoms):
            atoms[i]['x'] = ga['x']
            atoms[i]['y'] = ga['y']
            atoms[i]['z'] = ga['z']
        # else: extra GRO atoms beyond atom list length — silently skip

    return atoms

