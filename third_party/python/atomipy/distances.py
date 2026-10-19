import numpy as np

from .cell_utils import normalize_box
from . import config

# Try to import tqdm for progress bar
try:
    from tqdm import tqdm
    has_tqdm = True
except ImportError:
    has_tqdm = False

def _images_needed(Cell, cutoff):
    """Return (nx, ny, nz) image counts needed to capture all neighbours
    within `cutoff` Angstrom of an atom in the home cell."""
    from .cell_utils import normalize_box
    
    Box_dim, _ = normalize_box(Cell)
    if Box_dim is None:
        raise ValueError("Cell parameter must not be None")
    if len(Box_dim) == 3:
        # Orthogonal cell: pad to 9 elements with zero off-diagonals
        lx, ly, lz = Box_dim
        xy = xz = yz = 0.0
    elif len(Box_dim) == 9:
        lx, ly, lz = Box_dim[0], Box_dim[1], Box_dim[2]
        xy, xz, yz = Box_dim[5], Box_dim[7], Box_dim[8]
    else:
        raise ValueError(
            f"normalize_box returned unexpected length {len(Box_dim)}; "
            f"expected 3 or 9"
        )
    
    H = np.array([
        [lx, 0,  0 ],
        [xy, ly, 0 ],
        [xz, yz, lz]
    ], dtype=np.float64)
    Hinv = np.linalg.inv(H)
    return tuple(max(1, int(np.ceil(cutoff * np.linalg.norm(Hinv[i])))) for i in range(3)), Hinv

def dist_matrix(atoms, Box, cutoff=None):
    """Calculate the distance matrix between atoms following the MATLAB implementation approach.
    
    This function closely mimics the behavior of the MATLAB dist_matrix_MATLAB.m function,
    calculating distances with periodic boundary conditions in a per-atom loop approach.
    
    Args:
        atoms: list of atom dictionaries, each having 'x', 'y', 'z' coordinates.
        Box: a 1x3, 1x6 or 1x9 list representing Cell dimensions (in Angstroms):
            - For orthogonal boxes, a 1x3 list [lx, ly, lz] where Box = Box_dim, and Cell would be [lx, ly, lz, 90, 90, 90]
            - For Cell parameters, a 1x6 list [a, b, c, alpha, beta, gamma] (Cell format)
            - For triclinic boxes, a 1x9 list [lx, ly, lz, 0, 0, xy, 0, xz, yz] (GROMACS Box_dim format)
        cutoff: Optional maximum distance. If provided and Box is too small to hold all images, 
            it is highly recommended to use get_neighbor_list with dm_method='sparse' instead.
       
    Returns:
        A tuple of four numpy arrays: 
        - A numpy array of shape (N, N) with pairwise distances.
        - Three numpy arrays of shape (N, N) with pairwise x, y, z differences.
        
    Note:
        This implementation follows the approach in the MATLAB dist_matrix_MATLAB.m function,
        using per-atom iteration. Dense matrices cannot represent multiple periodic images of the same atom pair.
    """
    
    if Box is None:
        raise ValueError("Box parameter must be provided")
    
    # Determine Box format and convert as needed
    Box_dim, Cell = normalize_box(Box)
    assert Box_dim is not None  # Box is validated above
    
    # Extract Box dimensions
    if len(Box_dim) == 3:
        # Orthogonal Box
        lx, ly, lz = Box_dim
        xy, xz, yz = 0, 0, 0
    elif len(Box_dim) == 9:
        # Triclinic Box in GROMACS format [lx, ly, lz, 0, 0, xy, 0, xz, yz]
        lx, ly, lz = Box_dim[0], Box_dim[1], Box_dim[2]
        xy, xz, yz = Box_dim[5], Box_dim[7], Box_dim[8]
    else:
        raise ValueError(f"Unexpected Box_dim length {len(Box_dim)}; expected 3 or 9")
    
    # Extract atomic positions
    n_atoms = len(atoms)
    xyz = np.array([[atom['x'], atom['y'], atom['z']] for atom in atoms], dtype=np.float32)

    
    # Initialize output arrays
    distances = np.zeros((n_atoms, n_atoms), dtype=np.float32)
    dx = np.zeros((n_atoms, n_atoms), dtype=np.float32)
    dy = np.zeros((n_atoms, n_atoms), dtype=np.float32)
    dz = np.zeros((n_atoms, n_atoms), dtype=np.float32)

    # Calculate distance matrix
    last_percent = -1
    if len(Box_dim) == 3:
        # Orthogonal Box approach
        # Setup progress bar
        if has_tqdm:
            atom_iterator = tqdm(range(n_atoms), desc="Finding dists", unit="atom")
        else:
            print("Finding distances...")
            atom_iterator = range(n_atoms)
            
        for i in atom_iterator:
            # Update progress percentage for non-tqdm case
            if not has_tqdm and n_atoms > 100:
                percent = int(100 * i / n_atoms)
                if percent > last_percent and percent % 10 == 0:
                    print(f"  {percent}% complete...")
                    last_percent = percent
            # Calculate distance components
            rx = xyz[i, 0] - xyz[:, 0]
            ry = xyz[i, 1] - xyz[:, 1]
            rz = xyz[i, 2] - xyz[:, 2]
            
            # Apply minimum image convention for orthogonal Box
            rx[rx > lx/2] -= lx
            rx[rx < -lx/2] += lx
            
            ry[ry > ly/2] -= ly
            ry[ry < -ly/2] += ly
            
            rz[rz > lz/2] -= lz
            rz[rz < -lz/2] += lz
            
            # Calculate distances
            r = np.sqrt(rx**2 + ry**2 + rz**2)
            
            # Store results
            distances[:, i] = r
            dx[:, i] = -rx  # Note the negative sign to match MATLAB implementation
            dy[:, i] = -ry
            dz[:, i] = -rz
    else:
        # Triclinic Box approach
        # Setup progress bar
        if has_tqdm:
            atom_iterator = tqdm(range(n_atoms), desc="Finding dists", unit="atom")
        else:
            print("Finding distances...")
            atom_iterator = range(n_atoms)
            
        for i in atom_iterator:
            # Update progress percentage for non-tqdm case
            if not has_tqdm and n_atoms > 100:
                percent = int(100 * i / n_atoms)
                if percent > last_percent and percent % 10 == 0:
                    print(f"  {percent}% complete...")
                    last_percent = percent
            # Calculate initial distance components
            rx = xyz[i, 0] - xyz[:, 0]
            ry = xyz[i, 1] - xyz[:, 1]
            rz = xyz[i, 2] - xyz[:, 2]
            
            # Apply minimum image convention for triclinic Box
            # First handle z-direction
            z_gt_ind = rz > lz/2
            z_lt_ind = rz < -lz/2
            
            rz[z_gt_ind] -= lz
            rz[z_lt_ind] += lz
            
            rx[z_gt_ind] -= xz
            rx[z_lt_ind] += xz
            
            ry[z_gt_ind] -= yz
            ry[z_lt_ind] += yz
            
            # Then handle y-direction
            y_gt_ind = ry > ly/2
            y_lt_ind = ry < -ly/2
            
            ry[y_gt_ind] -= ly
            ry[y_lt_ind] += ly
            
            rx[y_gt_ind] -= xy
            rx[y_lt_ind] += xy
            
            # Finally handle x-direction
            x_gt_ind = rx > lx/2
            x_lt_ind = rx < -lx/2
            
            rx[x_gt_ind] -= lx
            rx[x_lt_ind] += lx
            
            # Calculate distances
            r = np.sqrt(rx**2 + ry**2 + rz**2)
            
            # Store results
            distances[:, i] = r
            dx[:, i] = -rx  # Note the negative sign to match MATLAB implementation
            dy[:, i] = -ry
            dz[:, i] = -rz
    
    # Transpose distances to match MATLAB output format
    distances = distances.T
    dx = dx.T
    dy = dy.T
    dz = dz.T
    
    return distances, dx, dy, dz

def dist_matrix_direct(atoms):
    """Calculate a direct distance matrix between atoms without periodic boundaries.
    
    Args:
        atoms: list of atom dictionaries, each having 'x', 'y', 'z' coordinates.
       
    Returns:
        A tuple of four numpy arrays: 
        - A numpy array of shape (N, N) with pairwise distances.
        - Three numpy arrays of shape (N, N) with pairwise x, y, z differences.
    """
    # Extract atomic positions
    n_atoms = len(atoms)
    positions = np.array([[atom['x'], atom['y'], atom['z']] for atom in atoms])
    
    # Calculate direct distances
    distances = np.zeros((n_atoms, n_atoms))
    dx = np.zeros((n_atoms, n_atoms))
    dy = np.zeros((n_atoms, n_atoms))
    dz = np.zeros((n_atoms, n_atoms))
    
    for i in range(n_atoms):
        rx = positions[i, 0] - positions[:, 0]
        ry = positions[i, 1] - positions[:, 1]
        rz = positions[i, 2] - positions[:, 2]
        
        r = np.sqrt(rx**2 + ry**2 + rz**2)
        
        distances[i, :] = r
        dx[i, :] = rx
        dy[i, :] = ry
        dz[i, :] = rz
    
    return distances, dx, dy, dz

def dist_matrix_hybrid(atoms, Box=None, use_pbc=True):
    """Calculate the distance matrix using either direct or PBC approach based on use_pbc flag.
    
    Args:
        atoms: list of atom dictionaries, each having 'x', 'y', 'z' coordinates.
        Box: a 1x3, 1x6 or 1x9 list representing Cell dimensions (in Angstroms):
            - For orthogonal boxes, a 1x3 list [lx, ly, lz]
            - For Cell parameters, a 1x6 list [a, b, c, alpha, beta, gamma] (Cell format)
            - For triclinic boxes, a 1x9 list [lx, ly, lz, 0, 0, xy, 0, xz, yz] (Box_dim format)
        use_pbc: Whether to use periodic boundary conditions (default: True).
       
    Returns:
        A tuple of four numpy arrays: 
        - A numpy array of shape (N, N) with pairwise distances.
        - Three numpy arrays of shape (N, N) with pairwise x, y, z differences.
    """
    if use_pbc:
        if Box is None:
            raise ValueError("Box parameter must be provided when use_pbc=True")
        return dist_matrix(atoms, Box)
    else:
        return dist_matrix_direct(atoms)

def get_neighbor_list(atoms, Box, cutoff, rmaxH=None, dm_method=None):
    """
    Central dispatcher for finding atom pairs within a cutoff distance.
    Automatically chooses between Direct (O(N^2)) and Sparse (O(N)) methods
    based on the system size and config.SPARSE_THRESHOLD.

    Args:
        atoms: list of atom dictionaries.
        Box: a 1x3, 1x6 or 1x9 list representing Cell dimensions.
        cutoff: maximum distance for non-hydrogen bonds.
        rmaxH: cutoff distance for hydrogen bonds. If None, uses cutoff.
        dm_method: optional override ('direct' or 'sparse').
        
    Returns:
        i_idx, j_idx (numpy.ndarray): Pairs of atom indices.
        dist (numpy.ndarray): Pairwise distances.
        dx, dy, dz (numpy.ndarray): Displacement components (r_j - r_i).
    """
    if rmaxH is None:
        rmaxH = cutoff

    n_atoms = len(atoms)
    
    # Decision logic
    is_sparse = False
    max_cutoff = max(cutoff, rmaxH)
    
    if Box is not None:
        from .cell_utils import normalize_box
        Box_dim, _ = normalize_box(Box)
        assert Box_dim is not None  # Box is not None in this branch
        _, Hinv = _images_needed(Box, max_cutoff)
        needs_multiple_images = any(1.0 / np.linalg.norm(Hinv[i]) < 2 * max_cutoff for i in range(3))
        # Skewed boxes cannot use the sequential MIC of the direct method reliably
        is_skewed = len(Box_dim) == 9 and any(abs(Box_dim[idx]) > 1e-6 for idx in [5, 7, 8])
    else:
        needs_multiple_images = False
        is_skewed = False

    if dm_method == 'sparse':
        is_sparse = True
    elif dm_method == 'direct':
        is_sparse = False
        if needs_multiple_images or is_skewed:
            # Force sparse if we need multiple images (dense NxN matrix can't hold them)
            # or if the box is skewed (MIC approximations fail)
            is_sparse = True
    elif n_atoms >= config.SPARSE_THRESHOLD or needs_multiple_images or is_skewed:
        is_sparse = True

    if is_sparse:
        # Use the optimized sparse neighbor list from cell_list_dist_matrix
        return neighbor_list_fast(atoms, Box, cutoff, rmaxH)
    else:
        # Use the direct distance matrix and convert to sparse format
        dmat, dx_mat, dy_mat, dz_mat = dist_matrix(atoms, Box)
        
        # Masking by cutoff (handling H vs non-H)
        types = np.array([atom.get('type', atom.get('name', '')) for atom in atoms])
        is_h = np.array([bool(t and t[0].upper() == 'H') for t in types])
        
        # Create a matrix of cutoffs
        cutoffs = np.where(is_h[:, np.newaxis] | is_h[np.newaxis, :], rmaxH, cutoff)
        
        mask = (dmat > 1e-7) & (dmat <= cutoffs)
        # Use upper triangle to avoid double counting
        upper_tri_mask = np.triu(np.ones((n_atoms, n_atoms), dtype=bool), k=1)
        mask = mask & upper_tri_mask
        
        i_idx, j_idx = np.where(mask)
        return (i_idx.astype(np.int32), 
                j_idx.astype(np.int32), 
                dmat[i_idx, j_idx].astype(np.float32), 
                dx_mat[i_idx, j_idx].astype(np.float32), 
                dy_mat[i_idx, j_idx].astype(np.float32), 
                dz_mat[i_idx, j_idx].astype(np.float32))

def get_progress_iterator(iterable, desc="Processing", unit="it"):
    if has_tqdm:
        return tqdm(iterable, desc=desc, unit=unit)
    return iterable

def cell_list_dist_matrix(atoms, Box, cutoff=2.45, rmaxH=1.2, H_type='H'):
    """
    Higly optimized Cell-list algorithm for finding atom pairs within a cutoff.
    Vectorized using NumPy to avoid nested Python loops. Supports Triclinic boxes.
    
    Args:
        atoms: list of atom dictionaries.
        Box: a 1x3, 1x6 or 1x9 list representing Cell dimensions.
        cutoff: maximum distance for non-hydrogen bonds.
        rmaxH: cutoff distance for hydrogen bonds.
        H_type: atom type string for hydrogen.
        
    Returns:
        dist_matrix: NxN numpy array (sparse-ish, mostly zeros).
        X_dist, Y_dist, Z_dist: NxN displacement component matrices.
        bond_list: Mx2 numpy array of atom indices.
        dist_list: Mx1 numpy array of distances.
    """
    N = len(atoms)
    positions = np.array([[atom['x'], atom['y'], atom['z']] for atom in atoms], dtype=np.float32)
    types = np.array([atom.get('type', '') for atom in atoms])
    is_h = np.array([bool(t and t[0].upper() == 'H') for t in types])
    
    if Box is None:
        raise ValueError("Box parameter must be provided")

    Box_dim, Cell = normalize_box(Box)
    assert Box_dim is not None and Cell is not None  # Box is validated above
    
    # Construct triclinic Box matrix H and its inverse Hinv
    a, b, c = Cell[0], Cell[1], Cell[2]
    if len(Cell) == 3:
        alpha, beta, gamma = 90.0, 90.0, 90.0
    else:
        alpha, beta, gamma = Cell[3], Cell[4], Cell[5]
        
    ar, br, gr = np.radians([alpha, beta, gamma])
    
    # Upper triangular lattice matrix H
    ax = a
    bx = b * np.cos(gr)
    by = b * np.sin(gr)
    cx = c * np.cos(br)
    cy = c * (np.cos(ar) - np.cos(br) * np.cos(gr)) / np.sin(gr)
    cz = np.sqrt(max(0, c**2 - cx**2 - cy**2))
    
    H = np.array([[ax, bx, cx], [0, by, cy], [0, 0, cz]], dtype=np.float32)
    Hinv = np.linalg.inv(H)
    
    # 1. Bin atoms into cells
    # We use fractional coordinates for easy binning in triclinic systems
    frac_coords = (Hinv @ positions.T).T
    frac_coords = frac_coords % 1.0
    positions = (H @ frac_coords.T).T  # Wrap positions to principal box
    
    # Cell size should be at least the cutoff
    max_cutoff = max(cutoff, rmaxH)
    
    # Effective box widths in each direction
    # For triclinic, we use the perpendicular widths (projections)
    # Using cz, by, ax is a safe approximation for orthogonal-like cells
    n_cells = np.maximum(np.floor(np.array([ax, by, cz]) / max_cutoff), 1).astype(int)
    
    # Map fractional coords to integer cell indices
    cell_idx = np.floor(frac_coords * n_cells).astype(int)
    cell_idx = np.clip(cell_idx, 0, n_cells - 1)
    
    # Flat cell index
    flat_idx = cell_idx[:, 0] * (n_cells[1] * n_cells[2]) + cell_idx[:, 1] * n_cells[2] + cell_idx[:, 2]
    num_cells_total = np.prod(n_cells)
    
    # Create head and next arrays (Linked List approach, but better for NumPy)
    # head[c] is the index of the first atom in cell c
    # next_atom[i] is the index of the next atom in the same cell as i
    head = np.full(num_cells_total, -1, dtype=np.int32)
    next_atom = np.full(N, -1, dtype=np.int32)
    
    for i in range(N):
        c = flat_idx[i]
        next_atom[i] = head[c]
        head[c] = i
        
    # 2. Iterate over cells and neighbors
    # Pre-allocate results (using lists for appending bonds is fine, but we'll pre-allocate matrices)
    dist_matrix = np.zeros((N, N), dtype=np.float32)
    X_dist = np.zeros((N, N), dtype=np.float32)
    Y_dist = np.zeros((N, N), dtype=np.float32)
    Z_dist = np.zeros((N, N), dtype=np.float32)
    
    bond_list = []
    dist_list = []
    
    # 27 neighbors including self by default, but expand if needed
    if Box_dim is not None and len(Box_dim) > 0:  # is_periodic
        (nx_img, ny_img, nz_img), _ = _images_needed(Cell, max_cutoff)
    else:
        nx_img, ny_img, nz_img = 1, 1, 1
        
    rx = np.arange(-nx_img, nx_img + 1)
    ry = np.arange(-ny_img, ny_img + 1)
    rz = np.arange(-nz_img, nz_img + 1)
    offsets = np.array(np.meshgrid(rx, ry, rz)).T.reshape(-1, 3)
    
    # We only need to check half the neighbors to avoid double counting, 
    # but with PBC it's safer to check all or use a specific subset.
    # To keep it simple and accurate, we check all and filter i < j later or at runtime.
    
    # Use a progress iterator for the outer x-dimension loop
    for cx in get_progress_iterator(range(n_cells[0]), desc="Finding dists", unit="CellX"):
        for cy in range(n_cells[1]):
            for cz_idx in range(n_cells[2]):
                c1 = cx * (n_cells[1] * n_cells[2]) + cy * n_cells[2] + cz_idx
                i = head[c1]
                if i == -1: continue
                
                # Get all atoms in this cell
                atoms1 = []
                while i != -1:
                    atoms1.append(i)
                    i = next_atom[i]
                atoms1 = np.array(atoms1, dtype=np.int32)
                
                for off in offsets:
                    nx_unwrapped = (cx + off[0])
                    ny_unwrapped = (cy + off[1])
                    nz_unwrapped = (cz_idx + off[2])
                    
                    nx_wrapped = nx_unwrapped % n_cells[0]
                    ny_wrapped = ny_unwrapped % n_cells[1]
                    nz_wrapped = nz_unwrapped % n_cells[2]
                    
                    shift_x = np.floor(nx_unwrapped / n_cells[0])
                    shift_y = np.floor(ny_unwrapped / n_cells[1])
                    shift_z = np.floor(nz_unwrapped / n_cells[2])
                    
                    c2 = nx_wrapped * (n_cells[1] * n_cells[2]) + ny_wrapped * n_cells[2] + nz_wrapped
                    j = head[c2]
                    if j == -1: continue
                    
                    atoms2 = []
                    while j != -1:
                        atoms2.append(j)
                        j = next_atom[j]
                    atoms2 = np.array(atoms2, dtype=np.int32)
                    
                    # Compute distances between atoms1 and atoms2
                    # Vectorized chunk
                    p1 = positions[atoms1]
                    p2 = positions[atoms2]
                    
                    # Outer subtraction for N1 x N2 pairs
                    # p1: (N1, 3), p2: (N2, 3) -> diff: (N1, N2, 3)
                    diff = p2[np.newaxis, :, :] - p1[:, np.newaxis, :]
                    
                    shift_frac = np.array([shift_x, shift_y, shift_z], dtype=np.float32)
                    shift_cart = H @ shift_frac
                    diff_cart = diff + shift_cart
                    
                    d2 = np.sum(diff_cart**2, axis=2)
                    d = np.sqrt(d2)
                    
                    # Masking by cutoff
                    # Note: different cutoffs for H vs non-H
                    is_h1 = is_h[atoms1]
                    is_h2 = is_h[atoms2]
                    # matrix of cutoffs (N1 x N2)
                    cutoffs = np.where(is_h1[:, np.newaxis] | is_h2[np.newaxis, :], rmaxH, cutoff)
                    
                    mask = (d > 1e-7) & (d <= cutoffs)
                    
                    # Filter i < j to avoid double counts if needed, but here we fill the whole matrix.
                    # Actually, if c1 == c2, only i < j. If c1 != c2, we'll double count unless we filter.
                    # Easiest way: only fill if i < j.
                    
                    ii, jj = np.where(mask)
                    for k in range(len(ii)):
                        idx1 = atoms1[ii[k]]
                        idx2 = atoms2[jj[k]]
                        if idx1 < idx2:
                            val = d[ii[k], jj[k]]
                            dist_matrix[idx1, idx2] = val
                            dist_matrix[idx2, idx1] = val
                            X_dist[idx1, idx2] = diff_cart[ii[k], jj[k], 0]
                            X_dist[idx2, idx1] = -X_dist[idx1, idx2]
                            Y_dist[idx1, idx2] = diff_cart[ii[k], jj[k], 1]
                            Y_dist[idx2, idx1] = -Y_dist[idx1, idx2]
                            Z_dist[idx1, idx2] = diff_cart[ii[k], jj[k], 2]
                            Z_dist[idx2, idx1] = -Z_dist[idx1, idx2]
                            bond_list.append([idx1, idx2])
                            dist_list.append(val)
                            
    return dist_matrix, X_dist, Y_dist, Z_dist, np.array(bond_list), np.array(dist_list)

# Alias for backward compatibility during migration
cell_list_dist_matrix_fast = cell_list_dist_matrix

def neighbor_list_fast(atoms, Box, cutoff=2.45, rmaxH=None, H_type='H'):
    """
    Higly optimized Cell-list algorithm for finding atom pairs within a cutoff.
    Returns sparse lists instead of NxN matrices to save memory.
    Supports Triclinic boxes.
    
    Args:
        atoms: list of atom dictionaries.
        Box: a 1x3, 1x6 or 1x9 list representing Cell dimensions.
        cutoff: maximum distance for non-hydrogen bonds.
        rmaxH: cutoff distance for hydrogen bonds. If None, uses cutoff.
        H_type: atom type string for hydrogen.
        
    Returns:
        i_idx, j_idx: Nx1 numpy arrays of atom indices.
        dist: Nx1 numpy array of distances.
        dx, dy, dz: Nx1 numpy arrays of displacement components (r_j - r_i).
    """
    N = len(atoms)
    positions = np.array([[atom['x'], atom['y'], atom['z']] for atom in atoms], dtype=np.float32)
    types = np.array([atom.get('type', '') for atom in atoms])
    is_h = np.array([bool(t and t[0].upper() == 'H') for t in types])
    
    if rmaxH is None:
        rmaxH = cutoff
        
    if Box is None:
        # Non-periodic case: use a bounding box
        min_coords = np.min(positions, axis=0)
        max_coords = np.max(positions, axis=0)
        Box_dim = max_coords - min_coords + 2 * max(cutoff, rmaxH)
        H = np.diag(Box_dim).astype(np.float32)
        Hinv = np.linalg.inv(H)
        # Shift positions to be positive relative to min_coords
        positions_shifted = positions - min_coords + max(cutoff, rmaxH)
        frac_coords = (Hinv @ positions_shifted.T).T
        is_periodic = False
    else:
        Box_dim, Cell = normalize_box(Box)
        assert Box_dim is not None and Cell is not None  # Box is not None in this branch
        # Construct triclinic Box matrix H and its inverse Hinv
        a, b, c = Cell[0], Cell[1], Cell[2]
        if len(Cell) == 3:
            alpha, beta, gamma = 90.0, 90.0, 90.0
        else:
            alpha, beta, gamma = Cell[3], Cell[4], Cell[5]
        ar, br, gr = np.radians([alpha, beta, gamma])
        ax = a
        bx = b * np.cos(gr)
        by = b * np.sin(gr)
        cx = c * np.cos(br)
        cy = c * (np.cos(ar) - np.cos(br) * np.cos(gr)) / np.sin(gr)
        cz = np.sqrt(max(0, c**2 - cx**2 - cy**2))
        H = np.array([[ax, bx, cx], [0, by, cy], [0, 0, cz]], dtype=np.float32)
        Hinv = np.linalg.inv(H)
        frac_coords = (Hinv @ positions.T).T
        frac_coords = frac_coords % 1.0
        positions = (H @ frac_coords.T).T  # Wrap positions to principal box
        is_periodic = True

    max_cutoff = max(cutoff, rmaxH)
    # Effective box widths in each direction
    if is_periodic:
        n_cells = np.maximum(np.floor(np.array([H[0,0], H[1,1], H[2,2]]) / max_cutoff), 1).astype(int)
    else:
        n_cells = np.maximum(np.floor(Box_dim / max_cutoff), 1).astype(int)
        
    cell_idx = np.floor(frac_coords * n_cells).astype(int)
    cell_idx = np.clip(cell_idx, 0, n_cells - 1)
    flat_idx = cell_idx[:, 0] * (n_cells[1] * n_cells[2]) + cell_idx[:, 1] * n_cells[2] + cell_idx[:, 2]
    num_cells_total = np.prod(n_cells)
    
    head = np.full(num_cells_total, -1, dtype=np.int32)
    next_atom = np.full(N, -1, dtype=np.int32)
    for i in range(N):
        c = flat_idx[i]
        next_atom[i] = head[c]
        head[c] = i
        
    i_idx_list = []
    j_idx_list = []
    dist_list = []
    dx_list = []
    dy_list = []
    dz_list = []
    
    if is_periodic:
        (nx_img, ny_img, nz_img), _ = _images_needed(Box, max_cutoff)
    else:
        nx_img, ny_img, nz_img = 1, 1, 1
        
    rx = np.arange(-nx_img, nx_img + 1)
    ry = np.arange(-ny_img, ny_img + 1)
    rz = np.arange(-nz_img, nz_img + 1)
    offsets = np.array(np.meshgrid(rx, ry, rz)).T.reshape(-1, 3)
    
    # Use a progress iterator for the outer x-dimension loop
    for cx in get_progress_iterator(range(n_cells[0]), desc="Finding sparse dists", unit="CellX"):
        for cy in range(n_cells[1]):
            for cz_idx in range(n_cells[2]):
                c1 = cx * (n_cells[1] * n_cells[2]) + cy * n_cells[2] + cz_idx
                i_head = head[c1]
                if i_head == -1: continue
                
                atoms1 = []
                curr = i_head
                while curr != -1:
                    atoms1.append(curr)
                    curr = next_atom[curr]
                atoms1 = np.array(atoms1, dtype=np.int32)
                
                # We iterate over offsets directly because multiple offsets might wrap 
                # to the same cell but represent DIFFERENT periodic images.
                for off in offsets:
                    nx_unwrapped = (cx + off[0])
                    ny_unwrapped = (cy + off[1])
                    nz_unwrapped = (cz_idx + off[2])
                    
                    if is_periodic:
                        nx_wrapped = nx_unwrapped % n_cells[0]
                        ny_wrapped = ny_unwrapped % n_cells[1]
                        nz_wrapped = nz_unwrapped % n_cells[2]
                        
                        shift_x = np.floor(nx_unwrapped / n_cells[0])
                        shift_y = np.floor(ny_unwrapped / n_cells[1])
                        shift_z = np.floor(nz_unwrapped / n_cells[2])
                    else:
                        if nx_unwrapped < 0 or nx_unwrapped >= n_cells[0] or ny_unwrapped < 0 or ny_unwrapped >= n_cells[1] or nz_unwrapped < 0 or nz_unwrapped >= n_cells[2]:
                            continue
                        nx_wrapped, ny_wrapped, nz_wrapped = nx_unwrapped, ny_unwrapped, nz_unwrapped
                        shift_x = shift_y = shift_z = 0.0
                    
                    c2 = nx_wrapped * (n_cells[1] * n_cells[2]) + ny_wrapped * n_cells[2] + nz_wrapped
                    j_head = head[c2]
                    if j_head == -1: continue
                    
                    atoms2 = []
                    curr = j_head
                    while curr != -1:
                        atoms2.append(curr)
                        curr = next_atom[curr]
                    atoms2 = np.array(atoms2, dtype=np.int32)
                    
                    p1 = positions[atoms1]
                    p2 = positions[atoms2]
                    
                    diff = p2[np.newaxis, :, :] - p1[:, np.newaxis, :]
                    
                    if is_periodic:
                        shift_frac = np.array([shift_x, shift_y, shift_z], dtype=np.float32)
                        shift_cart = H @ shift_frac
                        diff_cart = diff + shift_cart
                    else:
                        diff_cart = diff

                    d2 = np.sum(diff_cart**2, axis=2)
                    d = np.sqrt(d2)
                    
                    is_h1 = is_h[atoms1]
                    is_h2 = is_h[atoms2]
                    cutoffs = np.where(is_h1[:, np.newaxis] | is_h2[np.newaxis, :], rmaxH, cutoff)
                    
                    mask = (d > 1e-7) & (d <= cutoffs)
                    
                    ii_local, jj_local = np.where(mask)
                    for k in range(len(ii_local)):
                        idx1 = atoms1[ii_local[k]]
                        idx2 = atoms2[jj_local[k]]
                        if idx1 < idx2:
                            val = d[ii_local[k], jj_local[k]]
                            i_idx_list.append(idx1)
                            j_idx_list.append(idx2)
                            dist_list.append(val)
                            dx_list.append(diff_cart[ii_local[k], jj_local[k], 0])
                            dy_list.append(diff_cart[ii_local[k], jj_local[k], 1])
                            dz_list.append(diff_cart[ii_local[k], jj_local[k], 2])
                            
    return (np.array(i_idx_list, dtype=np.int32), 
            np.array(j_idx_list, dtype=np.int32), 
            np.array(dist_list, dtype=np.float32),
            np.array(dx_list, dtype=np.float32),
            np.array(dy_list, dtype=np.float32),
            np.array(dz_list, dtype=np.float32))

# Carry over helper functions from original file for full compatibility
def convert_to_sparse_dict(dist_matrix, X_dist, Y_dist, Z_dist, cutoff):
    """Convert full distance matrices to a sparse dictionary format."""
    distance_dict = {}
    i_indices, j_indices = np.where((dist_matrix > 0) & (dist_matrix <= cutoff) & (np.triu(np.ones(dist_matrix.shape), k=1) > 0))
    for idx in range(len(i_indices)):
        i, j = i_indices[idx], j_indices[idx]
        distance_dict[(i, j)] = (dist_matrix[i, j], X_dist[i, j], Y_dist[i, j], Z_dist[i, j])
    return distance_dict

def get_neighbors(dist_matrix, X_dist, Y_dist, Z_dist, atom_index, r_max=None):
    """Get all neighbors of a specific atom from the distance matrices."""
    row = dist_matrix[atom_index]
    if r_max is None:
        mask = (row > 0) & (np.arange(len(row)) != atom_index)
    else:
        mask = (row > 0) & (row <= r_max) & (np.arange(len(row)) != atom_index)
    j_indices = np.where(mask)[0]
    neighbors = []
    for j in j_indices:
        neighbors.append((j, dist_matrix[atom_index, j], X_dist[atom_index, j], Y_dist[atom_index, j], Z_dist[atom_index, j]))
    if len(neighbors) > 1:
        # Sort by distance
        neighbors.sort(key=lambda x: x[1])
    return neighbors
