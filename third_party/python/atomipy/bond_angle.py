import numpy as np
from .distances import get_neighbor_list
from . import config

def bond_angle(atoms, Box, rmaxH=1.2, rmaxM=2.45, same_element_bonds=False, same_molecule_only=True, calculate_coordination=True, neighbor_element=None, dm_method=None, verbose=True):
    """
    Parameters
    ----------
    atoms : list of dict
        List of atom dictionaries containing Cartesian coordinates (keys: ``x``, ``y``, ``z``).
    Box : list or array-like
        Simulation cell dimensions.
    rmaxH : float, optional
        Cutoff distance (Å) for bonds involving hydrogen (default: 1.2).
    rmaxM : float, optional
        Cutoff distance (Å) for bonds between non-hydrogen atoms (default: 2.45).
    same_element_bonds : bool, optional
        If False, bonds between atoms of the same element are ignored (default: False).
    same_molecule_only : bool, optional
        If True, restrict bonds to atoms sharing the same ``molid`` (default: True).
    calculate_coordination : bool, optional
        If True, add coordination numbers to each atom dictionary (default: True).
    neighbor_element : str, optional
        If provided, coordination counts only neighbors of this element.
    dm_method : str, optional
        Manual selection of distance matrix method:
        - 'direct': Force O(N^2) direct distance calculation.
        - 'sparse': Force O(N) memory-efficient neighbor list.
        - 'fast_cl': Force O(N) memory-intensive Full Cell List.
        - None (default): Auto-select based on `config.SPARSE_THRESHOLD`.
    verbose : bool, optional
        If True, print the number of identified bonds (default: True).

    Performance Note
    ----------------
    This function uses the central dispatcher `get_neighbor_list` to automatically 
    switch between the Direct method (for small systems) and the Sparse Neighbor 
    List method (for systems >= `config.SPARSE_THRESHOLD`). Use `dm_method` to
    override this behavior.
    """
    # Get the number of atoms
    N = len(atoms)

    # Use the central dispatcher to get neighbors and distances
    # It automatically handles Direct vs Sparse based on config.SPARSE_THRESHOLD
    i_idx, j_idx, dists, dx_s, dy_s, dz_s = get_neighbor_list(
        atoms, 
        Box, 
        cutoff=max(rmaxH, rmaxM), 
        rmaxH=rmaxH, 
        dm_method=dm_method
    )

    # Initialize lists for all atoms
    for i in range(N):
        atoms[i]['neigh'] = []
        atoms[i]['bonds'] = []
        atoms[i]['angles'] = []
    
    # Process bonds and adjacency
    bond_pairs = []
    
    # Determine element types for filtering (if needed)
    types = np.array([atom.get('type', atom.get('name', '')) for atom in atoms])
    is_h = np.array([bool(t and t[0].upper() == 'H') for t in types])
    
    # Since rmaxH and rmaxM might differ, we need to filter the dispatcher results
    # which were calculated with max(rmaxH, rmaxM)
    cutoff_vec = np.where(is_h[i_idx] | is_h[j_idx], rmaxH, rmaxM)
    mask = (dists <= cutoff_vec)
    
    cand_i = i_idx[mask]
    cand_j = j_idx[mask]
    cand_dists = dists[mask]
    cand_dx = dx_s[mask]
    cand_dy = dy_s[mask]
    cand_dz = dz_s[mask]
    
    # Adjacency with vectors for angle calculation
    # neighbors_vecs[i] = list of (j, dx, dy, dz) where dx = x_j - x_i
    neighbors_vecs = [[] for _ in range(N)]

    # Filter bonds based on element types and molecule IDs
    for k in range(len(cand_i)):
        i, j = cand_i[k], cand_j[k]
        dist = cand_dists[k]
        
        el_i = atoms[i].get('element', 'X')
        el_j = atoms[j].get('element', 'X')
        molid_i = atoms[i].get('molid', None)
        molid_j = atoms[j].get('molid', None)
        
        # Apply element check and molecule check
        molecule_condition = True if not same_molecule_only else (molid_i == molid_j)
        element_condition = same_element_bonds or el_i != el_j
        
        if element_condition and molecule_condition:
            atoms[i]['neigh'].append(j)
            atoms[i]['bonds'].append((j, dist))
            atoms[j]['neigh'].append(i)
            atoms[j]['bonds'].append((i, dist))
            bond_pairs.append((i, j, dist))
            
            dx_ij = cand_dx[k]
            dy_ij = cand_dy[k]
            dz_ij = cand_dz[k]
            neighbors_vecs[i].append((j, dx_ij, dy_ij, dz_ij))
            neighbors_vecs[j].append((i, -dx_ij, -dy_ij, -dz_ij))

    if verbose:
        print(f"  Identified {len(bond_pairs)} valid bonds")

    # Calculate angles
    angle_data = []
    for i in range(N):
        if len(atoms[i]['neigh']) < 2:
            continue
            
        # Use neighbors_vecs for displacement lookup (available for all methods via dispatcher)
        for m in range(len(neighbors_vecs[i])):
            for n in range(m+1, len(neighbors_vecs[i])):
                j, dx_ij, dy_ij, dz_ij = neighbors_vecs[i][m]
                k, dx_ik, dy_ik, dz_ik = neighbors_vecs[i][n]
                rij = np.array([dx_ij, dy_ij, dz_ij])
                rik = np.array([dx_ik, dy_ik, dz_ik])
                    
                rij_norm = np.linalg.norm(rij)
                rik_norm = np.linalg.norm(rik)
                cos_angle = np.dot(rij, rik) / (rij_norm * rik_norm)
                cos_angle = max(min(cos_angle, 1.0), -1.0)
                angle = np.degrees(np.arccos(cos_angle))
                
                atoms[i]['angles'].append(((j, k), angle))
                # Standard Ordering (atom1 < atom3)
                if j < k:
                    atom1, atom3 = j, k
                    dx12, dy12, dz12 = dx_ij, dy_ij, dz_ij
                    dx23, dy23, dz23 = dx_ik, dy_ik, dz_ik
                else:
                    atom1, atom3 = k, j
                    dx12, dy12, dz12 = dx_ik, dy_ik, dz_ik
                    dx23, dy23, dz23 = dx_ij, dy_ij, dz_ij
                
                angle_data.append((atom1, i, atom3, angle, dx12, dy12, dz12, dx23, dy23, dz23))
    
    # Convert bond_pairs list to Nx3 numpy array
    Bond_index = np.array(bond_pairs)
    
    # For each bond, ensure the smaller atom index is in the first column
    if len(Bond_index) > 0:
        # This should already be taken care of when creating bond_pairs,
        # but let's make sure by doing a final check
        for i in range(len(Bond_index)):
            if Bond_index[i, 0] > Bond_index[i, 1]:
                # Swap indices to put smaller first
                Bond_index[i, 0], Bond_index[i, 1] = Bond_index[i, 1], Bond_index[i, 0]
        
        # Now sort rows based on first column (atom1_idx) and then second column (atom2_idx)
        sorted_indices = np.lexsort((Bond_index[:, 1], Bond_index[:, 0]))
        Bond_index = Bond_index[sorted_indices]
    
    # Convert angle_data list to Mx10 numpy array
    Angle_index = np.array(angle_data)
    
    # Sort Angle_index row-wise by atomic indices
    if len(Angle_index) > 0:
        # Sort the entire array: first by middle atom (column 1), 
        # then by lowest bonded atom (column 0), then by highest bonded atom (column 2)
        sorted_indices = np.lexsort((Angle_index[:, 2], Angle_index[:, 0], Angle_index[:, 1]))
        Angle_index = Angle_index[sorted_indices]
    
    # Calculate coordination numbers if requested
    if calculate_coordination:
        for i, atom in enumerate(atoms):
            neighbors = atom.get('neigh', [])
            
            # Filter neighbors by element if requested
            if neighbor_element and neighbors:
                neighbors = [idx for idx in neighbors if atoms[idx].get('element') == neighbor_element]
                
            atom['cn'] = len(neighbors)
    
    return atoms, Bond_index, Angle_index


def bond_angle_dihedral(atoms, Box, rmaxH=1.2, rmaxM=2.45, same_element_bonds=False,
                        same_molecule_only=True, calculate_coordination=True, neighbor_element=None, verbose=True):
    """
    Calculate bonds, angles, dihedrals, and 1-4 pair interactions.

    This mirrors the MATLAB bond_angle_dihedral_atom.m flow: bonds/angles are built first,
    then dihedrals are assembled from adjacency and refined using the angle vectors
    (including reversed orientations) to compute dihedral angles. The pairlist is derived
    from terminal atoms of the dihedrals with existing bonds removed.

    Parameters
    ----------
    atoms : list of dict
        List of atom dictionaries containing Cartesian coordinates (keys: ``x``, ``y``, ``z``).
    Box : list or array-like
        Simulation cell dimensions in one of the supported formats:
        - 1x3: [lx, ly, lz] for orthogonal boxes
        - 1x6: [a, b, c, alpha, beta, gamma] for cell parameters
        - 1x9: [lx, ly, lz, 0, 0, xy, 0, xz, yz] for GROMACS triclinic format
    rmaxH : float, optional
        Cutoff distance (Å) for bonds involving hydrogen (default: 1.2).
    rmaxM : float, optional
        Cutoff distance (Å) for bonds between non-hydrogen atoms (default: 2.45).
    same_element_bonds : bool, optional
        If False, bonds between atoms of the same element are ignored (default: False).
    same_molecule_only : bool, optional
        If True, restrict bonds to atoms sharing the same ``molid`` (default: True).
    calculate_coordination : bool, optional
        If True, add coordination numbers to each atom dictionary (default: True).
    neighbor_element : str, optional
        If provided, coordination counts only neighbors of this element.
    verbose : bool, optional
        If True, print the number of identified bonds (default: True).

    Returns
    -------
    atoms : list of dict
        Atom list updated with neighbor, bond, angle, and coordination information.
    Bond_index : numpy.ndarray
        Array with shape (n_bonds, 3) containing bonded atom indices (0-based) and distances.
    Angle_index : numpy.ndarray
        Array with shape (n_angles, 10) describing angles as returned by :func:`bond_angle`.
    Dihedral_index : numpy.ndarray
        Array with shape (n_dihedrals, 5): [atom1, atom2, atom3, atom4, dihedral_angle_deg].
    Pairlist : numpy.ndarray
        Array with shape (n_pairs, 2) listing unique 1-4 pairs not directly bonded.

    Raises
    ------
    ValueError
        If ``atoms`` is empty or ``Box`` is missing/invalid.
    """
    if atoms is None or len(atoms) == 0:
        raise ValueError("atoms list cannot be empty")

    if Box is None:
        raise ValueError("Box parameter must be provided")

    atoms, Bond_index, Angle_index = bond_angle(
        atoms,
        Box,
        rmaxH=rmaxH,
        rmaxM=rmaxM,
        same_element_bonds=same_element_bonds,
        same_molecule_only=same_molecule_only,
        calculate_coordination=calculate_coordination,
        neighbor_element=neighbor_element,
        verbose=verbose,
    )

    Bond_index = np.atleast_2d(np.array(Bond_index))
    Angle_index = np.atleast_2d(np.array(Angle_index))

    if Bond_index.size == 0 or Angle_index.size == 0:
        empty_dihedral = np.empty((0, 5))
        empty_pairlist = np.empty((0, 2), dtype=int)
        return atoms, Bond_index, Angle_index, empty_dihedral, empty_pairlist

    # Build adjacency from bonds
    nbonds = Bond_index.shape[0]
    adjacency = [[] for _ in range(len(atoms))]
    for i in range(nbonds):
        a = int(Bond_index[i, 0])
        b = int(Bond_index[i, 1])
        adjacency[a].append(b)
        adjacency[b].append(a)

    # Build oriented list using the same sparse/dense logic
    if len(atoms) >= config.SPARSE_THRESHOLD:
        from .distances import neighbor_list_fast
        i_idx, j_idx, dists, dx_s, dy_s, dz_s = neighbor_list_fast(atoms, Box, cutoff=max(rmaxH, rmaxM), rmaxH=rmaxH)
        # Create mapping for quick lookup: (i, j) -> (dx, dy, dz)
        # For dihedrals, we only care about bonded neighbors
        bond_vecs = {}
        for k in range(len(i_idx)):
            idx1, idx2 = i_idx[k], j_idx[k]
            bond_vecs[(idx1, idx2)] = (dx_s[k], dy_s[k], dz_s[k])
            bond_vecs[(idx2, idx1)] = (-dx_s[k], -dy_s[k], -dz_s[k])
        
        oriented_angles = []
        for center, neighs in enumerate(adjacency):
            if len(neighs) < 2: continue
            for n1 in neighs:
                for n2 in neighs:
                    if n1 == n2: continue
                    if (center, n1) not in bond_vecs or (center, n2) not in bond_vecs: continue
                    
                    v1 = np.array(bond_vecs[(center, n1)])
                    v2 = np.array(bond_vecs[(center, n2)])
                    norm1 = np.linalg.norm(v1)
                    norm2 = np.linalg.norm(v2)
                    if norm1 == 0 or norm2 == 0: continue
                    cos_angle = np.dot(v1, v2) / (norm1 * norm2)
                    cos_angle = max(min(cos_angle, 1.0), -1.0)
                    angle = np.degrees(np.arccos(cos_angle))
                    oriented_angles.append((n1, center, n2, angle, v1[0], v1[1], v1[2], v2[0], v2[1], v2[2]))
    else:
        # Standard dense approach for smaller systems
        from .distances import dist_matrix
        _, dx, dy, dz = dist_matrix(atoms, Box)
        oriented_angles = []
        for center, neighs in enumerate(adjacency):
            if len(neighs) < 2: continue
            for n1 in neighs:
                for n2 in neighs:
                    if n1 == n2: continue
                    v1 = np.array([dx[center, n1], dy[center, n1], dz[center, n1]])
                    v2 = np.array([dx[center, n2], dy[center, n2], dz[center, n2]])
                    norm1 = np.linalg.norm(v1)
                    norm2 = np.linalg.norm(v2)
                    if norm1 == 0 or norm2 == 0: continue
                    cos_angle = np.dot(v1, v2) / (norm1 * norm2)
                    cos_angle = max(min(cos_angle, 1.0), -1.0)
                    angle = np.degrees(np.arccos(cos_angle))
                    oriented_angles.append((n1, center, n2, angle, v1[0], v1[1], v1[2], v2[0], v2[1], v2[2]))
    oriented_angles = np.array(oriented_angles)
    if oriented_angles.size == 0:
        empty_dihedral = np.empty((0, 5))
        empty_pairlist = np.empty((0, 2), dtype=int)
        return atoms, Bond_index, Angle_index, empty_dihedral, empty_pairlist
    angle_indices = oriented_angles[:, :3].astype(int)

    # Prepare angle stack (forward + reversed) to compute dihedral angles
    angles_rev = np.zeros_like(oriented_angles)
    angles_rev[:, 0] = angle_indices[:, 2]
    angles_rev[:, 1] = angle_indices[:, 1]
    angles_rev[:, 2] = angle_indices[:, 0]
    angles_rev[:, 3] = oriented_angles[:, 3]
    angles_rev[:, 4:7] = oriented_angles[:, 7:10]
    angles_rev[:, 7:10] = oriented_angles[:, 4:7]

    angle_stack = np.vstack((angles_rev, oriented_angles))
    angle_stack_indices = angle_stack[:, :3].astype(int)

    dihedral_list = []
    for i in range(angle_stack.shape[0]):
        for j in range(i, angle_stack.shape[0]):
            if (angle_stack_indices[i, 1] == angle_stack_indices[j, 0] and
                    angle_stack_indices[i, 2] == angle_stack_indices[j, 1]):
                A = np.cross(angle_stack[i, 4:7], angle_stack[i, 7:10])
                B = np.cross(angle_stack[j, 4:7], angle_stack[j, 7:10])
                normA = np.linalg.norm(A)
                normB = np.linalg.norm(B)
                if normA < 1e-8 or normB < 1e-8:
                    continue
                cos_phi = np.dot(A, B) / (normA * normB)
                cos_phi = max(min(cos_phi, 1.0), -1.0)
                theta = float(np.round(np.degrees(np.arccos(cos_phi)), 2))

                if angle_stack_indices[i, 1] < angle_stack_indices[i, 2]:
                    dihedral = (
                        angle_stack_indices[i, 0],
                        angle_stack_indices[i, 1],
                        angle_stack_indices[i, 2],
                        angle_stack_indices[j, 2],
                        theta,
                    )
                else:
                    dihedral = (
                        angle_stack_indices[j, 2],
                        angle_stack_indices[i, 2],
                        angle_stack_indices[i, 1],
                        angle_stack_indices[i, 0],
                        theta,
                    )
                dihedral_list.append(dihedral)

    if dihedral_list:
        Dihedral_index = np.unique(np.array(dihedral_list, dtype=float), axis=0)
        sort_idx = np.lexsort((Dihedral_index[:, 2], Dihedral_index[:, 1]))
        Dihedral_index = Dihedral_index[sort_idx]
    else:
        Dihedral_index = np.empty((0, 5))

    # Pairlist from dihedral terminals, removing bonded pairs
    if len(Dihedral_index) > 0:
        Pairlist = np.sort(Dihedral_index[:, [0, 3]].astype(int), axis=1)
        Pairlist = np.unique(Pairlist, axis=0)
        if Bond_index.size > 0:
            bond_pairs = np.sort(Bond_index[:, :2].astype(int), axis=1)
            keep_mask = []
            for pair in Pairlist:
                if np.any(np.all(bond_pairs == np.sort(pair), axis=1)):
                    keep_mask.append(False)
                else:
                    keep_mask.append(True)
            Pairlist = Pairlist[np.array(keep_mask, dtype=bool)]
    else:
        Pairlist = np.empty((0, 2), dtype=int)

    return atoms, Bond_index, Angle_index, Dihedral_index, Pairlist
