"""
This module provides functions for assigning formal charges to atoms.

The main function assigns charges based on residue names:
- ION residues get their full formal charge
- SOL residues get charges following the OPC3 water model
- Other atoms get half the formal charge of their most common oxidation state
"""

def assign_formal_charges(atoms):
    """
    Assign formal charges to atoms based on residue name and atom type.
    
    Args:
        atoms: List of atom dictionaries
        
    Returns:
        The updated list of atom dictionaries with 'charge' field assigned
    
    Example:
        # Assign formal charges to all atoms
        atoms = ap.charge_formal.assign_formal_charges(atoms)
    """
    # Initialize total charge tracking
    ion_charge = 0.0
    water_charge = 0.0
    other_charge = 0.0
    
    # Process each atom
    for atom in atoms:
        resname = atom.get('resname', '')
        atom_type = atom.get('type', '')
        element = atom.get('element', '')
        
        if resname == 'ION':
            # Assign formal charges to ions
            atom['charge'] = get_ion_charge(atom_type)
            ion_charge += atom['charge']
        elif resname == 'SOL':
            # Assign OPC3 water model charges
            atom['charge'] = get_water_charge(atom_type)
            water_charge += atom['charge']
        else:
            # Assign half the formal charge based on the most common oxidation state
            atom['charge'] = get_half_formal_charge(element or atom_type)
            other_charge += atom['charge']
    
    # Print summary
    total_charge = ion_charge + water_charge + other_charge
    print(f"Charge summary:")
    print(f"  Ions: {ion_charge:.4f}")
    print(f"  Water: {water_charge:.4f}")
    print(f"  Other: {other_charge:.4f}")
    print(f"  Total: {total_charge:.4f}")
    
    return atoms


def get_ion_charge(atom_type):
    """
    Get the formal charge for an ion based on its atom type.
    
    Args:
        atom_type: Atom type string
        
    Returns:
        Float formal charge value
    """
    # Convert to lowercase for case-insensitive comparison
    atom_type_lower = atom_type.lower()
    
    # Monovalent cations (+1)
    if any(ion in atom_type_lower for ion in ['li', 'na', 'k', 'rb', 'cs']):
        if atom_type_lower.endswith('+'):
            return 1.0
        else:
            return 1.0  # Assume +1 even if + is not explicitly in the type
            
    # Divalent cations (+2)
    if any(ion in atom_type_lower for ion in ['mg', 'ca', 'sr', 'ba', 'zn', 'cu', 'ni']):
        if '2+' in atom_type_lower:
            return 2.0
        else:
            return 2.0  # Assume +2 even if 2+ is not explicitly in the type
            
    # Trivalent cations (+3)
    if any(ion in atom_type_lower for ion in ['al','alt','ale','alo','fe3+']):
        if '3+' in atom_type_lower:
            return 3.0
        elif 'fe' in atom_type_lower:
            # Special case for iron which can have multiple oxidation states
            if 'fe2' in atom_type_lower:
                return 2.0
            else:
                return 3.0
        else:
            return 3.0
            
    # Anions
    if atom_type_lower.endswith('-') or atom_type_lower in ['f', 'cl', 'br', 'i']:
        if atom_type_lower in ['f', 'f-', 'cl', 'cl-', 'br', 'br-', 'i', 'i-']:
            return -1.0
            
    # Default case if no match
    print(f"Warning: No formal charge defined for ion type '{atom_type}'. Assuming charge 0.0")
    return 0.0


def get_water_charge(atom_type):
    """
    Get the charge for a water atom based on the OPC3 water model.
    
    Args:
        atom_type: Atom type string
        
    Returns:
        Float charge value according to OPC3 model
    """
    # OPC3 water model charges
    # Reference: https://doi.org/10.1021/acs.jctc.6b00469
    atom_type_lower = atom_type.lower()
    
    if atom_type_lower.startswith('o') or atom_type_lower == 'ow':
        return -0.89517  # Oxygen charge in OPC3 model
    elif atom_type_lower.startswith('h') or atom_type_lower == 'hw':
        return 0.447585  # Hydrogen charge in OPC3 model (divided among two H atoms)
    
    # Default case if not recognized
    print(f"Warning: Unrecognized water atom type '{atom_type}'. Assuming charge 0.0")
    return 0.0


def get_half_formal_charge(element_or_type):
    """
    Get half the formal charge for an atom based on its most common oxidation state.
    
    Args:
        element_or_type: Element symbol or atom type
        
    Returns:
        Float charge value (half of the most common oxidation state)
    """
    # Extract element symbol from atom type if needed
    element = element_or_type.strip('0123456789+-').split('_')[0]
    element = element[:2] if len(element) > 1 and element[1].islower() else element[0]
    
    # Common oxidation states for elements (most frequent oxidation state)
    oxidation_states = {
        # Group 1 (Alkali metals)
        'H': 1,
        'Li': 1,
        'Na': 1,
        'K': 1,
        'Rb': 1,
        'Cs': 1,
        
        # Group 2 (Alkaline earth metals)
        'Be': 2,
        'Mg': 2,
        'Ca': 2,
        'Sr': 2,
        'Ba': 2,
        
        # Group 13
        'B': 3,
        'Al': 3,
        'Ga': 3,
        'In': 3,
        
        # Group 14
        'C': 4,  # or -4, but +4 is common in minerals
        'Si': 4,
        'Ge': 4,
        'Sn': 4,
        'Pb': 2,  # Pb(II) is more common than Pb(IV)
        
        # Group 15
        'N': -3,
        'P': 5,
        'As': 3,
        'Sb': 3,
        'Bi': 3,
        
        # Group 16
        'O': -2,
        'S': -2,
        'Se': -2,
        'Te': -2,
        
        # Group 17 (Halogens)
        'F': -1,
        'Cl': -1,
        'Br': -1,
        'I': -1,
        
        # Transition metals (most common oxidation states)
        'Ti': 4,
        'V': 5,
        'Cr': 3,
        'Mn': 2,
        'Fe': 3,  # Fe(III) is common in minerals
        'Co': 2,
        'Ni': 2,
        'Cu': 2,
        'Zn': 2,
        'Zr': 4,
        'Nb': 5,
        'Mo': 6,
        'Tc': 7,
        'Ru': 3,
        'Rh': 3,
        'Pd': 2,
        'Ag': 1,
        'Cd': 2,
        'Hf': 4,
        'Ta': 5,
        'W': 6,
        'Re': 7,
        'Os': 4,
        'Ir': 3,
        'Pt': 2,
        'Au': 3,
        'Hg': 2,
    }
    
    # Get oxidation state and halve it (case-insensitive lookup)
    element_upper = element.upper()
    
    # Check for case-insensitive match
    for elem, oxidation in oxidation_states.items():
        if elem.upper() == element_upper:
            return oxidation / 2.0
    
    # Default case
    print(f"Warning: No oxidation state defined for element '{element}'. Assuming charge 0.0")
    return 0.0


def balance_charges(atoms, target_total_charge=0.0):
    """
    Balance the charges of atoms to reach a target total charge.
    
    This function is useful when the sum of formal charges doesn't match
    the desired total charge of the system.
    
    Args:
        atoms: List of atom dictionaries with 'charge' field
        target_total_charge: The desired total charge (default: 0.0 for neutrality)
        
    Returns:
        The updated list of atoms with balanced charges
    """
    # Calculate current total charge
    current_total = sum(atom.get('charge', 0) for atom in atoms)
    
    # Find atoms that are not water or ions to distribute charge correction
    adjust_indices = [i for i, atom in enumerate(atoms) 
                     if atom.get('resname', '') not in ['SOL', 'ION']]
    
    if adjust_indices:
        # Calculate charge adjustment per atom
        charge_adjust = (target_total_charge - current_total) / len(adjust_indices)
        
        # Apply adjustment
        for i in adjust_indices:
            atoms[i]['charge'] += charge_adjust
            
        # Verify final charge
        final_total = sum(atom.get('charge', 0) for atom in atoms)
        print(f"Final total charge: {final_total:.4f} (target was {target_total_charge:.4f})")
    else:
        print("Warning: No non-water, non-ion atoms found for charge balancing.")
    
    return atoms
