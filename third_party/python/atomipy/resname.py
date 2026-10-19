"""
This module provides functions for assigning residue names to atoms.

The main function guesses and assigns residue names (resname) based on atom types,
with special handling for water and ions.
"""

def assign_resname(atoms, default_resname='MIN'):
    """
    Assign residue names to atoms based on their types.
    
    This function assigns 'SOL' to water atoms, 'ION' to ion atoms,
    and the specified default residue name to all other atoms.
    
    Parameters
    ----------
    atoms : list of dict
        Atom dictionaries.
    default_resname : str, optional
        Residue name to assign to non-water, non-ion atoms (default 'MIN').

    Returns
    -------
    list of dict
        Updated atoms with `resname` assigned.

    Examples
    --------
    atoms = assign_resname(atoms)
    atoms = assign_resname(atoms, default_resname='CLAY')
    """
    # Define atom type patterns for water and ions
    water_types = ['Hw', 'Ow', 'OW', 'HW','OH2']
    
    ion_types = [
        # Charged ion type names (explicit charge notation)
        'Li+', 'LI+', 'Na+', 'NA+', 'K+', 
        'Rb+', 'RB+', 'Cs+', 'CS+',
        'Ca2+', 'CA2+', 'Cu2+', 'CU2+',
        'Ni2+', 'NI2+', 'Zn2+', 'ZN2+',
        'Sr2+', 'SR2+', 'Ba2+', 'BA2+',
        'Mg2+', 'MG2+',
        'F-', 'Cl-', 'CL-', 'Br-', 'BR-', 'I-',
        
        # GROMACS/CHARMM ion type names (unambiguous)
        'SOD', 'POT', 'CLA', 'CAL'
    ]
    
    # Ion residue names (common naming conventions)
    ion_resnames = [
        'SOD', 'POT', 'CLA', 'CAL', 'MG', 'NA', 'K', 'CL', 'CA', 'ION',
        'LI', 'RB', 'CS', 'CU', 'NI', 'ZN', 'SR', 'BA', 'BR', 'F'
    ]
    
    # Create list of atom indices for each category
    sol_indices = []
    ion_indices = []
    
    # Process each atom to identify water and ions
    for i, atom in enumerate(atoms):
        atom_type = atom.get('type', '')
        atom_resname = atom.get('resname', '').upper()
        
        # Check for water atoms (case-insensitive prefix match)
        is_water = any(atom_type.lower().startswith(water_type.lower()) for water_type in water_types)
        
        # Check for ion atoms by type OR by resname
        is_ion = (atom_type in ion_types or 
                  atom_type.upper() in ion_types or
                  atom_resname in ion_resnames)
        
        if is_water:
            sol_indices.append(i)
        elif is_ion:
            ion_indices.append(i)
    
    # Sort indices
    sol_indices.sort()
    ion_indices.sort()
    
    # Map specific ion types to their canonical GROMACS resname
    def get_canonical_ion_resname(atom):
        raw_type = str(atom.get('type') or '').strip().upper()
        raw_res = str(atom.get('resname') or '').strip().upper()
        raw_elem = str(atom.get('element') or '').strip().upper()
        
        # Check all sources of element/type information
        for src in [raw_type, raw_elem, raw_res]:
            clean = src.rstrip('+-0123456789')
            if clean in ['NA', 'SOD']: return 'Na'
            if clean in ['CL', 'CLA']: return 'Cl'
            if clean in ['LI']: return 'Li'
            if clean in ['K', 'POT']: return 'K'
            if clean in ['CA', 'CAL']: return 'Ca'
            if clean in ['MG']: return 'Mg'
            if clean in ['ZN']: return 'Zn'
            if clean in ['CS']: return 'Cs'
            if clean in ['RB']: return 'Rb'
            if clean in ['F']: return 'F'
            if clean in ['BR']: return 'Br'
            if clean in ['I']: return 'I'
            if clean in ['CU']: return 'Cu'
            if clean in ['NI']: return 'Ni'
            if clean in ['SR']: return 'Sr'
            if clean in ['BA']: return 'Ba'
            
        return 'ION'

    # Assign 'SOL' to water atoms
    for i in sol_indices:
        atoms[i]['resname'] = 'SOL'
    
    # Assign specific element resnames to ion atoms
    for i in ion_indices:
        atoms[i]['resname'] = get_canonical_ion_resname(atoms[i])
    
    # Assign default resname to remaining atoms (always assign, overwriting existing)
    other_indices = [i for i in range(len(atoms)) if i not in sol_indices and i not in ion_indices]
    for i in other_indices:
        atoms[i]['resname'] = default_resname
    
    # Do a last check of resnum/molid assignment:
    # Ensure all atoms have 'resnum' and 'molid' consistent with their molecule boundaries
    current_molid = 1
    if len(atoms) > 0:
        atoms[0]['molid'] = current_molid
        atoms[0]['resnum'] = current_molid
        
        for i in range(1, len(atoms)):
            # If the resname changes, or if they had different original molids, they are different molecules.
            # Water molecules ('SOL') should have at most 3 atoms per molecule (O + 2H), so we group them in 3s or by type.
            # Actually, standard water has OW, HW1, HW2. If we see a new OW/Hw, or if resname changes, start a new molecule.
            new_mol = False
            if atoms[i]['resname'] != atoms[i-1]['resname']:
                new_mol = True
            elif atoms[i]['resname'] == 'SOL':
                # Water: start a new molecule when we see an Oxygen atom
                atype = str(atoms[i].get('type') or '').upper()
                if atype.startswith('O') or atype.startswith('OW'):
                    new_mol = True
            elif atoms[i]['resname'] in ['Na', 'Cl', 'Li', 'K', 'Ca', 'Mg', 'Zn', 'Cs', 'Rb', 'F', 'Br', 'I', 'ION']:
                # Ions: each ion is its own molecule
                new_mol = True
            elif atoms[i].get('molid') != atoms[i-1].get('molid'):
                new_mol = True
                
            if new_mol:
                current_molid += 1
            atoms[i]['molid'] = current_molid
            atoms[i]['resnum'] = current_molid

    # Print summary
    print(f"Assigned resnames: {len(sol_indices)} water (SOL), {len(ion_indices)} ions, "
          f"{len(other_indices)} other atoms ({default_resname})")
    
    return atoms


def change_default_resname(atoms, new_resname, current_resname='MIN'):
    """
    Change the residue name for atoms with a specific current residue name.
    
    This is useful to reclassify groups of atoms without affecting water and ions.
    
    Args:
        atoms: List of atom dictionaries
        new_resname: The new residue name to assign
        current_resname: The current residue name to change (default: 'MIN')
    
    Returns:
        The updated list of atom dictionaries
    
    Example:
        # Change all 'MIN' residues to 'CLAY'
        atoms = ap.resname.change_default_resname(atoms, 'CLAY')
    """
    count = 0
    for atom in atoms:
        if atom.get('resname') == current_resname:
            atom['resname'] = new_resname
            count += 1
    
    print(f"Changed {count} atoms from resname '{current_resname}' to '{new_resname}'")
    return atoms
