"""
This module provides charge assignment functions following the MINFF forcefield methodology.

The charge_minff function assigns and balances charges for atoms according to the MINFF
forcefield rules, particularly for isomorphic substitution sites.
"""

import numpy as np
from .bond_angle import bond_angle

def charge_minff(atoms, box_dim, atom_labels=None, charges=None, resname=None):
    """
    Assign charges to atoms based on MINFF forcefield principles.
    
    This function follows the approach of the MATLAB charge_minff_MATLAB.m function,
    which smears out the charge at isomorphic substitution sites according to MINFF
    principles. It first assigns specific charges to specified atom types, then 
    distributes the remaining charges across oxygen atoms.
    
    Args:
        atoms: List of atom dictionaries
        box_dim: Box dimensions for periodic boundary conditions
        atom_labels: Optional list of atom types/elements to assign specific charges to
        charges: Optional list of charge values corresponding to atom_labels
        resname: Optional residue name filter (e.g., 'MIN'). If provided, only atoms with 
                this residue name will have charges assigned.
        
    Returns:
        The updated list of atom dictionaries with 'charge' field added
    
    Example:
        # Set specific charges for mineral atoms (resname='MIN') only
        atoms = ap.charge_minff.charge_minff(atoms, box_dim, 
                                             ['Al', 'Mg', 'Si', 'H'], 
                                             [1.575, 1.36, 2.1, 0.425],
                                             resname='MIN')
    """
    # Initialize all charges to zero
    n_atoms = len(atoms)
    for atom in atoms:
        atom['charge'] = 0.0
    
    # Filter atoms by resname if specified
    if resname is not None:
        # Create a filtered list of indices for atoms matching the resname
        target_atoms = [i for i, atom in enumerate(atoms) if atom.get('resname', '').upper() == resname.upper()]
        if not target_atoms:
            print(f"Warning: No atoms found with resname '{resname}'")
            return atoms
        print(f"Found {len(target_atoms)} atoms with resname '{resname}'")
    else:
        # Use all atoms if no resname filter is specified
        target_atoms = list(range(n_atoms))
    
    # Create lists for atom indices with explicit charges vs. oxygens/fluorines
    metal_indices = []
    
    # If atom_labels and charges are provided, assign specific charges
    if atom_labels is not None and charges is not None:
        # Convert atom_labels to list if it's not already
        if not isinstance(atom_labels, list):
            atom_labels = [atom_labels]
        
        # Convert charges to list if it's not already
        if not isinstance(charges, list):
            charges = [charges]
        
        # Sort atom_labels and charges together (for consistency with MATLAB)
        sorted_pairs = sorted(zip(atom_labels, charges), key=lambda x: x[0])
        atom_labels = [pair[0] for pair in sorted_pairs]
        charges = [pair[1] for pair in sorted_pairs]
        
        # For each atom type/element in atom_labels, assign the specified charge
        for label, charge in zip(atom_labels, charges):
            for i, atom in enumerate(atoms):
                # Case-insensitive comparison with atom's type
                atom_type = atom.get('type', atom.get('element', ''))
                if atom_type.lower() == label.lower():
                    atom['charge'] = charge
                    metal_indices.append(i)
    
    # Find oxygen and fluorine atoms (filtered by resname if specified)
    ox_indices = [i for i in target_atoms if 'type' in atoms[i] and atoms[i]['type'].lower().startswith('o')]
    fs_indices = [i for i in target_atoms if 'type' in atoms[i] and atoms[i]['type'].lower().startswith('fs')]
    
    # Ensure we have bond information
    if 'bonds' not in atoms[0] and 'neigh' not in atoms[0]:
        # Calculate bonds if they aren't already present
        atoms, bond_index, angle_index = bond_angle(atoms, box_dim, rmaxH=1.2, rmaxM=2.45)
    
    # Process oxygen atoms
    for i in ox_indices:
        # Get bonded atom indices
        neighbors = atoms[i].get('neigh', [])
        if not neighbors:
            continue
        
        # Calculate charge based on neighbors
        zsum = 0.0
        for j in neighbors:
            # Determine formal charge of neighbor based on type
            atom_type = atoms[j].get('type', '').lower()
            
            # Determine formal valence Z
            if atom_type.startswith('si'):
                z = 4
            elif atom_type.startswith('al'):
                z = 3
            elif atom_type.startswith('fe2'):
                z = 2
            elif atom_type.startswith('fe'):
                z = 3
            elif atom_type.startswith('f'):  # Fs
                z = 3
            elif atom_type.startswith('ti'):
                z = 4
            elif atom_type.startswith('li'):
                z = 1
            elif atom_type.startswith('mg'):
                z = 2
            elif atom_type.startswith('ca'):
                z = 2
            elif atom_type.startswith('h'):
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
            
        # Set oxygen charge
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
            atom_type = atoms[j].get('type', '').lower()
            
            # Determine formal valence Z
            if atom_type.startswith('si'):
                z = 4
            elif atom_type.startswith('al'):
                z = 3
            elif atom_type.startswith('fe2'):
                z = 2
            elif atom_type.startswith('fe'):
                z = 3
            elif atom_type.startswith('ti'):
                z = 4
            elif atom_type.startswith('li'):
                z = 1
            elif atom_type.startswith('mg'):
                z = 2
            elif atom_type.startswith('ca'):
                z = 2
            elif atom_type.startswith('h'):
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
    
    # Calculate total charge (only for atoms matching the resname filter)
    if resname is not None:
        total_charge = sum(atoms[i].get('charge', 0) for i in target_atoms)
    else:
        total_charge = sum(atom.get('charge', 0) for atom in atoms)
    print(f"Total charge: {total_charge}")
    
    # Check if total charge is integer
    if round(total_charge) != total_charge:
        print("Warning: Non-integer total charge. Adjusting to nearest integer.")
        # Auto-balance to nearest integer
        atoms = balance_charges(atoms, round(total_charge))
    
    return atoms


def balance_charges(atoms, target_total_charge=None, resname=None, target_atoms=None):
    """
    Balance the charges of the atoms to reach an integer total charge.
    
    This function distributes any charge imbalance evenly across oxygen atoms
    to achieve an integer total charge, either specified or the nearest integer to the current total.
    
    Args:
        atoms: List of atom dictionaries with 'charge' field
        target_total_charge: Optional integer target charge. If None, will use the nearest integer to the current total.
        resname: Optional residue name to filter atoms by
        target_atoms: Optional list of atom indices to target for charge balancing
        
    Returns:
        The updated list of atoms with balanced charges
    """
    # Calculate current total charge
    if resname is not None:
        current_total = sum(atoms[i].get('charge', 0) for i in target_atoms)
    else:
        current_total = sum(atom.get('charge', 0) for atom in atoms)
    
    # If target_total_charge is None, use nearest integer to current total
    if target_total_charge is None:
        target_total_charge = round(current_total)
    
    # Find oxygen atoms to distribute charge correction (filtered by resname if specified)
    if resname is not None:
        ox_indices = [i for i in target_atoms if 'type' in atoms[i] and atoms[i]['type'].lower().startswith('o')]
    else:
        ox_indices = [i for i, atom in enumerate(atoms) if 'type' in atom and atom['type'].lower().startswith('o')]
    
    if ox_indices:
        # Calculate charge adjustment per oxygen atom
        charge_adjust = (target_total_charge - current_total) / len(ox_indices)
        
        # Apply adjustment
        for i in ox_indices:
            atoms[i]['charge'] += charge_adjust
            
        # Verify final charge
        if resname is not None:
            final_total = sum(atoms[i].get('charge', 0) for i in target_atoms)
        else:
            final_total = sum(atom.get('charge', 0) for atom in atoms)
        print(f"Final total charge: {final_total} (target was {target_total_charge})")
    else:
        print("Warning: No oxygen atoms found for charge balancing.")
    
    return atoms
