def mass():
    """Return dictionary mapping elements to their atomic masses in atomic mass units (amu).
    
    Returns
    -------
    dict
        Dictionary mapping element symbols to their atomic masses in amu.
    """
    masses = {
        # Alkali metals
        'H': 1.00784,
        'Hw': 1.00784,
        'Li': 6.941,
        'Na': 22.9898,
        'K': 39.102,
        'Rb': 85.4678,
        'Cs': 132.9054,
        'Fr': 223.0197,
        
        # Alkaline earth metals
        'Be': 9.0122,
        'Mg': 24.305,
        'Ca': 40.078,
        'Sr': 87.62,
        'Ba': 137.327,
        'Ra': 226.0254,
        
        # Transition metals
        'Sc': 44.9559,
        'Ti': 47.867,
        'V': 50.9415,
        'Cr': 51.9961,
        'Mn': 54.938,
        'Fe': 55.845,
        'Co': 58.9332,
        'Ni': 58.6934,
        'Cu': 63.546,
        'Zn': 65.38,
        'Y': 88.9059,
        'Zr': 91.224,
        'Nb': 92.9064,
        'Mo': 95.95,
        'Tc': 98.0,
        'Ru': 101.07,
        'Rh': 102.9055,
        'Pd': 106.42,
        'Ag': 107.8682,
        'Cd': 112.411,
        'Hf': 178.49,
        'Ta': 180.9479,
        'W': 183.84,
        'Re': 186.207,
        'Os': 190.23,
        'Ir': 192.217,
        'Pt': 195.084,
        'Au': 196.9665,
        'Hg': 200.59,
        
        # Post-transition metals
        'Al': 26.982,
        'Ga': 69.723,
        'In': 114.818,
        'Sn': 118.71,
        'Tl': 204.3833,
        'Pb': 207.2,
        'Bi': 208.9804,
        
        # Metalloids
        'B': 10.811,
        'Si': 28.085,
        'Ge': 72.64,
        'As': 74.9216,
        'Sb': 121.76,
        'Te': 127.6,
        'Po': 209.0,
        
        # Non-metals
        'C': 12.0107,
        'N': 14.0067,
        'O': 15.9994,
        'Ow': 15.9994,
        'F': 18.9984,
        'P': 30.9738,
        'S': 32.065,
        'Cl': 35.453,
        'Se': 78.96,
        'Br': 79.904,
        'I': 126.9045,
        'At': 210.0,
        
        # Noble gases
        'He': 4.0026,
        'Ne': 20.1797,
        'Ar': 39.948,
        'Kr': 83.798,
        'Xe': 131.293,
        'Rn': 222.0
    }
    return masses


def set_atomic_masses(atoms):
    """Set the mass attribute for each atom in the atoms list based on its element.
    
    Parameters
    ----------
    atoms : list of dict
        List of atom dictionaries. Each atom should have an 'element' field.
        If the element field is missing, the function will try to use the 'type' field.
    
    Returns
    -------
    list of dict
        The same list of atoms but with 'mass' attribute added to each atom dictionary.
    """
    # Get mass dictionary
    masses_dict = mass()
    
    # Set mass for each atom
    for atom in atoms:
        # Try to get element from atom dictionary
        element = atom.get('element', atom.get('type', ''))
        
        # Convert element to standard format (strip numbers, etc.)
        # This handles cases like 'Mg2+', 'O2-', etc.
        import re
        element = re.sub(r'[^a-zA-Z]', '', element)
        
        # Handle case sensitivity - use just first one or two letters with correct case
        if len(element) > 0:
            if len(element) > 1 and element[1].islower():
                element = element[0].upper() + element[1].lower()
            else:
                element = element[0].upper()
        
        # Assign mass if element is known
        if element in masses_dict:
            atom['mass'] = masses_dict[element]
        elif len(element) > 1 and (element[0].upper() + element[1].lower()) in masses_dict:
            # Fallback: try first two characters with proper case (e.g. 'SIT' -> 'Si', 'MGO' -> 'Mg')
            fallback2 = element[0].upper() + element[1].lower()
            atom['mass'] = masses_dict[fallback2]
        elif len(element) > 1 and element[0].upper() in masses_dict:
            # Fallback: try first character only (e.g. 'Hw' -> 'H', 'Ow' -> 'O')
            fallback = element[0].upper()
            atom['mass'] = masses_dict[fallback]
        else:
            print(f"Warning: Unknown element '{element}' for atom {atom.get('index', '?')}. Using default mass 1.0.")
            atom['mass'] = 1.0
    
    return atoms


def com(atoms, add_to_atoms=True):
    """Calculate the center of mass of a molecule or slab without PBC wrapping.
    
    Parameters
    ----------
    atoms : list of dict
        List of atom dictionaries. Each atom should have 'x', 'y', 'z' coordinates
        and either 'mass' or 'element'/'type' fields.
    add_to_atoms : bool, optional
        If True, adds the center of mass to each atom dictionary. Default is True.
    
    Returns
    -------
    tuple
        (com_x, com_y, com_z) - Coordinates of the center of mass.
    
    Notes
    -----
    - The function does not consider periodic boundary conditions (no wrapping).
    - If atoms don't have mass assigned, it will call set_atomic_masses() first.
    """
    # Check if atoms have mass attribute, if not, set them. Check ALL atoms (not just the
    # first 10) so a later mass-less atom doesn't slip through and KeyError below.
    if not all('mass' in atom for atom in atoms):
        atoms = set_atomic_masses(atoms)
    
    total_mass = 0.0
    weighted_x = 0.0
    weighted_y = 0.0
    weighted_z = 0.0
    
    # Calculate weighted sum of positions
    for atom in atoms:
        mass = atom.get('mass', 0.0)
        weighted_x += atom['x'] * mass
        weighted_y += atom['y'] * mass
        weighted_z += atom['z'] * mass
        total_mass += mass
    
    # Calculate center of mass
    if total_mass > 0:
        com_x = weighted_x / total_mass
        com_y = weighted_y / total_mass
        com_z = weighted_z / total_mass
    else:
        # Fallback if no mass is found (shouldn't happen)
        print("Warning: Total mass is zero. Using geometric center instead.")
        com_x = sum(atom['x'] for atom in atoms) / len(atoms)
        com_y = sum(atom['y'] for atom in atoms) / len(atoms)
        com_z = sum(atom['z'] for atom in atoms) / len(atoms)
    
    # Add COM to each atom dictionary if requested. Use a FRESH dict per atom so mutating
    # one atom's 'com' doesn't change every other atom's (they must not share a reference).
    if add_to_atoms:
        for atom in atoms:
            atom['com'] = {'x': com_x, 'y': com_y, 'z': com_z}
    
    return (com_x, com_y, com_z)
