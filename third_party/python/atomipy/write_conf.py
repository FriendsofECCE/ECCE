import os
import contextlib
from .cell_utils import Box_dim2Cell, Cell2Box_dim, normalize_box
from ._provenance import provenance_string


@contextlib.contextmanager
def _smart_open(f, mode='w'):
    """Context manager to handle both file paths and file-like objects."""
    if hasattr(f, 'write'):
        yield f
    else:
        with open(f, mode) as opened_f:
            yield opened_f


def _infer_element(atom):
    """Infer element symbol from atom attributes."""
    # Priority: element > fftype > type
    for key in ['element', 'fftype', 'type']:
        val = atom.get(key)
        if not val or not isinstance(val, str):
            continue
        
        # Strip digits and extra chars
        import re
        m = re.match(r'([A-Za-z]+)', val.strip())
        if not m:
            continue
            
        symbol = m.group(1)
        
        # Special common labels in mineralogy/MD
        # e.g. HW, OW, MW (water/minerals), SI, AL, MG, FE, CA
        if len(symbol) >= 2:
            s_up = symbol.upper()
            if s_up.startswith('HW'): return 'H'
            if s_up.startswith('OW'): return 'O'
            if s_up in ['SI', 'AL', 'MG', 'FE', 'CA', 'CL', 'NA', 'LI', 'ZN']:
                return s_up[0] + s_up[1].lower()
            
        # Default: first character for common organic elements, 
        # or first 2 chars for others if applicable
        if symbol[0].upper() in 'HCONSP':
            return symbol[0].upper()
        
        return symbol[:2] if len(symbol) >= 2 else symbol.upper()
    
    return 'X'


def pdb(atoms, Box, file_path, write_conect=False, write_element=True, reindex_residues=False):
    """Write atoms and Cell dimensions to a PDB file.

    Args:
       atoms: list of atom dictionaries.
       Box: a 1x6 or 1x9 list representing Cell dimensions (in Angstroms), either as 
            a Cell variable having Cell parameters array [a, b, c, alpha, beta, gamma], or as 
            a Box_dim variable having Box dimensions [lx, ly, lz, 0, 0, xy, 0, xz, yz] for triclinic cells.
           Note that for orthogonal boxes Cell = Box_dim.
       file_path: output filepath.
       write_conect: bool, optional. If True, write CONECT records from 'bonds' or 'neigh' fields (default: False).
       write_element: bool, optional. If True, write element symbols and formal charges (cols 77-80) (default: True).

    Examples
    --------
    import atomipy as ap
    atoms, Box_dim = ap.import_gro("structure.gro")
    ap.write_conf.pdb(atoms, Box_dim, "out.pdb")
    """
    if not atoms:
        return

    # Normalize Box to Cell parameters
    _, Cell = normalize_box(Box)
    
    with _smart_open(file_path, 'w') as f:
        # Add REMARK lines
        f.write(f"REMARK    {provenance_string()}\n")
        f.write("REMARK    THIS IS A SIMULATION BOX\n")
        
        if Cell is not None:
            # Write CRYST1 record; formatted width according to PDB spec
            a, b, c, alpha, beta, gamma = Cell
            f.write(f"CRYST1{a:9.3f}{b:9.3f}{c:9.3f}{alpha:7.2f}{beta:7.2f}{gamma:7.2f} P 1           1\n")
            
            # Write SCALE1, SCALE2, SCALE3 records (orthogonal-to-fractional coordinates transformation)
            import numpy as np
            alpha_rad, beta_rad, gamma_rad = np.radians([alpha, beta, gamma])
            ca, cb, cg = np.cos([alpha_rad, beta_rad, gamma_rad])
            sg = np.sin(gamma_rad)
            v = np.sqrt(1 - ca**2 - cb**2 - cg**2 + 2*ca*cb*cg)
            
            s11, s12, s13 = 1/a, -cg / (a*sg), (ca*cg - cb) / (a*v*sg)
            s21, s22, s23 = 0.0, 1 / (b*sg), (cb*cg - ca) / (b*v*sg)
            s31, s32, s33 = 0.0, 0.0, sg / (c*v)
            
            f.write(f"SCALE1    {s11:10.6f}{s12:10.6f}{s13:10.6f}     {0.0:10.5f}\n")
            f.write(f"SCALE2    {s21:10.6f}{s22:10.6f}{s23:10.6f}     {0.0:10.5f}\n")
            f.write(f"SCALE3    {s31:10.6f}{s32:10.6f}{s33:10.6f}     {0.0:10.5f}\n")
        
        # Preserve the true molecule id across a PDB round-trip. PDB has no molid column,
        # and the OpenMM-safe per-atom residue reindexing (below) would otherwise turn molid
        # per-atom on re-import. We stash the real molid in the segID field (cols 73-76) and
        # flag it with this REMARK so import_pdb can restore molid regardless of res_seq.
        f.write("REMARK   ATOMIPY molid stored in segID (cols 73-76)\n")

        # Add MODEL record
        f.write("MODEL        1\n")

        # By DEFAULT the residue-number column mirrors molid (like the .gro writer), so an
        # exported mineral shows one residue == one molecule. Opt in with
        # reindex_residues=True to instead give each atom its own residue number — needed
        # only when the PDB will be read by OpenMM's PDBFile, which drops atoms that share a
        # NAME inside the same residue (a mineral has many identical Alo/Ob/... names). The
        # standard OpenMM path here loads .gro, not this PDB, so the default stays clean.
        # (The true molid is also stashed in segID/REMARK below, so a round-trip is exact
        # either way.) Only reindex when asked AND it's the single-residue repeating-name case.
        _res_ids = {(a.get('resnum') if a.get('resnum') is not None else a.get('molid', 1)) for a in atoms}
        _names_all = [str(a.get('fftype') or a.get('type') or a.get('element') or '') for a in atoms]
        _reindex_residues = reindex_residues and (len(_res_ids) <= 1 and len(set(_names_all)) < len(_names_all))

        for i, atom in enumerate(atoms, start=1):
            # Format the ATOM record according to PDB specification
            index = atom.get('index', i)
            
            # Get atom name from 'fftype' field, fall back to 'type', then 'element'
            # PDB atom names (cols 13-16). Rules: up to 4 chars.
            # - Alignment is tricky: usually left-aligned for elements <= 2 chars,
            #   right-aligned (?) or starting col 13 for longer names.
            #   Let's try space-padding and left-alignment for simplicity first.
            raw_atomname = atom.get('fftype')
            if raw_atomname is None:
                raw_atomname = atom.get('type')
            if raw_atomname is None:
                raw_atomname = atom.get('element')
            if raw_atomname is None or str(raw_atomname).strip() == '':
                raw_atomname = 'X'
            else:
                raw_atomname = str(raw_atomname)[:4]
                
            if len(raw_atomname) == 1: # Single char element, place in col 14
                pdb_atomname = f" {raw_atomname}  "
            else: # 2+ chars, start in col 13
                pdb_atomname = f"{raw_atomname:<4}"
            
            alt_loc = ' ' # Column 17: Alternate location indicator
            
            # Get residue name: use resname if available, otherwise UNK
            # PDB residue names (cols 18-20). 3 chars, right-aligned.
            raw_resname = atom.get('resname')
            if raw_resname is None or str(raw_resname).strip() == '':
                raw_resname = 'UNK'
            else:
                raw_resname = str(raw_resname)
            pdb_resname = f"{raw_resname[:3]:>3}"
            
            chain_id = atom.get('chain_id', 'A') # Column 22: Chain identifier
            res_seq = atom.get('resnum')
            if res_seq is None:
                res_seq = atom.get('molid', 1)

            # Only reindex into per-atom residues for the single-residue / repeating-name
            # case detected above (keeps OpenMM from dropping duplicate-named atoms), so a
            # legitimate residue numbered 1 is no longer split apart.
            if _reindex_residues:
                res_seq = i

            res_seq = (res_seq - 1) % 9999 + 1 # Columns 23-26: max 4 digits (1 to 9999)
            icode = atom.get('icode', ' ') # Column 27: Code for insertion of residues
            
            x = atom.get('x', 0.0)
            y = atom.get('y', 0.0)
            z = atom.get('z', 0.0)

            # Occupancy (cols 55-60), default 1.00
            occupancy_val = atom.get('occupancy', 1.00)
            try:
                occupancy_val = float(occupancy_val)
            except (ValueError, TypeError):
                occupancy_val = 1.00

            # Temperature Factor (cols 61-66), default to partial charge if available
            temp_factor_val = atom.get('charge')
            if temp_factor_val is None:
                temp_factor_val = atom.get('temp_factor', 0.00)
            
            try:
                temp_factor_val = float(temp_factor_val)
            except (ValueError, TypeError):
                temp_factor_val = 0.00

            if write_element:
                # Element symbol (cols 77-78), right-justified, proper capitalization.
                raw_element = atom.get('element')
                if not raw_element:
                    raw_element = _infer_element(atom)

                element_str = str(raw_element).strip()
                if element_str:
                    # Proper PDB element formatting: right-justified in 2 columns
                    element_symbol_pdb = f"{element_str.upper():>2}"
                else:
                    element_symbol_pdb = " X"

                # Charge (cols 79-80), right-justified. Default to empty if not found.
                # PDB standard mandates formal charges (e.g. +2, -1), not partial forcefield charges.
                from .charge import get_formal_charge
                element_val = atom.get('element')
                if element_val is None:
                    element_val = atom.get('type')
                if element_val is None:
                    element_val = ''
                formal_chg = get_formal_charge(element_val)
                
                if formal_chg != 0:
                    # Typically formatted like " 2" or "-1" for right-justified charge output
                    charge_str = f"{formal_chg:>2.0f}"
                else:
                    charge_str = "  "
            else:
                element_symbol_pdb = "  "
                charge_str = "  "
            
            # Construct the ATOM line using f-string formatting for precise columns
            # ATOM record fields and their typical column formatting:
            # Record name (ATOM/HETATM)  1-6
            # Atom serial number         7-11  (index:5d)
            # Space                      12
            # Atom name                  13-16 (pdb_atomname: <4 or similar)
            # Alt loc indicator          17    (alt_loc:1s)
            # Residue name               18-20 (pdb_resname:>3s)
            # Space                      21
            # Chain ID                   22    (chain_id:1s)
            # Residue sequence number    23-26 (res_seq:4d)
            # Insertion code             27    (icode:1s)
            # 3 spaces                   28-30
            # X coordinate               31-38 (x:8.3f)
            # Y coordinate               39-46 (y:8.3f)
            # Z coordinate               47-54 (z:8.3f)
            # Occupancy                  55-60 (occupancy_val:6.2f)
            # Temperature factor         61-66 (temp_factor_val:6.2f)
            # Spaces                     67-72 ("      ")
            # Seg ID                     73-76 (_seg_str:4s = true molid, carries it round-trip)
            # Element symbol             77-78 (element_symbol_pdb:>2s)
            # Charge                     79-80 (charge_str:>2s)
            # segID holds the real molid (capped to 4 cols like res_seq), so a round-trip
            # preserves molid even when res_seq is reindexed per-atom for OpenMM.
            _seg_molid = ((int(atom.get('molid', 1)) - 1) % 9999) + 1
            _seg_str = f"{_seg_molid:4d}"
            f.write(f"ATOM  {index:5d} {pdb_atomname}{alt_loc}{pdb_resname} {chain_id}{res_seq:4d}{icode}   {x:8.3f}{y:8.3f}{z:8.3f}{occupancy_val:6.2f}{temp_factor_val:6.2f}      {_seg_str}{element_symbol_pdb}{charge_str}\n")
        
        # Add ENDMDL record
        f.write("ENDMDL\n")

        if write_conect:
            # Check if topological data exists, otherwise calculate it
            has_bonds = any('bonds' in a and a['bonds'] for a in atoms)
            has_neigh = any('neigh' in a and a['neigh'] for a in atoms)
            
            if not has_bonds and not has_neigh:
                from .bond_angle import bond_angle
                # Assign default MINFF-compliant bonding using bond_angle
                atoms_from_bond, _, _ = bond_angle(atoms, Box)
                # bond_angle updates in place, but we re-assign to be safe
                atoms = atoms_from_bond

            # Build index mapping for CONECT 
            # (map 0-based atom list indices to PDB 1-based serials)
            index_map = []
            for i, atom in enumerate(atoms, start=1):
                index_map.append(atom.get('index', i))
                
            for i, atom in enumerate(atoms):
                bonds_data = atom.get('bonds', [])
                neigh_data = atom.get('neigh', [])
                
                # Extract neighbor indices. Fall back to neigh if bonds is empty/non-existent
                if bonds_data:
                    # bonds is usually [(j, dist), ...]
                    neighbors = [b[0] for b in bonds_data]
                else:
                    neighbors = neigh_data
                    
                if not neighbors:
                    continue
                    
                source_serial = index_map[i]
                target_serials = [index_map[n] for n in neighbors if n < len(index_map)]
                
                # Chunk target_serials into groups of 4 (PDB spec)
                for k in range(0, len(target_serials), 4):
                    chunk = target_serials[k:k+4]
                    # Format: CONECT  idx1  idx2  idx3  idx4  idx5
                    line = f"CONECT{source_serial:>5}"
                    for t in chunk:
                        line += f"{t:>5}"
                    line += "\n"
                    f.write(line)
        
        f.write("END\n")


def gro(atoms, Box, file_path):
    """Write atoms and Box dimensions to a Gromacs .gro file.

    Gromacs .gro files store coordinates in nanometers (nm), but atomipy uses Angstroms (Å).
    This function automatically converts the coordinates and Box dimensions from Å to nm.

    Args:
       atoms: list of atom dictionaries with coordinates in Angstroms.
       Box: a 1x6 or 1x9 list representing Cell dimensions in Angstroms, either as 
            a Cell variable having Cell parameters array [a, b, c, alpha, beta, gamma], or as 
            a Box_dim variable having Box dimensions [lx, ly, lz, 0, 0, xy, 0, xz, yz] for triclinic cells.
            Note that for orthogonal boxes Cell = Box_dim.
       file_path: output filepath.

    Examples
    --------
    import atomipy as ap
    atoms, Box_dim = ap.import_gro("structure.gro")
    ap.write_conf.gro(atoms, Box_dim, "out.gro")
    """
    if not atoms:
        return

    # Conversion factor from Angstroms to nm
    angstrom_to_nm = 0.1

    Box_dim, _ = normalize_box(Box)
    with _smart_open(file_path, 'w') as f:
        # Write title
        f.write(f"{provenance_string()}\n")
        # Write number of atoms
        f.write(f"{len(atoms)}\n")
        # Write each atom line with GRO format: residue number (5 chars), residue name (5 chars), atom name (5 chars), atom number (5 chars), x (8.3f), y (8.3f), z (8.3f) and optionally velocities vx, vy, vz (8.4f each)
        for i, atom in enumerate(atoms, start=1):
            # Residue number = molecule id (5-char field wraps at 100000, GROMACS convention).
            resnum = atom.get('molid', 1) % 100000  # Use molecule ID as residue number in GRO format
            resname = atom.get('resname', 'UNK')
            
            # Use 'type' field for atom name, fall back to element or first character of resname
            atomname = atom.get('type')
            if atomname is None:
                atomname = atom.get('element')
            if atomname is None:
                atomname = resname[0] if resname else 'X'
            atomname = str(atomname)

            # Atom serial is POSITIONAL in a .gro (5-char field, wraps at 100000). Always
            # write a sequential number so subset/reordered atom lists (e.g. solvated boxes
            # whose 'index' carries the replicated template's sparse ids) can't leave gaps
            # or duplicates.
            index = i % 100000

            # Convert coordinates from Angstroms to nm for .gro format
            x = atom.get('x', 0.0) * angstrom_to_nm
            y = atom.get('y', 0.0) * angstrom_to_nm
            z = atom.get('z', 0.0) * angstrom_to_nm
            
            # Check if velocities are present and convert them too
            vx = atom.get('vx', None)
            vy = atom.get('vy', None)
            vz = atom.get('vz', None)
            
            if vx is not None and vy is not None and vz is not None:
                # Convert velocities from Å/ps to nm/ps
                vx *= angstrom_to_nm
                vy *= angstrom_to_nm
                vz *= angstrom_to_nm
                line = f"{resnum:5d}{resname[:5]:<5s}{atomname:>5s}{index:5d}{x:8.3f}{y:8.3f}{z:8.3f}{vx:8.4f}{vy:8.4f}{vz:8.4f}\n"
            else:
                line = f"{resnum:5d}{resname[:5]:<5s}{atomname:>5s}{index:5d}{x:8.3f}{y:8.3f}{z:8.3f}\n"
                
            f.write(line)
            
        if Box_dim is not None:
            # Convert Box dimensions from Angstroms to nm
            Box_dim_nm = [val * angstrom_to_nm for val in Box_dim]
            
            # Handle different Box_dim formats
            if len(Box_dim) == 3:  # Orthogonal Box with just 3 dimensions
                # For orthogonal boxes, GROMACS expects just 3 values
                Box_str = "".join(f" {val:10.5f}" for val in Box_dim_nm[:3])
            elif len(Box_dim) == 9:  # Triclinic Box with 9 dimensions
                # For triclinic boxes, GROMACS expects all 9 values
                Box_str = "".join(f" {val:10.5f}" for val in Box_dim_nm)
            else:
                # Handle unexpected Box_dim length
                print(f"Warning: Box_dim has unexpected length {len(Box_dim)}. Expected 3 or 9.")
                # Default to using whatever was provided
                Box_str = "".join(f" {val:10.5f}" for val in Box_dim_nm)
                
            f.write(Box_str + "\n")
        else:
            f.write("\n")


def xyz(atoms, Box=None, file_path=None):
    """Write atoms and Cell dimensions to an XYZ file.

    XYZ format has the following structure:
    - First line: number of atoms
    - Second line: comment line with Box dimensions (if provided). Box dimensions are a 1x6 or 1x9 list representing Cell dimensions (in Angstroms), either as 
            a Cell variable having Cell parameters array [a, b, c, alpha, beta, gamma], or as 
            a Box_dim variable having Box dimensions [lx, ly, lz, 0, 0, xy, 0, xz, yz] for triclinic cells.
            Note that for orthogonal boxes Cell = Box_dim.
    - Remaining lines: atom entries in format: Element X Y Z

    Args:
       atoms: list of atom dictionaries.
       Box: Optional 1x6 list [a, b, c, alpha, beta, gamma] or Box_dim (1x3 or 1x9 list).
           Default is to write Cell parameters if provided.
       file_path: output filepath.

    Examples
    --------
    import atomipy as ap
    atoms, Cell = ap.import_xyz("structure.xyz")
    ap.write_conf.xyz(atoms, Cell, "out.xyz")
    """
    if not atoms:
        return

    # Normalize Box to Cell parameters
    Cell = None
    if Box is not None:
        _, Cell = normalize_box(Box)


    with _smart_open(file_path, 'w') as f:
        # Write number of atoms on first line
        f.write(f"{len(atoms)}\n")
        
        # Write comment line with Box dimensions if available
        if Cell is not None:
            if len(Cell) == 6:  # Cell parameters format
                f.write(f"#    {Cell[0]:.5f}   {Cell[1]:.5f}   {Cell[2]:.5f}   {Cell[3]:.5f}   {Cell[4]:.5f}   {Cell[5]:.5f}  | {provenance_string()}\n")
            elif len(Cell) == 3:  # Orthogonal Box
                f.write(f"#    {Cell[0]:.5f}   {Cell[1]:.5f}   {Cell[2]:.5f}  | {provenance_string()}\n")
            elif len(Cell) == 9:  # Triclinic Box
                # Write all 9 values
                Box_str = '   '.join(f"{val:.5f}" for val in Cell)
                f.write(f"#    {Box_str}  | {provenance_string()}\n")
            else:
                # No recognized format, just write a placeholder comment
                f.write(f"# {provenance_string()}\n")
        else:
            # No Cell information, just write a placeholder comment
            f.write(f"# {provenance_string()}\n")
            
        # Write atom entries
        for atom in atoms:
            # Get atom type name instead of element
            atom_type = atom.get('type')
            if atom_type is None:
                atom_type = atom.get('element')
            if atom_type is None:
                atom_type = 'X'
            atom_type = str(atom_type)
            
            # Get coordinates
            x = atom.get('x', 0.0)
            y = atom.get('y', 0.0)
            z = atom.get('z', 0.0)
            
            # Write atom entry: AtomType X Y Z with right-aligned coordinates and extra spacing
            f.write(f"{atom_type:<10} {x:>12.5f}    {y:>12.5f}    {z:>12.5f}\n")

def write_pdb(atoms, Box, filename=None, write_conect=True, write_element=True, reindex_residues=False):
    """
    Wrapper for pdb() for backward compatibility.

    reindex_residues=False (default) writes the residue-number column from molid, so one
    molecule == one residue (matching the .gro writer). Set True only when the PDB is to be
    loaded by OpenMM's PDBFile, which needs each identically-named mineral atom in its own
    residue.
    """
    pdb(atoms, Box, filename, write_conect=write_conect, write_element=write_element,
        reindex_residues=reindex_residues)


def cif(atoms, Box, file_path=None, title='Generated by atomipy'):
    """Write atoms and Cell dimensions to a CIF file.

    Writes a standard small-molecule CIF file with cell parameters,
    space group (P 1), and atom site data in both fractional and
    Cartesian formats.  When atoms carry Debye-Waller fields produced
    by :func:`atomipy.analysis.condense_supercell` (``U_iso``,
    ``U_11`` … ``U_23``) those are written as additional CIF loops.

    Parameters
    ----------
    atoms : list of dict
        List of atom dictionaries (standard atomipy format).
    Box : list
        Cell or Box_dim: 1x3, 1x6, or 1x9 list.
    file_path : str
        Output filepath (should end in .cif).
    title : str, optional
        Data block title. Default is 'Generated by atomipy'.

    Notes
    -----
    - If atoms already have 'xfrac', 'yfrac', 'zfrac' fields those
      are written directly.  Otherwise, Cartesian coordinates are
      converted to fractional using the transform module.
    - Space group is always written as 'P 1' (no symmetry reduction
      is attempted — the file contains the full unit cell).
    - When ``U_iso`` / ``B_iso`` are present they are written as
      ``_atom_site_U_iso_or_equiv`` in the main atom loop.
    - When ``U_11`` … ``U_23`` are present a separate
      ``_atom_site_aniso_*`` loop is appended.

    Examples
    --------
    import atomipy as ap
    atoms, Cell = ap.import_pdb("structure.pdb")
    ap.write_cif(atoms, Cell, "output.cif")
    """
    if not atoms:
        return

    from .transform import cartesian_to_fractional

    # Normalize Box to Cell parameters
    _, Cell = normalize_box(Box)

    if Cell is None:
        raise ValueError("CIF export requires a simulation box/cell.")

    a, b, c, alpha, beta, gamma = Cell

    # Ensure atoms have fractional coordinates
    has_frac = all('xfrac' in atom for atom in atoms)
    if not has_frac:
        cartesian_to_fractional(atoms=atoms, Box=Cell, add_to_atoms=True)

    # Detect optional ADP fields
    has_u_iso  = any('U_iso' in atom or 'B_iso' in atom for atom in atoms)
    has_aniso  = any('U_11' in atom for atom in atoms)

    # Make title CIF-safe (no spaces, replace with underscores)
    data_name = title.replace(' ', '_').replace("'", '').replace('"', '')

    # Build unique labels (CIF requires unique labels per site)
    label_counts = {}
    labels = []
    for atom in atoms:
        base = atom.get('type', atom.get('element', 'X'))
        if base not in label_counts:
            label_counts[base] = 0
        label_counts[base] += 1
        labels.append(f"{base}{label_counts[base]}")

    with _smart_open(file_path, 'w') as f:
        f.write(f"data_{data_name}\n")
        f.write(f"# {provenance_string()}\n")
        f.write(f"#\n")

        # Cell parameters
        f.write(f"_cell_length_a    {a:.5f}\n")
        f.write(f"_cell_length_b    {b:.5f}\n")
        f.write(f"_cell_length_c    {c:.5f}\n")
        f.write(f"_cell_angle_alpha {alpha:.4f}\n")
        f.write(f"_cell_angle_beta  {beta:.4f}\n")
        f.write(f"_cell_angle_gamma {gamma:.4f}\n")
        f.write(f"\n")

        # Space group (always P 1 — no symmetry reduction)
        f.write(f"_symmetry_space_group_name_H-M  'P 1'\n")
        f.write(f"_symmetry_Int_Tables_number      1\n")
        f.write(f"\n")

        # ------------------------------------------------------------------ #
        # Main atom-site loop                                                  #
        # ------------------------------------------------------------------ #
        f.write("loop_\n")
        f.write("  _atom_site_label\n")
        f.write("  _atom_site_type_symbol\n")
        f.write("  _atom_site_fract_x\n")
        f.write("  _atom_site_fract_y\n")
        f.write("  _atom_site_fract_z\n")
        f.write("  _atom_site_Cartn_x\n")
        f.write("  _atom_site_Cartn_y\n")
        f.write("  _atom_site_Cartn_z\n")
        f.write("  _atom_site_occupancy\n")
        if has_u_iso:
            f.write("  _atom_site_U_iso_or_equiv\n")
            f.write("  _atom_site_B_iso_or_equiv\n")
        if has_aniso:
            f.write("  _atom_site_adp_type\n")

        for i, atom in enumerate(atoms):
            label   = labels[i]
            element = atom.get('element', atom.get('type', 'X'))
            if element:
                element = element[0].upper() + element[1:].lower() if len(element) > 1 else element.upper()

            fx  = atom.get('xfrac', 0.0)
            fy  = atom.get('yfrac', 0.0)
            fz  = atom.get('zfrac', 0.0)
            cx  = atom.get('x', 0.0)
            cy  = atom.get('y', 0.0)
            cz  = atom.get('z', 0.0)
            occ = atom.get('occupancy', 1.0)

            line = (f"  {label:<8s} {element:<4s} "
                    f"{fx:10.6f} {fy:10.6f} {fz:10.6f}  "
                    f"{cx:10.5f} {cy:10.5f} {cz:10.5f}  "
                    f"{occ:.4f}")
            if has_u_iso:
                u_iso = atom.get('U_iso', atom.get('B_iso', 0.0) / (8.0 * 3.14159265**2))
                b_iso = atom.get('B_iso', u_iso * 8.0 * 3.14159265**2)
                line += f"  {u_iso:.6f}  {b_iso:.5f}"
            if has_aniso:
                line += "  Uani" if 'U_11' in atom else "  Uiso"
            f.write(line + "\n")

        # ------------------------------------------------------------------ #
        # Anisotropic ADP loop (only when U_ij data are present)              #
        # ------------------------------------------------------------------ #
        if has_aniso:
            f.write("\n")
            f.write("loop_\n")
            f.write("  _atom_site_aniso_label\n")
            f.write("  _atom_site_aniso_U_11\n")
            f.write("  _atom_site_aniso_U_22\n")
            f.write("  _atom_site_aniso_U_33\n")
            f.write("  _atom_site_aniso_U_12\n")
            f.write("  _atom_site_aniso_U_13\n")
            f.write("  _atom_site_aniso_U_23\n")

            for i, atom in enumerate(atoms):
                if 'U_11' not in atom:
                    continue
                label = labels[i]
                f.write(f"  {label:<8s} "
                        f"{atom['U_11']:10.6f} {atom['U_22']:10.6f} {atom['U_33']:10.6f}  "
                        f"{atom['U_12']:10.6f} {atom['U_13']:10.6f} {atom['U_23']:10.6f}\n")

    print(f"Wrote {len(atoms)} atoms to CIF file: {file_path}")


def cjson(atoms, Box=None, file_path=None, name=None):
    """Write atoms (and an optional unit cell) to a Chemical JSON (.cjson) file.

    Chemical JSON is the open Avogadro2 / Open Chemistry format. This writer
    emits 3D coordinates (Angstroms), atomic numbers, bonds (with order),
    formal charges, and partial charges when present on the atoms.

    Args:
        atoms: list of atom dictionaries.
        Box: optional Cell [a,b,c,alpha,beta,gamma] or Box_dim; written as a unitCell.
        file_path: output path (or file-like object).
        name: optional molecule name (defaults to first atom's 'name'/'resname').
    """
    import json as _json
    from .import_conf import SYMBOL_TO_NUMBER

    if not atoms:
        return

    n = len(atoms)
    coords = []
    numbers = []
    formal = []
    partial = []
    has_partial = False
    for a in atoms:
        coords.extend([round(float(a.get('x', 0.0)), 6),
                       round(float(a.get('y', 0.0)), 6),
                       round(float(a.get('z', 0.0)), 6)])
        z = a.get('atomic_number')
        if not z:
            sym = (a.get('element') or _infer_element(a) or 'X')
            z = SYMBOL_TO_NUMBER.get(str(sym).upper(), 0)
        numbers.append(int(z))
        formal.append(int(a.get('formal_charge', 0) or 0))
        # 'charge' may be a numeric partial charge (organics) or a PDB-style
        # string ('', '1+'); only treat numeric values as partial charges.
        c = a.get('charge', '')
        try:
            partial.append(round(float(c), 6)); has_partial = True
        except (TypeError, ValueError):
            partial.append(0.0)

    # Bonds: dedup undirected pairs from each atom's 'bonds'/'neigh'.
    seen = set()
    conn = []
    order = []
    for i, a in enumerate(atoms):
        for nb in a.get('bonds', []) or []:
            if isinstance(nb, (tuple, list)):
                j1, bo = int(nb[0]), (nb[1] if len(nb) > 1 else 1)
            else:
                j1, bo = int(nb), 1
            j = j1 - 1  # atom 'bonds' store 1-based indices
            key = (min(i, j), max(i, j))
            if 0 <= j < n and key not in seen:
                seen.add(key)
                conn.extend([key[0], key[1]])
                order.append(bo)

    mol_name = name or atoms[0].get('name') or atoms[0].get('resname') or 'molecule'
    out = {
        "chemicalJson": 1,
        "name": str(mol_name),
        "atoms": {
            "coords": {"3d": coords},
            "elements": {"number": numbers},
            "formalCharges": formal,
        },
    }
    if has_partial:
        out["partialCharges"] = {"atomipy": partial}
    if conn:
        out["bonds"] = {"connections": {"index": conn}, "order": order}

    _, Cell = normalize_box(Box) if Box is not None else (None, None)
    if Cell is not None and len(Cell) == 6:
        out["unitCell"] = {
            "a": Cell[0], "b": Cell[1], "c": Cell[2],
            "alpha": Cell[3], "beta": Cell[4], "gamma": Cell[5],
        }

    out["_generator"] = provenance_string()
    with _smart_open(file_path, 'w') as f:
        _json.dump(out, f)


def auto(atoms, Box, file_path):
    """Automatically choose the appropriate write function based on file extension.
    
    This function will analyze the file extension and call either write_pdb, write_gro, or write_xyz
    based on the detected format.
    
    Args:
        atoms: List of atom dictionaries
        Box: Either Cell parameters for PDB (1x6 list) or Box_dim for GRO/XYZ (1x3 or 1x9 list)
        file_path: Path for the output file
    """
    _, ext = os.path.splitext(file_path)
    ext = ext.lower()
    
    if ext == '.pdb':
        pdb(atoms, Box, file_path)
    elif ext == '.gro':
        gro(atoms, Box, file_path)
    elif ext == '.xyz':
        xyz(atoms, Box, file_path)
    elif ext in ('.cif', '.mmcif', '.mcif'):
        cif(atoms, Box, file_path)
    elif ext == '.pqr':
        pqr(atoms, Box, file_path)
    elif ext in ('.poscar', '.contcar') or os.path.basename(file_path).upper() in ('POSCAR', 'CONTCAR'):
        poscar(atoms, Box, file_path)
    elif ext == '.sdf':
        sdf(atoms, file_path)
    elif ext in ('.cjson', '.json'):
        cjson(atoms, Box, file_path)
    else:
        # Default to PDB if the extension is unrecognized
        pdb(atoms, Box, file_path)

def pqr(atoms, Box, file_path):
    """Write atoms and Cell dimensions to a PQR file.
    
    PQR is a modified PDB format where occupancy and temperature factor
    are replaced by charge and radius.
    """
    if not atoms:
        return

    _, Cell = normalize_box(Box)
    with _smart_open(file_path, 'w') as f:
        f.write(f"REMARK    {provenance_string()} (PQR WRITER)\n")
        if Cell is not None:
            a, b, c, alpha, beta, gamma = Cell
            f.write(f"CRYST1{a:9.3f}{b:9.3f}{c:9.3f}{alpha:7.2f}{beta:7.2f}{gamma:7.2f} P 1           1\n")
            
        for i, atom in enumerate(atoms, start=1):
            index = atom.get('index', i)
            resnum = atom.get('molid', 1)
            resname = f"{atom.get('resname', 'SOL')[:3]:>3}"
            atname = f"{atom.get('type', atom.get('element', 'X'))[:4]:<4}"
            x, y, z = atom.get('x', 0.0), atom.get('y', 0.0), atom.get('z', 0.0)
            charge = atom.get('charge', 0.0)
            radius = atom.get('radius', atom.get('vdw_radius', 0.0))
            
            # PQR format: ATOM index name resname resnum x y z charge radius
            f.write(f"ATOM  {index:5d} {atname} {resname}  {resnum:4d}    {x:8.3f}{y:8.3f}{z:8.3f}{charge:8.4f}{radius:8.4f}\n")
        f.write("TER\nEND\n")

def poscar(atoms, Box, file_path, comment=None):
    """Write atoms to a VASP POSCAR file.
    
    Args:
       atoms: list of atom dictionaries.
       Box: Box dimensions (1x3, 1x6, or 1x9).
       file_path: output filepath.
    """
    if not atoms:
        return

    from .cell_utils import normalize_box
    Box_dim, Cell = normalize_box(Box)
    
    # Construct lattice matrix
    # For simplicity, use the Cell parameters and convert to matrix
    import numpy as np
    if Cell is None:
        raise ValueError("POSCAR export requires a simulation box/cell.")
        
    a, b, c, alpha, beta, gamma = Cell
    alpha_r, beta_r, gamma_r = np.radians([alpha, beta, gamma])
    
    # Basis vectors
    v1 = np.array([a, 0, 0])
    v2 = np.array([b * np.cos(gamma_r), b * np.sin(gamma_r), 0])
    v3_x = c * np.cos(beta_r)
    v3_y = c * (np.cos(alpha_r) - np.cos(beta_r) * np.cos(gamma_r)) / np.sin(gamma_r)
    v3_z = np.sqrt(max(0, c**2 - v3_x**2 - v3_y**2))
    v3 = np.array([v3_x, v3_y, v3_z])
    
    lattice = np.array([v1, v2, v3])
    
    # Group atoms by element for VASP 5 format
    elements = []
    for atom in atoms:
        el = atom.get('element', _infer_element(atom))
        elements.append(el)
        
    unique_elements = []
    for el in elements:
        if el not in unique_elements:
            unique_elements.append(el)
            
    counts = [elements.count(el) for el in unique_elements]
    
    with _smart_open(file_path, 'w') as f:
        f.write(f"{comment if comment is not None else provenance_string()}\n")
        f.write("1.0\n")
        for row in lattice:
            f.write(f"  {row[0]:20.10f} {row[1]:20.10f} {row[2]:20.10f}\n")
        
        f.write("  " + "  ".join(unique_elements) + "\n")
        f.write("  " + "  ".join(str(c) for c in counts) + "\n")
        f.write("Cartesian\n")
        
        # Write atoms ordered by element
        for el in unique_elements:
            for atom in atoms:
                if atom.get('element', _infer_element(atom)) == el:
                    f.write(f"  {atom['x']:20.10f} {atom['y']:20.10f} {atom['z']:20.10f}\n")

def sdf(atoms, file_path):
    """Write atoms to an MDL SDF (V2000) file.

    SDF is commonly used for small molecules. When the atoms carry bond
    information (``atom['bonds']`` as (neighbour_1based, order) tuples, e.g.
    from a Chemical JSON import), a V2000 bonds block with orders is written —
    which downstream tools such as antechamber/acpype rely on for atom typing.
    """
    if not atoms:
        return

    # Collect unique undirected bonds with order from atom['bonds'].
    n_atoms = len(atoms)
    seen = set()
    bond_lines = []  # (a1_1based, a2_1based, order)
    for i, atom in enumerate(atoms):
        for nb in atom.get('bonds', []) or []:
            if isinstance(nb, (tuple, list)):
                j1, order = int(nb[0]), int(nb[1]) if len(nb) > 1 else 1
            else:
                j1, order = int(nb), 1
            a, b = i + 1, j1  # 1-based
            if not (1 <= b <= n_atoms):
                continue
            key = (min(a, b), max(a, b))
            if key not in seen:
                seen.add(key)
                bond_lines.append((key[0], key[1], order if order in (1, 2, 3) else 1))

    with _smart_open(file_path, 'w') as f:
        f.write(f"{provenance_string()}\natomipy\n\n")  # Title, Program, Comment

        n_bonds = len(bond_lines)
        f.write(f"{n_atoms:3d}{n_bonds:3d}  0  0  0  0  0  0  0  0999 V2000\n")

        for atom in atoms:
            x, y, z = atom.get('x', 0.0), atom.get('y', 0.0), atom.get('z', 0.0)
            element = atom.get('element', _infer_element(atom))
            f.write(f"{x:10.4f}{y:10.4f}{z:10.4f} {element:<3s} 0  0  0  0  0  0  0  0  0  0  0  0\n")

        for a, b, order in bond_lines:
            f.write(f"{a:3d}{b:3d}{order:3d}  0  0  0  0\n")

        f.write("M  END\n")
        f.write("$$$$\n")

def write_traj(frames, file_path):
    """Write multiple frames to a trajectory file.
    
    Args:
       frames: list of (atoms, Box) tuples.
       file_path: output filepath.
    """
    ext = os.path.splitext(file_path)[1].lower()
    
    # We clear the file first
    with open(file_path, 'w') as f:
        pass
        
    for i, (atoms, box) in enumerate(frames, start=1):
        if ext == '.pdb':
            # Append in MODEL/ENDMDL format
            with open(file_path, 'a') as f:
                f.write(f"MODEL     {i:4d}\n")
            pdb(atoms, box, file_path) # wait, pdb() writes its own headers. 
            # I should probably refactor pdb/gro/xyz to support append or handle it here.
            # For simplicity, let's just use the existing writers and strip headers if needed,
            # or better, just implement a simple traj writer loop.
            pass
        elif ext == '.gro':
            gro(atoms, box, file_path) # wait, gro() usually overwrites.
            pass
            
    # Actually, the existing writers overwrite. I need to handle the file handle.
    # Refactoring pdb, gro, xyz to take a file handle instead of just path.
    # Actually _smart_open already handles file-like objects!
    
    with open(file_path, 'w') as f_out:
        for i, (atoms, box) in enumerate(frames, start=1):
            if ext == '.pdb':
                f_out.write(f"MODEL     {i:4d}\n")
                pdb(atoms, box, f_out)
                f_out.write("ENDMDL\n")
            elif ext == '.gro':
                gro(atoms, box, f_out)
            elif ext == '.xyz':
                xyz(atoms, box, f_out)
