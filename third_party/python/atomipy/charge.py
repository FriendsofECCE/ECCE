"""
This module consolidates charge assignment functions from:
  - charge_formal.py
  - charge_minff.py
  - charge_clayff.py

It provides functions to:
  • Assign formal charges based on residue names and oxidation states.
  • Assign charges following MINFF and CLAYFF forcefield principles.
  • Balance the total system charge.
"""

import copy
from .bond_angle import bond_angle

# =====================================================
# Formal charge functions (from charge_formal.py)
# =====================================================
def assign_formal_charges(atoms):
    """
    Assign formal charges to atoms based on residue name and atom type.
    
    For 'ION' residues, assigns full formal charge.
    For 'SOL' residues, assigns charges following the OPC3 water model.
    For all other atoms, assigns half the formal charge based on the element's most common oxidation state.
    
    Args:
        atoms: List of atom dictionaries.
        
    Returns:
        Updated atoms list with a 'charge' field.
    """
    ion_charge = 0.0
    water_charge = 0.0
    other_charge = 0.0
    
    for atom in atoms:
        resname = atom.get('resname', '')
        atom_type = atom.get('type', '')
        element = atom.get('element', '')
        
        if resname.upper() == 'ION':
            atom['charge'] = get_ion_charge(atom_type)
            ion_charge += atom['charge']
        elif resname.upper() == 'SOL':
            atom['charge'] = get_water_charge(atom_type)
            water_charge += atom['charge']
        else:
            atom['charge'] = get_half_formal_charge(element or atom_type)
            other_charge += atom['charge']
    
    total_charge = ion_charge + water_charge + other_charge
    return atoms

def get_ion_charge(atom_type):
    """
    Get the formal charge for an ion based on its atom type.
    """
    atom_type_lower = atom_type.lower()
    if any(x in atom_type_lower for x in ['li', 'na', 'k', 'rb', 'cs']):
        return 1.0
    if any(x in atom_type_lower for x in ['mg', 'ca', 'sr', 'ba', 'zn', 'cu', 'ni']):
        return 2.0
    if any(x in atom_type_lower for x in ['al', 'fe3+']):
        if 'fe2' in atom_type_lower:
            return 2.0
        else:
            return 3.0
    if atom_type_lower.endswith('-') or atom_type_lower in ['f', 'cl', 'br', 'i']:
        return -1.0
    print(f"Warning: No formal charge defined for ion type '{atom_type}'. Assuming charge 0.0")
    return 0.0

def get_water_charge(atom_type):
    """
    Get the water charge based on the OPC3 water model.
    """
    atom_type_lower = atom_type.lower()
    if atom_type_lower.startswith('o') or atom_type_lower == 'ow':
        return -0.89517
    if atom_type_lower.startswith('h'):
        return 0.447585
    print(f"Warning: No water charge defined for atom type '{atom_type}'. Assuming charge 0.0")
    return 0.0

def get_formal_charge(element_or_type):
    """
    Get the full formal charge for an element or atom type based on its common oxidation state.
    """
    oxidation_states = {
        'H': 1, 'LI': 1, 'NA': 1, 'K': 1, 'RB': 1, 'CS': 1,
        'BE': 2, 'MG': 2, 'CA': 2, 'SR': 2, 'BA': 2,
        'B': 3, 'AL': 3, 'GA': 3, 'IN': 3,
        'C': 4, 'SI': 4, 'GE': 4, 'SN': 4, 'PB': 2,
        'N': -3, 'P': 5, 'AS': 3, 'SB': 3, 'BI': 3,
        'O': -2, 'S': -2, 'SE': -2, 'TE': -2,
        'F': -1, 'CL': -1, 'BR': -1, 'I': -1,
        'TI': 4, 'V': 5, 'CR': 3, 'MN': 2, 'FE': 3,
        'CO': 2, 'NI': 2, 'CU': 2, 'ZN': 2,
        'ZR': 4, 'NB': 5, 'MO': 6, 'TC': 7, 'RU': 3,
        'RH': 3, 'PD': 2, 'AG': 1, 'CD': 2,
        'HF': 4, 'TA': 5, 'W': 6, 'RE': 7, 'OS': 4,
        'IR': 3, 'PT': 2, 'AU': 3, 'HG': 2,
    }
    oxidation_states_upper = {k.upper(): v for k, v in oxidation_states.items()}
    key = str(element_or_type).upper()
    if key in oxidation_states_upper:
        return float(oxidation_states_upper[key])
    if len(key) > 1 and key[:2] in oxidation_states_upper:
        return float(oxidation_states_upper[key[:2]])
    if len(key) > 0 and key[0] in oxidation_states_upper:
        return float(oxidation_states_upper[key[0]])
    print(f"Warning: No oxidation state defined for '{element_or_type}'. Assuming charge 0.0")
    return 0.0

def get_half_formal_charge(element_or_type):
    """
    Get half the formal charge for an element or atom type based on its common oxidation state.
    """
    return get_formal_charge(element_or_type) / 2.0


# =====================================================
# MINFF forcefield charge functions (from charge_minff.py)
# =====================================================
def charge_minff(atoms, Box, atom_labels=None, charges=None, resname=None):
    """
    Assign charges to atoms based on MINFF forcefield principles.
    
    This assigns specific charges to given atom labels, and then adjusts oxygen charges based on neighboring atoms.
    
    Args:
        atoms: List of atom dictionaries.
        Box: Box dimensions for periodic boundary conditions.
        atom_labels: Optional list of atom types to assign specific charges.
        charges: Optional list of charge values corresponding to atom_labels.
        resname: Optional residue name filter (e.g., 'MIN').
        
    Returns:
        Updated list of atoms with a 'charge' field.
    """
    n_atoms = len(atoms)
    for atom in atoms:
        atom['charge'] = 0.0

    if resname is not None:
        target_atoms = [i for i, atom in enumerate(atoms) if atom.get('resname', '').upper() == resname.upper()]
        if not target_atoms:
            print(f"Warning: No atoms found with resname '{resname}'")
            return atoms
        print(f"Found {len(target_atoms)} atoms with resname '{resname}'")
    else:
        target_atoms = list(range(n_atoms))

    metal_indices = []
    if atom_labels is not None and charges is not None:
        if not isinstance(atom_labels, list):
            atom_labels = [atom_labels]
        if not isinstance(charges, list):
            charges = [charges]
        sorted_pairs = sorted(zip(atom_labels, charges), key=lambda x: x[0])
        atom_labels = [pair[0] for pair in sorted_pairs]
        charges = [pair[1] for pair in sorted_pairs]
        for label, charge in zip(atom_labels, charges):
            for i, atom in enumerate(atoms):
                atom_type = atom.get('type', atom.get('element', ''))
                if atom_type.lower() == label.lower():
                    atom['charge'] = charge
                    metal_indices.append(i)

    ox_indices = [i for i in target_atoms if 'type' in atoms[i] and atoms[i]['type'].lower().startswith('o')]
    fs_indices = [i for i in target_atoms if 'type' in atoms[i] and atoms[i]['type'].lower().startswith('fs')]

    if 'bonds' not in atoms[0] and 'neigh' not in atoms[0]:
        atoms, _, _ = bond_angle(atoms, Box, rmaxH=1.2, rmaxM=2.45)

    for i in ox_indices:
        neighbors = atoms[i].get('neigh', [])
        if not neighbors:
            continue
        zsum = 0.0
        for j in neighbors:
            neighbor_type = atoms[j].get('type', '').lower()
            if neighbor_type.startswith('si'):
                z = 4
            elif neighbor_type.startswith('al'):
                z = 3
            elif neighbor_type.startswith('fe2'):
                z = 2
            elif neighbor_type.startswith('feo2'):
                z = 2
            elif neighbor_type.startswith('fet2'):
                z = 2
            elif neighbor_type.startswith('fee2'):
                z = 2
            elif neighbor_type.startswith('fe3'):
                z = 3
            elif neighbor_type.startswith('feo3'):
                z = 3
            elif neighbor_type.startswith('fet3'):
                z = 3
            elif neighbor_type.startswith('fee3'):
                z = 3
            elif neighbor_type.startswith('ti'):
                z = 4
            elif neighbor_type.startswith('li'):
                z = 1
            elif neighbor_type.startswith('mg'):
                z = 2
            elif neighbor_type.startswith('ca'):
                z = 2
            elif neighbor_type.startswith('h'):
                z = 1
            else:
                z = 0
            zp = atoms[j].get('charge', 0)
            cn = len(atoms[j].get('neigh', [])) or 1
            zsum += (z - zp) / cn
        atoms[i]['charge'] = -2.00 + zsum

        # Process fluorine atoms (similar to oxygen)
    for i in fs_indices:
        # Get bonded atom indices
        neighbors = atoms[i].get('neigh', [])
        if not neighbors:
            continue
        
        # Calculate charge based on neighbors
        zsum = 0.0
        for j in neighbors:
            # Determine formal charge of neighbor based on type
            neighbor_type = atoms[j].get('type', '').lower()
            
            # Determine formal valence Z
            if neighbor_type.startswith('si'):
                z = 4
            elif neighbor_type.startswith('al'):
                z = 3
            elif neighbor_type.startswith('fe2'):
                z = 2
            elif neighbor_type.startswith('feo2'):
                z = 2
            elif neighbor_type.startswith('fet2'):
                z = 2
            elif neighbor_type.startswith('fee2'):
                z = 2
            elif neighbor_type.startswith('fe3'):
                z = 3
            elif neighbor_type.startswith('feo3'):
                z = 3
            elif neighbor_type.startswith('fet3'):
                z = 3
            elif neighbor_type.startswith('fee3'):
                z = 3
            elif neighbor_type.startswith('ti'):
                z = 4
            elif neighbor_type.startswith('li'):
                z = 1
            elif neighbor_type.startswith('mg'):
                z = 2
            elif neighbor_type.startswith('ca'):
                z = 2
            elif neighbor_type.startswith('h'):
                z = 1
            else:
                z = 0
                
            # Get current charge and coordination number
            zp = atoms[j].get('charge', 0)
            cn = len(atoms[j].get('neigh', []))
            if cn == 0:  # Avoid division by zero
                cn = 1
                
            # Contribution to charge balance
            zsum += (z - zp) / cn
            
        # Set fluorine charge
        atoms[i]['charge'] = -1.00 + zsum

    hw_indices = [i for i in target_atoms if 'type' in atoms[i] and atoms[i]['type'].lower().startswith('hw')]
    ow_indices = [i for i in target_atoms if 'type' in atoms[i] and atoms[i]['type'].lower().startswith('ow')]
    for i in hw_indices:
        atoms[i]['charge'] = 0.447585
    for i in ow_indices:
        atoms[i]['charge'] = -0.89517

    total_charge = sum(atoms[i].get('charge', 0) for i in target_atoms) if resname is not None else sum(atom.get('charge', 0) for atom in atoms)
    print(f"Total charge (MINFF): {total_charge:.6f}")
    if abs(round(total_charge) - total_charge) > 1e-5:
        print("Warning: Non-integer total charge. Adjusting to nearest integer.")
        atoms = balance_charges(atoms, round(total_charge))
    return atoms

# =====================================================
# CLAYFF forcefield charge functions (from charge_clayff.py)
# =====================================================
def charge_clayff(atoms, Box, atom_labels=None, charges=None, resname=None):
    """
    Assign charges to atoms based on CLAYFF forcefield principles.
    
    Similar to charge_minff but using CLAYFF conventions (modified oxygen neighbor considerations 
    and SPC/E water charge values).
    
    Args:
        atoms: List of atom dictionaries.
        Box: Simulation cell dimensions (1x3, 1x6, or 1x9).
        atom_labels: Optional list of atom types to assign specific charges.
        charges: Optional list of charge values corresponding to atom_labels.
        resname: Optional residue name filter (e.g., 'MIN').
        
    Returns:
        Updated list of atoms with a 'charge' field.
    """
    n_atoms = len(atoms)
    for atom in atoms:
        atom['charge'] = 0.0

    if resname is not None:
        target_atoms = [i for i, atom in enumerate(atoms) if atom.get('resname', '').upper() == resname.upper()]
        if not target_atoms:
            print(f"Warning: No atoms found with resname '{resname}'")
            return atoms
        print(f"Found {len(target_atoms)} atoms with resname '{resname}'")
    else:
        target_atoms = list(range(n_atoms))

    metal_indices = []
    if atom_labels is not None and charges is not None:
        if not isinstance(atom_labels, list):
            atom_labels = [atom_labels]
        if not isinstance(charges, list):
            charges = [charges]
        sorted_pairs = sorted(zip(atom_labels, charges), key=lambda x: x[0])
        atom_labels = [pair[0] for pair in sorted_pairs]
        charges = [pair[1] for pair in sorted_pairs]
        for label, charge in zip(atom_labels, charges):
            for i, atom in enumerate(atoms):
                atom_type = atom.get('type', atom.get('element', ''))
                if atom_type.lower() == label.lower():
                    atom['charge'] = charge
                    metal_indices.append(i)

    ox_indices = [i for i in target_atoms if 'type' in atoms[i] and atoms[i]['type'].lower().startswith('o')]

    if 'bonds' not in atoms[0] and 'neigh' not in atoms[0]:
        atoms, _, _ = bond_angle(atoms, Box, rmaxH=1.2, rmaxM=2.45)

    for i in ox_indices:
        neighbors = atoms[i].get('neigh', [])
        if not neighbors:
            continue
        zsum = 0.0
        for j in neighbors:
            neighbor_type = atoms[j].get('type', '').lower()
            if neighbor_type.startswith('si'):
                z = 4
            elif neighbor_type.startswith('al'):
                z = 3
            elif 'fe' in neighbor_type and '2' in neighbor_type:
                z = 2
            elif 'fe' in neighbor_type and '3' in neighbor_type:
                z = 3
            elif neighbor_type.startswith('f'):  # Adjusted for CLAYFF
                z = 3
            elif neighbor_type.startswith('ti'):
                z = 4
            elif neighbor_type.startswith('li'):
                z = 1
            elif neighbor_type.startswith('mg'):
                z = 2
            elif neighbor_type.startswith('ca'):
                z = 2
            elif neighbor_type.startswith('h'):
                z = 1
            else:
                z = 0
            zp = atoms[j].get('charge', 0)
            cn = len(atoms[j].get('neigh', [])) or 1
            zsum += (z - zp) / cn
        atoms[i]['charge'] = -2.00 + zsum

    hw_indices = [i for i in target_atoms if 'type' in atoms[i] and atoms[i]['type'].lower().startswith('hw')]
    ow_indices = [i for i in target_atoms if 'type' in atoms[i] and atoms[i]['type'].lower().startswith('ow')]
    for i in hw_indices:
        atoms[i]['charge'] = 0.4238
    for i in ow_indices:
        atoms[i]['charge'] = -0.8476

    total_charge = sum(atoms[i].get('charge', 0) for i in target_atoms) if resname is not None else sum(atom.get('charge', 0) for atom in atoms)
    print(f"Total charge (CLAYFF): {total_charge:.6f}")
    if abs(round(total_charge) - total_charge) > 1e-5:
        print("Warning: Non-integer total charge. Adjusting to nearest integer.")
        atoms = balance_charges(atoms, round(total_charge))
    return atoms

# =====================================================
# Common charge balancing function
# =====================================================
def balance_charges(atoms, target_total_charge=None):
    """
    Balance the charges of atoms to reach an integer total charge.
    
    Distributes any excess or deficit evenly among oxygen atoms.
    
    Args:
        atoms: List of atom dictionaries with a 'charge' field.
        target_total_charge: Target total charge (if None, rounds current total).
        
    Returns:
        Updated list of atoms with balanced charges.
    """
    current_total = sum(atom.get('charge', 0) for atom in atoms)
    if target_total_charge is None:
        target_total_charge = round(current_total)
    ox_indices = [i for i, atom in enumerate(atoms) if 'type' in atom and atom['type'].lower().startswith('o')]
    if ox_indices:
        charge_adjust = (target_total_charge - current_total) / len(ox_indices)
        for i in ox_indices:
            atoms[i]['charge'] += charge_adjust
        final_total = sum(atom.get('charge', 0) for atom in atoms)
        print(f"Final total charge: {final_total:.6f} (target was {target_total_charge})")
    else:
        print("Warning: No oxygen atoms found for charge balancing.")
    return atoms

if __name__ == '__main__':
    # Simple test run
    test_atoms = [
        {'type': 'O', 'resname': 'SOL'},
        {'type': 'HW', 'resname': 'SOL'},
        {'type': 'Al', 'resname': 'MIN'},
        {'type': 'Si', 'resname': 'MIN'},
    ]
    print("Assigning formal charges:")
    assign_formal_charges(test_atoms)
    print("MINFF charge assignment:")
    charge_minff(test_atoms, [10, 10, 10])
    print("CLAYFF charge assignment:")
    charge_clayff(test_atoms, [10, 10, 10])
