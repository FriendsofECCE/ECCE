"""
Utility functions for converting between different Cell representations.

This module provides functions to convert between Box_dim (a 1D array of
the simulation Box dimensions) and Cell (a 3×3 matrix representation of
the unit Cell).
"""

import numpy as np


def normalize_box(Box):
    """
    Normalize a Box parameter to both Box_dim and Cell representations.

    Accepts Box in 1x3 (orthogonal), 1x6 (Cell parameters), or 1x9
    (GROMACS triclinic) formats and returns a tuple (Box_dim, Cell).
    """
    if Box is None:
        return None, None

    Box = np.array(Box, dtype=float)

    if len(Box) == 3:
        Box_dim = Box
        Cell = np.array([Box[0], Box[1], Box[2], 90.0, 90.0, 90.0])
    elif len(Box) == 6:
        Cell = Box
        Box_dim = Cell2Box_dim(Cell)
    elif len(Box) == 9:
        Box_dim = Box
        Cell = Box_dim2Cell(Box_dim)
    else:
        raise ValueError(f"Invalid Box length: {len(Box)}. Expected 3, 6, or 9.")

    return Box_dim, Cell


def Box_dim2Cell(Box_dim):
    """
    Convert Box_dim to Box matrix and Cell parameters.
    
    Box_dim is a 1D array or list of Box dimensions, typically in Angstroms.
    For an orthogonal Box, Box_dim is [Lx, Ly, Lz].
    For a triclinic Box, Box_dim is [Lx, Ly, Lz, xy, xz, yz] or
    [Lx, Ly, Lz, alpha, beta, gamma] (angles in degrees).
    
    Args:
        Box_dim: A list or numpy array with Box dimensions.
            Length 3: [Lx, Ly, Lz] - orthogonal Box
            Length 6: [Lx, Ly, Lz, xy, xz, yz] - triclinic Box with Box vectors
            Length 6: [Lx, Ly, Lz, alpha, beta, gamma] - triclinic Box with angles (degrees)
            Length 9: [xx, xy, xz, yx, yy, yz, zx, zy, zz] - full 3×3 matrix in row-major order
    
    Returns:
        Box: A 3×3 numpy array representing the Box matrix
        Cell: A 1×6 numpy array with [a, b, c, alfa, beta, gamma]
    """
    if Box_dim is None:
        return np.array([50.0, 50.0, 50.0, 90.0, 90.0, 90.0])
        
    if isinstance(Box_dim, (int, float)) or (isinstance(Box_dim, np.ndarray) and Box_dim.ndim == 0):
        print(f"Warning: Box_dim2Cell received unsized value '{Box_dim}'. Using default orthogonal box.")
        return np.array([50.0, 50.0, 50.0, 90.0, 90.0, 90.0])
        
    Box_dim = np.array(Box_dim, dtype=float)
    if Box_dim.ndim == 0 or len(Box_dim.shape) == 0 or len(Box_dim) == 0:
        print("Warning: Box_dim2Cell received empty/unsized shape. Using default orthogonal box.")
        return np.array([50.0, 50.0, 50.0, 90.0, 90.0, 90.0])
    
    if len(Box_dim) == 3:
        # Orthogonal Box: [Lx, Ly, Lz]
        lx, ly, lz = Box_dim
        Box = np.array([
            [lx, 0.0, 0.0],
            [0.0, ly, 0.0],
            [0.0, 0.0, lz]
        ])
        # Create Cell parameters for orthogonal Box
        Cell = np.array([lx, ly, lz, 90.0, 90.0, 90.0])
    elif len(Box_dim) == 6:
        # Check if the values are angles or Box vectors
        if all(angle > 0 and angle < 180 for angle in Box_dim[3:6]):
            # Triclinic Box with angles: [Lx, Ly, Lz, alpha, beta, gamma]
            lx, ly, lz, alpha, beta, gamma = Box_dim
            
            # Store Cell parameters directly
            Cell = np.array([lx, ly, lz, alpha, beta, gamma])
            
            # Convert angles from degrees to radians
            alpha_rad = np.radians(alpha)
            beta_rad = np.radians(beta)
            gamma_rad = np.radians(gamma)
            
            # Calculate Box vectors
            cos_alpha = np.cos(alpha_rad)
            cos_beta = np.cos(beta_rad)
            cos_gamma = np.cos(gamma_rad)
            sin_gamma = np.sin(gamma_rad)
            
            Box = np.zeros((3, 3))
            Box[0, 0] = lx
            Box[1, 0] = 0.0
            Box[2, 0] = 0.0
            
            Box[0, 1] = ly * cos_gamma
            Box[1, 1] = ly * sin_gamma
            Box[2, 1] = 0.0
            
            Box[0, 2] = lz * cos_beta
            Box[1, 2] = lz * (cos_alpha - cos_beta * cos_gamma) / sin_gamma
            Box[2, 2] = lz * np.sqrt(1.0 - cos_alpha**2 - cos_beta**2 - cos_gamma**2 + 
                                     2.0 * cos_alpha * cos_beta * cos_gamma) / sin_gamma
        else:
            # Triclinic Box given as GROMACS tilt vectors: [Lx, Ly, Lz, xy, xz, yz].
            # Return proper cell parameters [a, b, c, alpha, beta, gamma] — NOT a 3x3
            # matrix (the previous code assigned a matrix to `Cell`, breaking every caller
            # that unpacks six values). Same formula as the 9-component branch below.
            lx, ly, lz, xy, xz, yz = Box_dim
            a = lx
            b = np.sqrt(ly**2 + xy**2)
            c = np.sqrt(lz**2 + xz**2 + yz**2)
            cos_alfa = (ly*yz + xy*xz) / (b*c) if b > 0 and c > 0 else 0.0
            cos_beta = xz / c if c > 0 else 0.0
            cos_gamma = xy / b if b > 0 else 0.0
            alfa = np.degrees(np.arccos(np.clip(cos_alfa, -1.0, 1.0)))
            beta = np.degrees(np.arccos(np.clip(cos_beta, -1.0, 1.0)))
            gamma = np.degrees(np.arccos(np.clip(cos_gamma, -1.0, 1.0)))
            Cell = np.array([a, b, c, alfa, beta, gamma])
    elif len(Box_dim) == 9:
        # GRO 9-component format:
        # Box_dim = [lx, ly, lz, 0, 0, xy, 0, xz, yz]
        lx = Box_dim[0]
        ly = Box_dim[1]
        lz = Box_dim[2]
        xy = Box_dim[5]  # different index than documented - based on your MATLAB code
        xz = Box_dim[7]
        yz = Box_dim[8]
        
        # Construct the unit Cell vectors to match your MATLAB implementation
        Box = np.zeros((3, 3))
        Box[0, 0] = lx            # xx
        Box[0, 1] = 0.0           # xy
        Box[0, 2] = 0.0           # xz
        Box[1, 0] = xy            # yx
        Box[1, 1] = ly            # yy
        Box[1, 2] = 0.0           # yz
        Box[2, 0] = xz            # zx
        Box[2, 1] = yz            # zy
        Box[2, 2] = lz            # zz
        
        # Calculate Cell parameters from Box dimensions using MATLAB formulas
        a = lx
        b = np.sqrt(ly**2 + xy**2)
        c = np.sqrt(lz**2 + xz**2 + yz**2)
        
        # Calculate angles using formulas from MATLAB implementation
        cos_alfa = (ly*yz + xy*xz)/(b*c)
        cos_beta = xz/c
        cos_gamma = xy/b
        
        # Convert to degrees
        alfa = np.degrees(np.arccos(np.clip(cos_alfa, -1.0, 1.0)))
        beta = np.degrees(np.arccos(np.clip(cos_beta, -1.0, 1.0)))
        gamma = np.degrees(np.arccos(np.clip(cos_gamma, -1.0, 1.0)))
        
        Cell = np.array([a, b, c, alfa, beta, gamma])
    else:
        raise ValueError(f"Invalid Box_dim length: {len(Box_dim)}. Expected 3, 6, or a 9.")
        
    return Cell


def Cell2Box_dim(Cell, original_Box_dim=None):
    """
    Convert Cell parameters [a, b, c, alfa, beta, gamma] to Box_dim.
    
    Args:
        Cell: A 1×6 numpy array with Cell parameters [a, b, c, alfa, beta, gamma]
                    where a, b, c are lengths and alfa, beta, gamma are angles in degrees.
        original_Box_dim: Optional, the original Box_dim from which the Cell parameters were derived.
                        If provided, it will be used to ensure consistency in triclinic parameters.
    
    Returns:
        Box_dim: A numpy array with Box dimensions
    """
    Cell = np.array(Cell, dtype=float)
    
    if len(Cell) == 3:
        Cell = list(Cell) + [90.0, 90.0, 90.0]
    
    if len(Cell) != 6:
        raise ValueError(f"Expected 6 Cell parameters, got {len(Cell)}")
    
    a, b, c, alfa, beta, gamma = Cell
    
    # Check if this is an orthogonal Box (all angles ~90 degrees)
    if (np.isclose(alfa, 90.0) and np.isclose(beta, 90.0) and np.isclose(gamma, 90.0)):
        # Simple orthogonal Box - return only [lx, ly, lz] as a 1x3 array
        return np.array([a, b, c], dtype=float)
    
    # Convert angles from degrees to radians for non-orthogonal calculations
    alfa_rad = np.radians(alfa)
    beta_rad = np.radians(beta)
    gamma_rad = np.radians(gamma)
    
    # Calculate Box vectors according to MATLAB implementation
    lx = a
    xy = b * np.cos(gamma_rad)  # Use direct cosine, not offset by π/2
    ly = np.sqrt(b**2 - xy**2)
    xz = c * np.cos(beta_rad)   # Use direct cosine, not offset by π/2
    
    # Calculate yz using the formula from MATLAB implementation
    yz = (b * c * np.cos(alfa_rad) - xy * xz) / ly
    
    # Calculate lz
    lz = np.sqrt(c**2 - xz**2 - yz**2)
    
    # For non-orthogonal Box with original dimensions provided
    if original_Box_dim is not None and len(original_Box_dim) == 9:
        # If original Box dimensions are provided, maintain the triclinic parameters
        # but update the Box lengths to match Cell parameters a, b, c
        return np.array([lx, ly, lz, 0.0, 0.0, original_Box_dim[5], 0.0, original_Box_dim[7], original_Box_dim[8]])

    # Return in GRO format: [lx, ly, lz, 0, 0, xy, 0, xz, yz]
    return np.array([lx, ly, lz, 0.0, 0.0, xy, 0.0, xz, yz])
