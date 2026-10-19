#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Transform module for coordinate transformations in atomipy.

This module consolidates functionality from the previous fract.py, triclinic.py, and ortho.py
modules into a unified interface for handling all coordinate transformations in molecular systems.

Key functionality includes:

1. Cartesian-fractional coordinate conversions:
   - cartesian_to_fractional(): Convert Cartesian coordinates to fractional coordinates
   - fractional_to_cartesian(): Convert fractional coordinates to Cartesian coordinates
   - direct_cartesian_to_fractional(): Direct conversion using crystallographic matrices
   - direct_fractional_to_cartesian(): Direct conversion using crystallographic matrices

2. Triclinic-orthogonal transformations:
   - triclinic_to_orthogonal(): Convert triclinic coordinates to orthogonal system
   - orthogonal_to_triclinic(): Convert orthogonal coordinates to triclinic system

3. Utility functions:
   - wrap(): Simplified interface to wrap atoms into simulation cell (auto-detects box format)
   - wrap_coordinates(): Low-level coordinate wrapping with flexible options
   - get_orthogonal_box(): Get orthogonal Box dimensions from triclinic parameters
   - get_cell_vectors(): Calculate Cell vectors from Box parameters

All functions support both direct atom dictionary input and numpy array input,
making them flexible for various use cases.
"""

import numpy as np
from .cell_utils import Box_dim2Cell, Cell2Box_dim, normalize_box


def cartesian_to_fractional(atoms=None, cart_coords=None, Box=None, add_to_atoms=True):
    """
    Convert cartesian coordinates to fractional coordinates.
    
    Parameters
    ----------
    atoms : list of dict, optional
        List of atom dictionaries with 'x', 'y', 'z' cartesian coordinates.
        If provided, these will be used for the conversion. Either atoms or cart_coords must be provided.
    cart_coords : numpy.ndarray, optional
        Nx3 array of cartesian coordinates, where N is the number of atoms.
        Either atoms or cart_coords must be provided.
    Box : list or array
        Simulation cell dimensions (1x3, 1x6, or 1x9).
    add_to_atoms : bool, optional
        If True and atoms is provided, adds fractional coordinates to the atom dictionaries
        as 'xfrac', 'yfrac', 'zfrac'. Default is True.
        
    Returns
    -------
        frac_coords : numpy.ndarray
        Nx3 array of fractional coordinates, where N is the number of atoms.
    atoms : list of dict, optional
        The original atoms list with updated fractional coordinate fields
        if add_to_atoms is True.
    """
    if atoms is None and cart_coords is None:
        raise ValueError("Either atoms or cart_coords must be provided")
    if Box is None:
        raise ValueError("Box must be provided")
    
    # Get cartesian coordinates from atoms if needed
    if cart_coords is None:
        cart_coords = np.array([[atom['x'], atom['y'], atom['z']] for atom in atoms])
    
    _, Cell = normalize_box(Box)
    
    # Get Cell vectors
    cell_vectors = get_cell_vectors(Cell)
    
    # Compute the transformation matrix
    # This is the inverse of the matrix used in fractional_to_cartesian
    det = np.dot(cell_vectors[0], np.cross(cell_vectors[1], cell_vectors[2]))
    inv_mat = np.zeros((3, 3))
    inv_mat[0] = np.cross(cell_vectors[1], cell_vectors[2]) / det
    inv_mat[1] = np.cross(cell_vectors[2], cell_vectors[0]) / det
    inv_mat[2] = np.cross(cell_vectors[0], cell_vectors[1]) / det
    
    # Apply transformation to each point
    frac_coords = np.zeros_like(cart_coords)
    for i, cart in enumerate(cart_coords):
        frac_coords[i] = np.dot(inv_mat, cart)
    
    # Add fractional coordinates to atoms if requested
    if atoms is not None and add_to_atoms:
        for i, atom in enumerate(atoms):
            atom['xfrac'] = float(round(frac_coords[i, 0], 6))
            atom['yfrac'] = float(round(frac_coords[i, 1], 6))
            atom['zfrac'] = float(round(frac_coords[i, 2], 6))
        return frac_coords, atoms
    
    return frac_coords


def fractional_to_cartesian(atoms=None, frac_coords=None, Box=None, add_to_atoms=True):
    """
    Convert fractional coordinates to cartesian coordinates.
    
    Parameters
    ----------
    atoms : list of dict, optional
        List of atom dictionaries with 'xfrac', 'yfrac', 'zfrac' fractional coordinates.
        If provided, these will be used for the conversion. Either atoms or frac_coords must be provided.
    frac_coords : numpy.ndarray, optional
        Nx3 array of fractional coordinates, where N is the number of atoms.
        Either atoms or frac_coords must be provided.
    Box : list or array
        Simulation cell dimensions (1x3, 1x6, or 1x9).
    add_to_atoms : bool, optional
        If True and atoms is provided, adds cartesian coordinates to the atom dictionaries
        as 'x', 'y', 'z'. Default is True.
        
    Returns
    -------
    cart_coords : numpy.ndarray
        Nx3 array of cartesian coordinates, where N is the number of atoms.
    atoms : list of dict, optional
        The original atoms list with updated cartesian coordinate fields
        if add_to_atoms is True.
    """
    if atoms is None and frac_coords is None:
        raise ValueError("Either atoms or frac_coords must be provided")
    if Box is None:
        raise ValueError("Box must be provided")
    
    _, Cell = normalize_box(Box)
    
    # Get fractional coordinates from atoms if needed
    if frac_coords is None:
        frac_coords = np.array([[atom.get('xfrac', 0.0), 
                                atom.get('yfrac', 0.0), 
                                atom.get('zfrac', 0.0)] 
                                for atom in atoms])
    
    # Get Cell vectors
    cell_vectors = get_cell_vectors(Cell)
    
    # Apply transformation to each point
    cart_coords = np.zeros_like(frac_coords)
    for i, frac in enumerate(frac_coords):
        cart_coords[i] = (cell_vectors[0] * frac[0] +
                        cell_vectors[1] * frac[1] +
                        cell_vectors[2] * frac[2])
    
    # Add cartesian coordinates to atoms if requested
    if atoms is not None and add_to_atoms:
        for i, atom in enumerate(atoms):
            atom['x'] = float(round(cart_coords[i, 0], 6))
            atom['y'] = float(round(cart_coords[i, 1], 6))
            atom['z'] = float(round(cart_coords[i, 2], 6))
        return cart_coords, atoms
    
    return cart_coords


def wrap_coordinates(atoms=None, coords=None, frac_coords=None, Box=None,
         add_to_atoms=True, return_type='fractional'):
    """
    Wrap coordinates to ensure they are within the primary unit Cell (0 to 1 in fractional coordinates).
    
    Parameters
    ----------
    atoms : list of dict, optional
        List of atom dictionaries with coordinate information.
        If provided along with add_to_atoms=True, the wrapped coordinates will be added to atoms.
    coords : numpy.ndarray, optional
        Nx3 array of cartesian coordinates to wrap.
    frac_coords : numpy.ndarray, optional
        Nx3 array of fractional coordinates to wrap. If provided, coords is ignored.
    Box : list or array, optional
        Simulation cell dimensions (1x3, 1x6, or 1x9). Required when conversions
        between cartesian and fractional coordinates are needed.
    add_to_atoms : bool, optional
        If True and atoms is provided, updates the atom dictionaries with wrapped coordinates.
    return_type : str, optional
        Specifies the type of coordinates to return: 'fractional' (default) or 'cartesian'.
        
    Returns
    -------
    wrapped_coords : numpy.ndarray
        Nx3 array of wrapped coordinates in the specified return_type.
    atoms : list of dict, optional
        The original atoms list with updated coordinate fields if add_to_atoms is True.
    """
    if atoms is None and coords is None and frac_coords is None:
        raise ValueError("Either atoms, coords, or frac_coords must be provided")
    
    return_cartesian = return_type.lower() == 'cartesian'
    needs_box = (
        coords is not None
        or frac_coords is None
        or return_cartesian
        or (atoms is not None and frac_coords is None and not all('xfrac' in atom for atom in atoms))
    )

    if needs_box and Box is None:
        raise ValueError("Box must be provided when converting coordinates")
    if needs_box:
        normalize_box(Box)
    
    # Step 1: Get fractional coordinates
    if frac_coords is None:
        if coords is not None:
            # Convert cartesian to fractional
            frac_coords = cartesian_to_fractional(cart_coords=coords, Box=Box)
        elif atoms is not None:
            # Check if atoms already have fractional coordinates
            if all('xfrac' in atom for atom in atoms):
                frac_coords = np.array([[atom['xfrac'], atom['yfrac'], atom['zfrac']] for atom in atoms])
            else:
                # Extract cartesian coordinates and convert to fractional
                cart_coords = np.array([[atom['x'], atom['y'], atom['z']] for atom in atoms])
                frac_coords = cartesian_to_fractional(cart_coords=cart_coords, Box=Box)
    
    # Step 2: Perform the wrapping operation on fractional coordinates
    wrapped_frac = frac_coords.copy()
    wrapped_frac = wrapped_frac % 1.0  # Simple modulo operation for primary Cell wrapping
    
    # Step 3: Update atoms if requested
    if atoms is not None and add_to_atoms:
        for i, atom in enumerate(atoms):
            atom['xfrac'] = float(round(wrapped_frac[i, 0], 6))
            atom['yfrac'] = float(round(wrapped_frac[i, 1], 6))
            atom['zfrac'] = float(round(wrapped_frac[i, 2], 6))
    
    # Step 4: Return coordinates in the requested format
    if return_type.lower() == 'cartesian':
        # Convert wrapped fractional coordinates back to cartesian
        wrapped_cart = fractional_to_cartesian(frac_coords=wrapped_frac, Box=Box)
        if atoms is not None and add_to_atoms:
            for i, atom in enumerate(atoms):
                atom['x'] = float(round(wrapped_cart[i, 0], 6))
                atom['y'] = float(round(wrapped_cart[i, 1], 6))
                atom['z'] = float(round(wrapped_cart[i, 2], 6))
            return wrapped_cart, atoms
        return wrapped_cart
    else:  # fractional
        if atoms is not None and add_to_atoms:
            return wrapped_frac, atoms
        return wrapped_frac


def wrap(atoms, Box, return_type='cartesian'):
    """
    Wrap atom coordinates to ensure they are within the primary simulation cell.
    
    This function handles both orthogonal and triclinic simulation boxes with automatic 
    format detection. It uses the helper functions Box_dim2Cell and Cell2Box_dim to ensure
    consistent coordinate transformations across the atomipy package.
    
    Parameters
    ----------
    atoms : list of dict
        List of atom dictionaries with coordinate information ('x', 'y', 'z').
    Box : list or array
        Simulation cell dimensions in one of the following formats:
        - Box_dim format (1x3): [Lx, Ly, Lz] for orthogonal boxes
        - Box_dim format (1x9): [lx, ly, lz, 0, 0, xy, 0, xz, yz] for triclinic boxes
        - Cell format (1x6): [a, b, c, alpha, beta, gamma] for cell parameters
    return_type : str, optional
        Specifies the type of coordinates to return: 'cartesian' (default) or 'fractional'.
        
    Returns
    -------
    atoms : list of dict
        The original atoms list with updated coordinate fields.
    
    Notes
    -----
    This function automatically detects the box format:
    - If box has length 3, it's treated as orthogonal Box_dim format
    - If box has length 9, it's treated as triclinic Box_dim format (GROMACS convention)
    - If box has length 6, it checks if values are angles (Cell format) or box vectors
    
    The function wraps coordinates using modulo 1.0 on fractional coordinates, which correctly
    handles both positive and negative coordinates for periodic boundary conditions.
    
    The transformation uses the cell matrix with lattice vectors as columns, following
    crystallographic convention where:
    - cell_matrix[:, 0] = a-vector
    - cell_matrix[:, 1] = b-vector  
    - cell_matrix[:, 2] = c-vector
    
    Examples
    --------
    # Orthogonal box
    atoms = wrap(atoms, [10.0, 10.0, 10.0])
     
    # Cell parameters
    atoms = wrap(atoms, [10.0, 10.0, 10.0, 90.0, 90.0, 120.0])
     
    # GROMACS triclinic format
    atoms = wrap(atoms, [10.0, 10.0, 10.0, 0, 0, 2.0, 0, 0.5, 1.5])
    """
    _, Cell = normalize_box(Box)
    
    # Step 1: Extract cartesian coordinates from atoms
    cart_coords = np.array([[atom['x'], atom['y'], atom['z']] for atom in atoms])
    
    # Step 2: Convert box to Cell parameters using helper function
    # This automatically handles all box format detection
    # Step 3: Build cell matrix from Cell parameters
    # The cell matrix has lattice vectors as COLUMNS (crystallographic convention)
    a, b, c, alpha, beta, gamma = Cell
    
    # Convert angles to radians
    alpha_rad = np.radians(alpha)
    beta_rad = np.radians(beta)
    gamma_rad = np.radians(gamma)
    
    # Calculate trigonometric values
    cos_alpha = np.cos(alpha_rad)
    cos_beta = np.cos(beta_rad)
    cos_gamma = np.cos(gamma_rad)
    sin_gamma = np.sin(gamma_rad)
    
    # Calculate volume term
    v = np.sqrt(1 - cos_alpha**2 - cos_beta**2 - cos_gamma**2 + 
                2 * cos_alpha * cos_beta * cos_gamma)
    
    # Build cell matrix with vectors as COLUMNS
    # This follows the crystallographic convention
    cell_matrix = np.zeros((3, 3))
    
    # First column: a-vector along x-axis
    cell_matrix[0, 0] = a
    cell_matrix[1, 0] = 0.0
    cell_matrix[2, 0] = 0.0
    
    # Second column: b-vector in xy-plane
    cell_matrix[0, 1] = b * cos_gamma
    cell_matrix[1, 1] = b * sin_gamma
    cell_matrix[2, 1] = 0.0
    
    # Third column: c-vector
    cell_matrix[0, 2] = c * cos_beta
    cell_matrix[1, 2] = c * (cos_alpha - cos_beta * cos_gamma) / sin_gamma
    cell_matrix[2, 2] = c * v / sin_gamma
    
    # Step 4: Convert Cartesian to fractional coordinates
    # For column vectors: frac = inv(M) @ cart, where @ is matrix multiply
    # For row vectors: frac = cart @ inv(M).T
    inv_cell_matrix = np.linalg.inv(cell_matrix)
    frac_coords = cart_coords @ inv_cell_matrix.T
    
    # Step 5: Wrap fractional coordinates using modulo
    wrapped_frac = frac_coords % 1.0
    
    # Step 6: Convert back to Cartesian if requested
    if return_type.lower() == 'cartesian':
        # For column vectors: cart = M @ frac
        # For row vectors: cart = frac @ M.T
        wrapped_coords = wrapped_frac @ cell_matrix.T
        
        # Update atoms with wrapped Cartesian coordinates
        for i, atom in enumerate(atoms):
            atom['x'] = float(round(wrapped_coords[i, 0], 6))
            atom['y'] = float(round(wrapped_coords[i, 1], 6))
            atom['z'] = float(round(wrapped_coords[i, 2], 6))
            atom['xfrac'] = float(round(wrapped_frac[i, 0], 6))
            atom['yfrac'] = float(round(wrapped_frac[i, 1], 6))
            atom['zfrac'] = float(round(wrapped_frac[i, 2], 6))
        
        return atoms
    
    else:  # fractional
        # Update atoms with wrapped fractional coordinates
        for i, atom in enumerate(atoms):
            atom['xfrac'] = float(round(wrapped_frac[i, 0], 6))
            atom['yfrac'] = float(round(wrapped_frac[i, 1], 6))
            atom['zfrac'] = float(round(wrapped_frac[i, 2], 6))
        
        return atoms


def triclinic_to_orthogonal(atoms=None, coords=None, Box=None, add_to_atoms=True):
    """
    Convert coordinates from triclinic to orthogonal representation.
{{ ... }}
    
    Parameters
    ----------
    atoms : list of dict, optional
        List of atom dictionaries with position information.
        If provided along with add_to_atoms=True, orthogonal coordinates
        will be added to atoms as 'x_ortho', 'y_ortho', 'z_ortho'.
    coords : numpy.ndarray, optional
        Nx3 array of triclinic coordinates.
    Box : list or array
        Simulation cell dimensions (1x3, 1x6, or 1x9).
    add_to_atoms : bool, optional
        If True and atoms is provided, adds orthogonal coordinates to the atom dictionaries.
        
    Returns
    -------
    ortho_coords : numpy.ndarray
        Nx3 array of orthogonal coordinates.
    atoms : list of dict, optional
        The original atoms list with added orthogonal coordinate fields
        if add_to_atoms is True.
    ortho_box : array
        The orthogonal Box dimensions [a', b', c'].
    """
    if atoms is None and coords is None:
        raise ValueError("Either atoms or coords must be provided")
    if Box is None:
        raise ValueError("Box must be provided")
    
    # Get the coordinate array from atoms if needed
    if coords is None and atoms is not None:
        coords = np.array([[atom['x'], atom['y'], atom['z']] for atom in atoms])
    
    _, Cell = normalize_box(Box)
    
    # Extract Box parameters
    a, b, c, alpha, beta, gamma = Cell
    
    # Convert to radians
    alpha_rad = np.radians(alpha)
    beta_rad = np.radians(beta)
    gamma_rad = np.radians(gamma)
    
    # Calculate orthogonal Box dimensions
    a_ortho = a
    b_ortho = b * np.sin(gamma_rad)
    c_ortho = c * np.sin(beta_rad)
    
    # Build transformation matrix
    transform_matrix = np.array([
        [1, np.cos(gamma_rad), np.cos(beta_rad)],
        [0, np.sin(gamma_rad), (np.cos(alpha_rad) - np.cos(beta_rad) * np.cos(gamma_rad)) / np.sin(gamma_rad)],
        [0, 0, np.sqrt(1 - np.cos(beta_rad)**2 - ((np.cos(alpha_rad) - np.cos(beta_rad) * np.cos(gamma_rad)) / np.sin(gamma_rad))**2)]
    ])
    
    # Scale transformation matrix by Box dimensions
    scale_matrix = np.diag([a, b, c])
    transform_matrix = np.dot(transform_matrix, scale_matrix)
    
    # Apply transformation to each coordinate
    ortho_coords = np.zeros_like(coords)
    for i, coord in enumerate(coords):
        ortho_coords[i] = np.dot(transform_matrix, coord)
    
    # Update atoms if requested
    if atoms is not None and add_to_atoms:
        if ortho_coords.shape[0] == len(atoms):
            for i, atom in enumerate(atoms):
                atom['x_ortho'] = float(round(ortho_coords[i, 0], 6))
                atom['y_ortho'] = float(round(ortho_coords[i, 1], 6))
                atom['z_ortho'] = float(round(ortho_coords[i, 2], 6))
            return ortho_coords, atoms, np.array([a_ortho, b_ortho, c_ortho])
    
    return ortho_coords, np.array([a_ortho, b_ortho, c_ortho])


def orthogonal_to_triclinic(ortho_coords, Box, atoms=None, add_to_atoms=True):
    """
    Convert coordinates from orthogonal to triclinic representation.
    
    Parameters
    ----------
    ortho_coords : numpy.ndarray
        Nx3 array of orthogonal coordinates.
    Box : list or array
        Box parameters as [a, b, c, alpha, beta, gamma].
    atoms : list of dict, optional
        List of atom dictionaries, if provided and add_to_atoms is True, triclinic
        coordinates will be added to atoms as 'x', 'y', 'z'.
    add_to_atoms : bool, optional
        If True and atoms is provided, adds triclinic coordinates to the atom dictionaries.
        
    Returns
    -------
    tri_coords : numpy.ndarray
        Nx3 array of triclinic coordinates.
    atoms : list of dict, optional
        The original atoms list with updated triclinic coordinate fields
        if add_to_atoms is True.
    """
    if ortho_coords is None or Box is None:
        raise ValueError("Both ortho_coords and Box must be provided")
    
    # Extract Box parameters
    a, b, c, alpha, beta, gamma = Box
    
    # Convert to radians
    alpha_rad = np.radians(alpha)
    beta_rad = np.radians(beta)
    gamma_rad = np.radians(gamma)
    
    # Build inverse transformation matrix
    transform_matrix = np.array([
        [1, np.cos(gamma_rad), np.cos(beta_rad)],
        [0, np.sin(gamma_rad), (np.cos(alpha_rad) - np.cos(beta_rad) * np.cos(gamma_rad)) / np.sin(gamma_rad)],
        [0, 0, np.sqrt(1 - np.cos(beta_rad)**2 - ((np.cos(alpha_rad) - np.cos(beta_rad) * np.cos(gamma_rad)) / np.sin(gamma_rad))**2)]
    ])
    
    # Scale transformation matrix by Box dimensions
    scale_matrix = np.diag([a, b, c])
    transform_matrix = np.dot(transform_matrix, scale_matrix)
    
    # Invert the transformation matrix
    inv_transform = np.linalg.inv(transform_matrix)
    
    # Apply inverse transformation to each coordinate
    cart_coords = np.zeros_like(ortho_coords)
    for i, coord in enumerate(ortho_coords):
        cart_coords[i] = np.dot(inv_transform, coord)
    
    # Update atoms if requested
    if atoms is not None and add_to_atoms:
      for i, atom in enumerate(atoms):
        atom['x'] = float(round(cart_coords[i, 0], 6))
        atom['y'] = float(round(cart_coords[i, 1], 6))
        atom['z'] = float(round(cart_coords[i, 2], 6))
      return cart_coords, atoms
    
    return cart_coords


def get_orthogonal_box(Box):
    """
    Get the dimensions of the orthogonal Box representing 
    the triclinic Cell.
    
    Parameters
    ----------
    Box : list or array
        Simulation cell dimensions (1x3, 1x6, or 1x9).
        
    Returns
    -------
    ortho_box : numpy.ndarray
        Orthogonal Box dimensions [a', b', c'].
    """
    if Box is None:
        raise ValueError("Box must be provided")
    
    _, Cell = normalize_box(Box)
    
    # Extract Box parameters
    a, b, c, alpha, beta, gamma = Cell
    
    # Convert to radians
    alpha_rad = np.radians(alpha)
    beta_rad = np.radians(beta)
    gamma_rad = np.radians(gamma)
    
    # Calculate orthogonal Box dimensions
    a_ortho = a
    b_ortho = b * np.sin(gamma_rad)
    c_ortho = c * np.sin(beta_rad)
    
    return np.array([a_ortho, b_ortho, c_ortho])


def get_cell_vectors(Box):
    """
    Calculate Cell vectors from Box parameters.
    
    Parameters
    ----------
    Box : list or array
        Box parameters in the format [a, b, c, alpha, beta, gamma].
        
    Returns
    -------
    cell_vectors : numpy.ndarray
        3x3 array of Cell vectors, where each row is a unit Cell vector.
    """
    a, b, c, alpha, beta, gamma = Box
    
    # Convert angles to radians
    alpha_rad = np.radians(alpha)
    beta_rad = np.radians(beta)
    gamma_rad = np.radians(gamma)
    
    # Calculate the Cell vectors
    v1 = np.array([a, 0, 0])
    v2 = np.array([b * np.cos(gamma_rad), b * np.sin(gamma_rad), 0])
    
    # Calculate the third vector components
    cx = c * np.cos(beta_rad)
    cy = c * (np.cos(alpha_rad) - np.cos(beta_rad) * np.cos(gamma_rad)) / np.sin(gamma_rad)
    cz = c * np.sqrt(1 - np.cos(beta_rad)**2 - ((np.cos(alpha_rad) - np.cos(beta_rad) * np.cos(gamma_rad)) / np.sin(gamma_rad))**2)
    
    v3 = np.array([cx, cy, cz])
    
    return np.array([v1, v2, v3])


def direct_cartesian_to_fractional(atoms=None, cart_coords=None, Box=None, add_to_atoms=True):
    """
    Direct conversion from cartesian coordinates to fractional coordinates.
    This function provides a direct implementation that follows the MATLAB approach
    without intermediate orthogonalization steps.
    
    Parameters
    ----------
    atoms : list of dict, optional
        List of atom dictionaries with 'x', 'y', 'z' cartesian coordinates.
        If provided, these will be used for the conversion. Either atoms or cart_coords must be provided.
    cart_coords : numpy.ndarray, optional
        Nx3 array of cartesian coordinates, where N is the number of atoms.
        Either atoms or cart_coords must be provided.
    Box : list or array
        Simulation cell dimensions (1x3, 1x6, or 1x9).
    add_to_atoms : bool, optional
        If True and atoms is provided, adds fractional coordinates to the atom dictionaries
        as 'xfrac', 'yfrac', 'zfrac'. Default is True.
        
    Returns
    -------
    frac_coords : numpy.ndarray
        Nx3 array of fractional coordinates, where N is the number of atoms.
    atoms : list of dict, optional
        The original atoms list with updated fractional coordinate fields
        if add_to_atoms is True.
    """
    if atoms is None and cart_coords is None:
        raise ValueError("Either atoms or cart_coords must be provided")
    if Box is None:
        raise ValueError("Box must be provided")
    
    _, Cell = normalize_box(Box)
    
    # Extract Box parameters
    a, b, c, alpha, beta, gamma = Cell
    
    # Convert angles to radians
    alpha_rad = np.radians(alpha)
    beta_rad = np.radians(beta)
    gamma_rad = np.radians(gamma)
    
    # Calculate trigonometric values
    cos_alpha = np.cos(alpha_rad)
    cos_beta = np.cos(beta_rad)
    cos_gamma = np.cos(gamma_rad)
    sin_gamma = np.sin(gamma_rad)
    
    # Calculate volume term
    v = np.sqrt(1 - cos_alpha**2 - cos_beta**2 - cos_gamma**2 + 
                2 * cos_alpha * cos_beta * cos_gamma)
    
    # Build transformation matrix from cartesian to fractional (ToFrac in MATLAB)
    to_frac = np.array([
        [1/a, -cos_gamma / (a * sin_gamma), 
         (cos_alpha * cos_gamma - cos_beta) / (a * v * sin_gamma)],
        [0, 1 / (b * sin_gamma), 
         (cos_beta * cos_gamma - cos_alpha) / (b * v * sin_gamma)],
        [0, 0, sin_gamma / (c * v)]
    ])
    
    # Get cartesian coordinates from atoms if needed
    if cart_coords is None:
        cart_coords = np.array([[atom['x'], atom['y'], atom['z']] for atom in atoms])
    
    # Apply transformation to each point
    frac_coords = np.zeros_like(cart_coords)
    for i, cart in enumerate(cart_coords):
        frac_coords[i] = np.dot(to_frac, cart)
    
    # Add fractional coordinates to atoms if requested
    if atoms is not None and add_to_atoms:
        for i, atom in enumerate(atoms):
            atom['xfrac'] = float(round(frac_coords[i, 0], 6))
            atom['yfrac'] = float(round(frac_coords[i, 1], 6))
            atom['zfrac'] = float(round(frac_coords[i, 2], 6))
        return frac_coords, atoms
    
    return frac_coords


def direct_fractional_to_cartesian(atoms=None, frac_coords=None, Box=None, add_to_atoms=True):
    """
    Direct conversion from fractional coordinates to Cartesian coordinates.
    This function provides a direct implementation that follows the MATLAB approach
    without intermediate orthogonalization steps.
    
    Parameters
    ----------
    atoms : list of dict, optional
        List of atom dictionaries with 'xfrac', 'yfrac', 'zfrac' fractional coordinates.
        If provided, these will be used for the conversion. Either atoms or frac_coords must be provided.
    frac_coords : numpy.ndarray, optional
        Nx3 array of fractional coordinates, where N is the number of atoms.
        Either atoms or frac_coords must be provided.
    Box : list or array
        Simulation cell dimensions (1x3, 1x6, or 1x9).
    add_to_atoms : bool, optional
        If True and atoms is provided, adds cartesian coordinates to the atom dictionaries
        as 'x', 'y', 'z'. Default is True.
        
    Returns
    -------
    cart_coords : numpy.ndarray
        Nx3 array of cartesian coordinates, where N is the number of atoms.
    atoms : list of dict, optional
        The original atoms list with updated cartesian coordinate fields
        if add_to_atoms is True.
    """
    if atoms is None and frac_coords is None:
        raise ValueError("Either atoms or frac_coords must be provided")
    if Box is None:
        raise ValueError("Box must be provided")
    
    _, Cell = normalize_box(Box)
    
    # Extract Box parameters
    a, b, c, alpha, beta, gamma = Cell
    
    # Convert angles to radians
    alpha_rad = np.radians(alpha)
    beta_rad = np.radians(beta)
    gamma_rad = np.radians(gamma)
    
    # Calculate trigonometric values
    cos_alpha = np.cos(alpha_rad)
    cos_beta = np.cos(beta_rad)
    cos_gamma = np.cos(gamma_rad)
    sin_gamma = np.sin(gamma_rad)
    
    # Calculate volume term
    v = np.sqrt(1 - cos_alpha**2 - cos_beta**2 - cos_gamma**2 + 
                2 * cos_alpha * cos_beta * cos_gamma)
    
    # Build transformation matrix from fractional to cartesian (FromFrac in MATLAB)
    from_frac = np.array([
        [a, b * cos_gamma, c * cos_beta],
        [0, b * sin_gamma, c * (cos_alpha - cos_beta * cos_gamma) / sin_gamma],
        [0, 0, c * v / sin_gamma]
    ])
    
    # Get fractional coordinates from atoms if needed
    if frac_coords is None:
        frac_coords = np.array([[atom.get('xfrac', 0.0), 
                                atom.get('yfrac', 0.0), 
                                atom.get('zfrac', 0.0)] 
                                for atom in atoms])
    
    # Apply transformation to each point
    cart_coords = np.zeros_like(frac_coords)
    for i, frac in enumerate(frac_coords):
        cart_coords[i] = np.dot(from_frac, frac)
    
    # Add cartesian coordinates to atoms if requested
    if atoms is not None and add_to_atoms:
        for i, atom in enumerate(atoms):
            atom['x'] = float(round(cart_coords[i, 0], 4))
            atom['y'] = float(round(cart_coords[i, 1], 4))
            atom['z'] = float(round(cart_coords[i, 2], 4))
        return cart_coords, atoms
    
    return cart_coords

def scale(atoms, Box, scale_factors, resname=None):
    """
    Scale atom coordinates and box dimensions.
    
    
    Parameters
    ----------
    atoms : list of dict
        List of atom dictionaries with coordinates
    Box : list of float
        Box dimensions (1x3, 1x6, or 1x9).
    scale_factors : float or sequence of float
        A single number (or length-1 vector) scales x, y and z ISOTROPICALLY
        (box and all coordinates by the same factor). A length-3 vector
        [sx, sy, sz] scales each axis — and the corresponding box edge — by its
        own factor, e.g. [1.0, 1.02, 0.975]. Accepts numpy scalars/arrays too.
    resname : str, optional
        Only scale atoms with this residue name, if provided. Default is None (all atoms).
        
    Returns
    -------
    tuple
        (scaled_atoms, scaled_box) - Atoms with scaled coordinates and new box dimensions
    
    Notes
    -----
    - For triclinic cells, the function first converts to orthogonal coordinates, 
      applies scaling, and then converts back to triclinic.
    - The function preserves the original cell angles for triclinic boxes.

    Examples
    --------
    atoms, Box_dim = ap.import_gro("structure.gro")
    atoms_scaled, Box_scaled = scale(atoms, Box_dim, 2.0)
    atoms_scaled, Box_scaled = scale(atoms, Box_dim, [1.0, 1.0, 2.0], resname="WAT")
    """
    # Make a deep copy to avoid modifying the original atoms
    atoms = [atom.copy() for atom in atoms]

    # Normalize scale_factors. A scalar or length-1 vector scales x/y/z ISOTROPICALLY;
    # a length-3 vector [sx, sy, sz] scales each axis by the corresponding factor.
    # Accepts Python numbers, lists/tuples, and numpy scalars/arrays.
    _sf = np.atleast_1d(np.asarray(scale_factors, dtype=float)).ravel()
    if _sf.size == 1:
        scale_factors = [float(_sf[0]), float(_sf[0]), float(_sf[0])]
    elif _sf.size == 3:
        scale_factors = [float(_sf[0]), float(_sf[1]), float(_sf[2])]
    else:
        raise ValueError("scale_factors must be a single number, a length-1 vector "
                         "(isotropic), or a length-3 vector [sx, sy, sz]")
    
    # Determine which atoms to scale
    if resname is not None:
        indices_to_scale = [i for i, atom in enumerate(atoms) 
                           if atom.get('resname', '').upper() == resname.upper()]
    else:
        indices_to_scale = list(range(len(atoms)))
    
    # Normalize box -> orthogonal Box_dim + [a, b, c, alpha, beta, gamma] cell.
    Box_dim, Cell = normalize_box(Box)

    # Scale by keeping FRACTIONAL coordinates fixed and scaling the cell edges per axis.
    # This is correct for both orthogonal and triclinic cells (angles preserved): atoms
    # stay at the same fractional positions, so the box and all coordinates scale together.
    new_cell = [Cell[0] * scale_factors[0], Cell[1] * scale_factors[1], Cell[2] * scale_factors[2],
                Cell[3], Cell[4], Cell[5]]
    new_box = Cell2Box_dim(new_cell)

    if atoms:
        cart = np.array([[a['x'], a['y'], a['z']] for a in atoms], dtype=float)
        frac = cartesian_to_fractional(cart_coords=cart, Box=Box_dim, add_to_atoms=False)
        cart_new = fractional_to_cartesian(frac_coords=frac, Box=new_box, add_to_atoms=False)
        for i in indices_to_scale:   # write back only the selected atoms (all, unless resname given)
            atoms[i]['x'] = float(cart_new[i, 0])
            atoms[i]['y'] = float(cart_new[i, 1])
            atoms[i]['z'] = float(cart_new[i, 2])

    return atoms, new_box
