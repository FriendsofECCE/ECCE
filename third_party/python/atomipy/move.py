"""
Move module for atomipy - provides functions for translating and rotating atoms.


This module contains functions to translate, rotate, and place atomic coordinates in atomipy data structures.
"""

import copy
import numpy as np


def translate(atoms, trans_vec, resname="all"):
    """
    Translate atom coordinates by a specified vector.
    
    
    Parameters
    ----------
    atoms : list of dict
        List of atom dictionaries with cartesian coordinates.
    trans_vec : list or array-like
        Translation vector [x, y, z] in Angstroms.
    resname : str, optional
        Residue name to translate. Default is "all" which translates all atoms.
        
    Returns
    -------
    atoms : list of dict
        The atoms list with updated coordinates.
        
    Examples
    --------
    # Translate all atoms by [1, 2, 3]:
    new_atoms = ap.translate(atoms, [1, 2, 3])
    
    # Only translate water molecules:
    new_atoms = ap.translate(atoms, [1, 2, 3], "SOL")
    """
    # Make a deep copy to avoid modifying the original
    atoms_copy = copy.deepcopy(atoms)
    
    # Convert translation vector to numpy array for consistency
    trans_vec = np.array(trans_vec, dtype=float)
    
    # Ensure we're working with a 3D vector
    if len(trans_vec) != 3:
        raise ValueError("Translation vector must have exactly 3 components (x, y, z).")
    
    # Determine which atoms to translate based on resname
    if resname.lower() == "all":
        # Translate all atoms
        for atom in atoms_copy:
            atom['x'] += trans_vec[0]
            atom['y'] += trans_vec[1]
            atom['z'] += trans_vec[2]
    else:
        # Translate only atoms with matching resname
        for atom in atoms_copy:
            if atom['resname'] == resname:
                atom['x'] += trans_vec[0]
                atom['y'] += trans_vec[1]
                atom['z'] += trans_vec[2]
    
    return atoms_copy


def rotate(atoms, Box=None, angles='random'):
    """
    Rotate atoms around their center of mass.
    
    Parameters
    ----------
    atoms : list of dict
        List of atom dictionaries with cartesian coordinates
    Box : list or array-like, optional
        Box dimensions (not used in basic rotation but included for compatibility)
    angles : list or str, optional
        Rotation angles [alpha, beta, gamma] in degrees, or 'random' for random angles
        
    Returns
    -------
    atoms : list of dict
        The atoms list with rotated coordinates
        
    Examples
    --------
    # Rotate atoms randomly:
    rotated_atoms = ap.rotate(atoms)
    
    # Rotate atoms by specific angles [30, 45, 90] degrees:
    rotated_atoms = ap.rotate(atoms, angles=[30, 45, 90])
    """
    # Import here to avoid circular imports
    from .mass import com
    
    # Make a deep copy to avoid modifying the original
    atoms_copy = copy.deepcopy(atoms)
    
    # Handle random rotation
    if angles == 'random':
        angles = [360 * np.random.random(),
                 360 * np.random.random(),
                 360 * np.random.random()]
    
    # Convert angles to radians
    alpha, beta, gamma = [np.radians(ang) for ang in angles]
    
    # Get center of mass
    center = com(atoms_copy, add_to_atoms=False)
    
    # Create rotation matrices
    rot_x = np.array([[1, 0, 0],
                      [0, np.cos(alpha), -np.sin(alpha)],
                      [0, np.sin(alpha), np.cos(alpha)]])
    
    rot_y = np.array([[np.cos(beta), 0, np.sin(beta)],
                      [0, 1, 0],
                      [-np.sin(beta), 0, np.cos(beta)]])
    
    rot_z = np.array([[np.cos(gamma), -np.sin(gamma), 0],
                      [np.sin(gamma), np.cos(gamma), 0],
                      [0, 0, 1]])
    
    # Combined rotation matrix (apply rotations in z-y-x order)
    rot = rot_z @ rot_y @ rot_x
    
    # Apply rotation to each atom
    for atom in atoms_copy:
        # Center coordinates at origin
        x = atom['x'] - center[0]
        y = atom['y'] - center[1]
        z = atom['z'] - center[2]
        
        # Rotate
        coords = np.array([x, y, z])
        new_coords = rot @ coords
        
        # Update coordinates
        atom['x'] = new_coords[0] + center[0]
        atom['y'] = new_coords[1] + center[1]
        atom['z'] = new_coords[2] + center[2]
    
    return atoms_copy


def place(atoms, position):
    """
    Place atoms at a specified position by moving their center of mass.
    
    Parameters
    ----------
    atoms : list of dict
        List of atom dictionaries with cartesian coordinates
    position : list or array-like
        Target position [x, y, z] for the center of mass
        
    Returns
    -------
    atoms : list of dict
        The atoms list with updated coordinates
        
    Examples
    --------
    # Place atoms with center of mass at [10, 10, 10]:
    placed_atoms = ap.place(atoms, [10, 10, 10])
    """
    # Import here to avoid circular imports
    from .mass import com
    
    # Make a deep copy to avoid modifying the original
    atoms_copy = copy.deepcopy(atoms)
    
    # Get current center of mass
    current_com = com(atoms_copy, add_to_atoms=False)
    
    # Calculate translation vector
    trans_vec = [position[0] - current_com[0],
                position[1] - current_com[1],
                position[2] - current_com[2]]
    
    # Apply translation
    return translate(atoms_copy, trans_vec)


def center(atoms, Box=None, resname="all", dim="xyz"):
    """
    Center atoms with respect to the box or specified residues along given dimensions.
    
    Parameters
    ----------
    atoms : list of dict
        List of atom dictionaries with cartesian coordinates
    Box : list or array-like, optional
        Box dimensions in any format supported by atomipy (1x3, 1x6, or 1x9)
    resname : str, optional
        Residue name to center. Default is "all" which centers all atoms.
    dim : str, optional
        Dimensions to center along, can be any combination of 'x', 'y', and 'z'.
        Default is 'xyz' which centers in all three dimensions.
        
    Returns
    -------
    atoms : list of dict
        The atoms list with updated coordinates
        
    Examples
    --------
    # Center all atoms in all dimensions:
    centered_atoms = ap.center(atoms, Box)
    
    # Center only water molecules in xy plane:
    centered_atoms = ap.center(atoms, Box, resname="SOL", dim="xy")
    
    # Center all atoms in z dimension only:
    centered_atoms = ap.center(atoms, Box, dim="z")
    """
    print("Centering")
    
    # Make a deep copy to avoid modifying the original
    atoms_copy = copy.deepcopy(atoms)
    
    # Determine which atoms to center based on resname
    if resname.lower() == "all":
        # Use all atoms
        atoms_to_center = atoms_copy
    else:
        # Filter atoms by resname
        atoms_to_center = [atom for atom in atoms_copy if atom.get('resname', '') == resname]
        
        if not atoms_to_center:
            print(f"Warning: No atoms found with resname '{resname}'. No centering performed.")
            return atoms_copy
    
    # Calculate geometric center of the atoms to be centered
    num_atoms = len(atoms_to_center)
    
    # Calculate geometric center
    center_x = sum(atom['x'] for atom in atoms_to_center) / num_atoms
    center_y = sum(atom['y'] for atom in atoms_to_center) / num_atoms
    center_z = sum(atom['z'] for atom in atoms_to_center) / num_atoms
    
    # Determine box center if box is provided
    if Box is not None:
        if len(Box) == 3:  # Orthogonal box
            box_center = [Box[0] / 2, Box[1] / 2, Box[2] / 2]
        elif len(Box) == 6:  # Cell parameters format
            # For simplicity, assume box center is halfway along each cell vector
            box_center = [Box[0] / 2, Box[1] / 2, Box[2] / 2]
        elif len(Box) == 9:  # GROMACS triclinic box format: [lx, ly, lz, 0, 0, xy, 0, xz, yz]
            # Use the actual box dimensions (first 3 elements), not the tilt factors
            box_center = [Box[0] / 2, Box[1] / 2, Box[2] / 2]
        else:
            print("Warning: Unrecognized box format. Using (0,0,0) as box center.")
            box_center = [0, 0, 0]
    else:
        # No box provided, center at origin
        box_center = [0, 0, 0]
    
    # Calculate translation vector components
    trans_x = 0
    trans_y = 0
    trans_z = 0
    
    if 'x' in dim.lower():
        trans_x = box_center[0] - center_x
    
    if 'y' in dim.lower():
        trans_y = box_center[1] - center_y
    
    if 'z' in dim.lower():
        trans_z = box_center[2] - center_z
    
    # Apply translation to all atoms or just the selected residue
    if resname.lower() == "all":
        # Translate all atoms
        for atom in atoms_copy:
            atom['x'] += trans_x
            atom['y'] += trans_y
            atom['z'] += trans_z
    else:
        # Translate only atoms with matching resname
        for atom in atoms_copy:
            if atom.get('resname', '') == resname:
                atom['x'] += trans_x
                atom['y'] += trans_y
                atom['z'] += trans_z
    
    return atoms_copy

def bend(atoms, radius, axis='y', center_z=None):
    """
    Bend a planar structure into a cylinder.
    
    This function transforms flat coordinates into cylindrical ones. 
    Assuming the slab is in the XY plane, it bends it around the specified axis.
    
    Parameters
    ----------
    atoms : list of dict
        List of atom dictionaries.
    radius : float
        Radius of the cylinder to bend into.
    axis : str, optional
        The axis to bend around ('x' or 'y'). Default is 'y'.
    center_z : float, optional
        The z-coordinate that will be exactly at the specified radius.
        If None, the average z of atoms is used.
        
    Returns
    -------
    list of dict
        Atoms with bent coordinates.
    """
    atoms_copy = copy.deepcopy(atoms)
    
    z_coords = np.array([a['z'] for a in atoms_copy])
    if center_z is None:
        center_z = np.mean(z_coords)
        
    for atom in atoms_copy:
        # Distance from the bending center in the z-direction
        z_offset = atom['z'] - center_z
        r = radius + z_offset
        
        if axis.lower() == 'y':
            # Bend X into theta
            theta = atom['x'] / radius
            atom['x'] = r * np.sin(theta)
            atom['z'] = r * np.cos(theta)
            # Y remains same
        elif axis.lower() == 'x':
            # Bend Y into theta
            theta = atom['y'] / radius
            atom['y'] = r * np.sin(theta)
            atom['z'] = r * np.cos(theta)
            # X remains same
            
    return atoms_copy
