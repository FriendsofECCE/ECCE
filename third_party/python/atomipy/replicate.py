"""
This module provides functions for replicating atomic structures along unit Cell dimensions.

The main function converts atoms to fractional coordinates, replicates the structure
along the unit Cell vectors, and converts back to cartesian coordinates with updated
Box dimensions.
"""

import copy
from .transform import (
    cartesian_to_fractional, fractional_to_cartesian, get_cell_vectors,
    direct_fractional_to_cartesian
)
from .cell_utils import Box_dim2Cell, Cell2Box_dim
from . import write_conf # Import for debug writing

def replicate_system(atoms, Box, replicate=[1, 1, 1], keep_molid=True, 
                   keep_resname=True, renumber_index=True):
    """
    Replicates a unit Cell along specified directions using crystallographic approach.
    
    This function implements the standard crystallographic replication process:
    1. Convert input coordinates to fractional coordinates (unit cube)
    2. Stack the unit Cell n1×n2×n3 times in a,b,c directions
    3. Scale the Cell parameters accordingly (preserving angles)
    4. Convert fractional coordinates back to cartesian
    
    Parameters
    ----------
    atoms : list of dict
        List of atom dictionaries with cartesian coordinates.
    Box : a 1x6 or 1x9 list representing Cell dimensions (in Angstroms), either as 
            a Cell variable having Cell parameters array [a, b, c, alpha, beta, gamma], or as 
            a Box_dim variable having Box dimensions [lx, ly, lz, 0, 0, xy, 0, xz, yz] for triclinic cells.
            Note that for orthogonal boxes Cell = Box_dim.
    replicate : list or array of length 3, optional
        Number of replications in the a, b, c directions. Default is [1, 1, 1].
    keep_molid : bool, optional
        If True, keeps the original molecule IDs for all replicas. Default is True.
    keep_resname : bool, optional
        If True, keeps the original residue names for all replicas. Default is True.
    renumber_index : bool, optional
        If True, renumbers atom indices sequentially. Default is True.
        
    Returns
    -------
    new_atoms : list of dict
        The replicated atoms list with updated coordinates.
    new_Box_dim : list
        The updated Box dimensions for the replicated Cell.
    new_cell : list
        The replicated Cell parameters as [a, b, c, alpha, beta, gamma].
        
    Examples
    --------
    # Replicate 2x2x1 using Cell parameters:
    new_atoms, new_Box_dim, new_Cell = ap.replicate.replicate_system(
        atoms, Box=[10, 10, 10, 90, 90, 90], replicate=[2, 2, 1]
    )
    
    # Replicate 2x2x2 with Box dimensions and assign new molecule IDs:
    new_atoms, new_Box_dim, new_Cell = ap.replicate.replicate_system(
        atoms, Box=[10, 10, 10], replicate=[2, 2, 2], keep_molid=False
    )
    """

    # Possibly convert Box_dim into [a,b,c,alpha,beta,gamma] form
    Box_dim = None
    Cell = None
    if Box is not None:
        if len(Box) == 9:
            Box_dim = Box 
            # Convert from Box_dim format to Cell format
            Cell = Box_dim2Cell(Box_dim)
        elif len(Box) == 6:
            Cell = Box
        elif len(Box) == 3:
            # Orthogonal Box
            Cell = list(Box) + [90.0, 90.0, 90.0]
            Box_dim = Box

    if Box_dim is None and Cell is None:
        raise ValueError("Either Box_dim or Cell must be provided")
    if Cell is None:
        raise ValueError("Could not determine Cell parameters from Box")
    
    # Handle integer input for replicate
    if isinstance(replicate, int):
        replicate = [replicate, replicate, replicate]
    
    # Ensure replicate is a list/array of length 3
    if len(replicate) != 3:
        raise ValueError("replicate must be a list/array of length 3")
    
    # Calculate Box dimensions from Cell parameters (for coordinate conversion)
    Box_dim = Cell2Box_dim(Cell)
    
    # Step 2: Convert to fractional coordinates (unit cube)
    # Using the new fract.py module which converts through orthogonal coordinates
    frac_coords, atoms_with_frac = cartesian_to_fractional(atoms, Box=Cell, add_to_atoms=True)
    
    # Step 3: Create storage for replicated atoms
    replicated_atoms = []
    
    # Find maximum values for renumbering
    max_index = max([atom.get('index', 0) for atom in atoms], default=0)
    max_molid = max([atom.get('molid', 0) for atom in atoms], default=0)
    max_resid = max([atom.get('resid', 0) for atom in atoms], default=0)
    
    # Store original cartesian coordinates for each atom (to handle special cases)
    original_coords = {}
    for i, atom in enumerate(atoms):
        original_coords[i] = {
            'x': atom.get('x', 0.0),
            'y': atom.get('y', 0.0),
            'z': atom.get('z', 0.0)
        }
    
    # --- Step 4: Sequential Replication --- 
    current_replication = copy.deepcopy(atoms_with_frac)
    num_atoms_base = len(atoms)
    max_idx_base = max_index
    max_molid_base = max_molid
    max_resid_base = max_resid
    
    # Stage 1: Replicate along X (a-vector)
    if replicate[0] > 1:
        replicated_x = []
        atoms_to_replicate_x = copy.deepcopy(current_replication)
        num_atoms_stage = len(atoms_to_replicate_x)
        max_idx_stage = max((atom.get('index', 0) for atom in atoms_to_replicate_x), default=-1)
        max_molid_stage = max((atom.get('molid', 0) for atom in atoms_to_replicate_x), default=-1)
        max_resid_stage = max((atom.get('resid', 0) for atom in atoms_to_replicate_x), default=-1)
        
        for i in range(replicate[0]):
            replica = copy.deepcopy(atoms_to_replicate_x)
            offset_idx = i * (max_idx_stage + 1)
            offset_molid = i * (max_molid_stage + 1)
            offset_resid = i * (max_resid_stage + 1)
            
            for atom in replica:
                atom['xfrac'] += i
                if renumber_index and 'index' in atom:
                    atom['index'] += offset_idx
                if not keep_molid and 'molid' in atom:
                    atom['molid'] += offset_molid
                if 'resid' in atom:
                    atom['resid'] += offset_resid
            replicated_x.extend(replica)
        current_replication = replicated_x

    # Stage 2: Replicate along Y (b-vector)
    if replicate[1] > 1:
        replicated_xy = []
        atoms_to_replicate_y = copy.deepcopy(current_replication) # Result from Stage 1
        num_atoms_stage = len(atoms_to_replicate_y)
        max_idx_stage = max((atom.get('index', 0) for atom in atoms_to_replicate_y), default=-1)
        max_molid_stage = max((atom.get('molid', 0) for atom in atoms_to_replicate_y), default=-1)
        max_resid_stage = max((atom.get('resid', 0) for atom in atoms_to_replicate_y), default=-1)

        for j in range(replicate[1]):
            replica = copy.deepcopy(atoms_to_replicate_y)
            offset_idx = j * (max_idx_stage + 1)
            offset_molid = j * (max_molid_stage + 1)
            offset_resid = j * (max_resid_stage + 1)
            
            for atom in replica:
                atom['yfrac'] += j
                if renumber_index and 'index' in atom:
                    atom['index'] += offset_idx
                if not keep_molid and 'molid' in atom:
                    atom['molid'] += offset_molid
                if 'resid' in atom:
                    atom['resid'] += offset_resid
            replicated_xy.extend(replica)
        current_replication = replicated_xy

    # Stage 3: Replicate along Z (c-vector)
    if replicate[2] > 1:
        replicated_xyz = []
        atoms_to_replicate_z = copy.deepcopy(current_replication) # Result from Stage 2
        num_atoms_stage = len(atoms_to_replicate_z)
        max_idx_stage = max((atom.get('index', 0) for atom in atoms_to_replicate_z), default=-1)
        max_molid_stage = max((atom.get('molid', 0) for atom in atoms_to_replicate_z), default=-1)
        max_resid_stage = max((atom.get('resid', 0) for atom in atoms_to_replicate_z), default=-1)

        for k in range(replicate[2]):
            replica = copy.deepcopy(atoms_to_replicate_z)
            offset_idx = k * (max_idx_stage + 1)
            offset_molid = k * (max_molid_stage + 1)
            offset_resid = k * (max_resid_stage + 1)
            
            for atom in replica:
                atom['zfrac'] += k
                if renumber_index and 'index' in atom:
                    atom['index'] += offset_idx
                if not keep_molid and 'molid' in atom:
                    atom['molid'] += offset_molid
                if 'resid' in atom:
                    atom['resid'] += offset_resid
            replicated_xyz.extend(replica)
        current_replication = replicated_xyz

    replicated_atoms = current_replication # Final result after all stages
    # --------------------------------------
    
    # Step 5: Create replicated Cell by scaling the original Cell parameters
    new_cell = [
        Cell[0] * replicate[0],  # a - scale by replication in x
        Cell[1] * replicate[1],  # b - scale by replication in y
        Cell[2] * replicate[2],  # c - scale by replication in z
        Cell[3],                 # alpha - angles remain unchanged
        Cell[4],                 # beta - angles remain unchanged
        Cell[5]                  # gamma - angles remain unchanged
    ]
    
    # Step 6: Generate new Box dimensions from the replicated Cell
    new_Box_dim = Cell2Box_dim(new_cell)
    
    # Step 7: For triclinic cells, we need to be careful with the coordinate transformation
    # to preserve atomic planes
    
    # First, convert the fractional coordinates to integers (which unit Cell they belong to)
    # and offsets within the unit Cell (0-1 range)
    for atom in replicated_atoms:
        # Calculate which unit Cell this atom belongs to in each direction
        ix = int(atom['xfrac'])
        iy = int(atom['yfrac'])
        iz = int(atom['zfrac'])
        
        # Calculate the fractional offset within that unit Cell (0-1 range)
        x_offset = atom['xfrac'] - ix
        y_offset = atom['yfrac'] - iy
        z_offset = atom['zfrac'] - iz
        
        # For proper replication that preserves planes, we calculate the new position
        # using the unit Cell indices and the original fractional coordinates
        atom['xfrac'] = (ix + x_offset) / replicate[0]
        atom['yfrac'] = (iy + y_offset) / replicate[1] 
        atom['zfrac'] = (iz + z_offset) / replicate[2]
    
    # Step 8: Convert replicated atoms back to cartesian coordinates
    # using the new (scaled) Cell parameters directly for crystallographic accuracy
    cart_coords = direct_fractional_to_cartesian(replicated_atoms, Box=new_cell, add_to_atoms=True)
    
    # Clean up temporary attributes used during replication
    for atom in replicated_atoms:
        if '_original_idx' in atom:
            del atom['_original_idx']
            
    # Ensure sequential 1-based indices if requested (fixing off-by-one gaps in stages)
    if renumber_index:
        for idx, atom in enumerate(replicated_atoms, start=1):
            atom['index'] = idx
    
    # As a final check, recalculate Cell parameters from Box dimensions to ensure consistency
    # This is redundant but serves as a sanity check
    new_cell = Box_dim2Cell(new_Box_dim)
    
    # Round Box dimensions to 5 decimal places
    new_Box_dim = [round(float(val), 5) for val in new_Box_dim]
    
    # Calculate new Cell parameters from new Box dimensions
    new_cell = Box_dim2Cell(new_Box_dim)
    
    # Return the replicated atoms, new Box dimensions, and new Cell parameters
    return replicated_atoms, new_Box_dim, new_cell


def replicate_atom(atoms, Box, replicate=[1, 1, 1], dim_order='xyz', 
                  add_molid=False, renumber_index=True):
    """
    Legacy function for compatibility with old replicate_atom functionality.
    
    This function calls replicate_system internally but maintains the same interface
    as the original replicate_atom function.
    
    Parameters
    ----------
    atoms : list of dict
        List of atom dictionaries with cartesian coordinates.
    Box : a 1x3, 1x6, or 1x9 list representing Cell dimensions (in Angstroms).
    replicate : list or array of length 3, optional
        Number of replications in the a, b, c directions. Default is [1, 1, 1].
    dim_order : str, optional
        Order of replication (e.g., 'xyz', 'yxz'). Default is 'xyz'.
        Note: This parameter is accepted for backward compatibility but is ignored.
        The replication is always performed in all three dimensions simultaneously.
    add_molid : bool, optional
        If True, adds new molecule IDs for each replica. Default is False.
    renumber_index : bool, optional
        If True, renumbers atom indices sequentially. Default is True.
        
    Returns
    -------
    new_atoms : list of dict
        The replicated atoms list with updated coordinates.
        
    Notes
    -----
    The dim_order parameter is accepted for backward compatibility but is ignored.
    The replication is always performed in all three dimensions simultaneously.
    """
    if Box is None:
        raise ValueError("Box must be provided for replication")

    # Call replicate_system with the appropriate parameters
    replicated_atoms, new_Box_dim, _ = replicate_system(
        atoms=atoms,
        Box=Box,
        replicate=replicate,
        keep_molid=not add_molid,
        keep_resname=True,
        renumber_index=renumber_index
    )
    
    # For backward compatibility, only return the atoms
    return replicated_atoms


def update_atom_indices(atoms):
    """
    Update atom indices to be sequential.
    
    Parameters
    ----------
    atoms : list of dict
        List of atom dictionaries.
        
    Returns
    -------
    atoms : list of dict
        The atoms list with updated indices.
    """
    for i, atom in enumerate(atoms):
        atom['index'] = i + 1
    return atoms
