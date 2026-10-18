"""
Solvent module for atomipy - provides functions for handling solvent molecules.

This module provides functions for:
- Identifying solvent molecules (e.g., water) in molecular systems
- Solvating structures with water or other solvents
"""

import copy
import os
import random
import numpy as np
from typing import Union
from . import config


def find_H2O(atoms, Box_dim=None, rmin=1.30):
    """
    Find water molecules in a molecular system using distance-based detection.

    Identifies water molecules by computing distances and finding oxygen atoms
    that have exactly 2 hydrogen atoms within the O-H bond distance cutoff.
    This works regardless of atom order or existing molid assignments.

    Parameters
    ----------
    atoms : list of dict
        List of atom dictionaries. Should contain 'element' or 'type' fields
        for atom identification, and 'x', 'y', 'z' coordinates.
    Box_dim : list of float, optional
        Box dimensions for periodic boundary conditions. If None, non-periodic
        distances are used (default: None).
    rmin : float, optional
        Maximum O-H bond distance in Angstroms for water detection
        (default: 1.25, matches typical water O-H bond length).

    Returns
    -------
    water_atoms : list of dict
        List of atom dictionaries belonging to water molecules. Atoms are
        updated with new molids and types ('Ow', 'Hw').
    non_water_atoms : list of dict
        Remaining atoms that are not part of water molecules.

    Notes
    -----
    - Uses distance matrix to find O atoms with exactly 2 H neighbors
    - Assigns new sequential molids to each water molecule
    - Changes atom types to 'Ow' and 'Hw' for water oxygens and hydrogens
    - Sets resname to 'SOL' for all water atoms
    - Works regardless of input atom order or existing molid values

    Examples
    --------
    atoms, Box_dim = ap.import_gro("system.gro")
    atoms = ap.element(atoms)
    SOL, noSOL = ap.find_H2O(atoms, Box_dim)
    print(f"Found {len(SOL)//3} water molecules")

    See Also
    --------
    assign_resname : Assign residue names including 'SOL' for water
    """
    from .distances import neighbor_list_fast
    
    if not atoms:
        return [], []
    
    atoms = copy.deepcopy(atoms)
    
    # 1. Identify types/elements for all atoms once
    N = len(atoms)
    is_oxygen = np.zeros(N, dtype=bool)
    is_hydrogen = np.zeros(N, dtype=bool)
    
    for i, atom in enumerate(atoms):
        element = atom.get('element', '')
        atom_type = atom.get('type', atom.get('atom_name', ''))
        
        # Check element first (can be 'O', 'Ow', 'Oh', etc), then type
        is_o = (element and element[0].upper() == 'O') or \
               (not element and atom_type and atom_type[0].upper() == 'O')
        is_h = (element and element[0].upper() == 'H') or \
               (not element and atom_type and atom_type[0].upper() == 'H')
        
        if is_o:
            is_oxygen[i] = True
        elif is_h:
            is_hydrogen[i] = True
    
    if not np.any(is_oxygen) or np.sum(is_hydrogen) < 2:
        print("Found 0 water molecules (0 atoms)")
        return [], atoms
    
    # 2. Use the central dispatcher to find all pairs within rmin
    # It automatically selects between Direct and Sparse based on config.SPARSE_THRESHOLD
    from .distances import get_neighbor_list
    i_idx, j_idx, dists, _, _, _ = get_neighbor_list(atoms, Box_dim, cutoff=rmin, rmaxH=rmin)
    
    # 3. Filter for O-H pairs
    # Since neighbor_list_fast returns i < j, we check both (i=O, j=H) and (i=H, j=O)
    is_oh_pair = (is_oxygen[i_idx] & is_hydrogen[j_idx]) | (is_hydrogen[i_idx] & is_oxygen[j_idx])
    
    oh_i = i_idx[is_oh_pair]
    oh_j = j_idx[is_oh_pair]
    oh_dists = dists[is_oh_pair]
    
    # Map H indices to their closest O neighbor
    h_to_o = {}
    for k in range(len(oh_i)):
        idx1, idx2 = oh_i[k], oh_j[k]
        o_idx = idx1 if is_oxygen[idx1] else idx2
        h_idx = idx2 if is_oxygen[idx1] else idx1
        
        dist = oh_dists[k]
        if h_idx not in h_to_o or dist < h_to_o[h_idx][1]:
            h_to_o[h_idx] = (o_idx, dist)
    
    # Map O indices back to their assigned H neighbors
    o_to_h = {}
    for h_idx, (o_idx, dist) in h_to_o.items():
        if o_idx not in o_to_h:
            o_to_h[o_idx] = []
        o_to_h[o_idx].append(h_idx)
    
    # 4. Find water molecules: O with exactly 2 H neighbors
    water_atoms = []
    water_indices_set = set()
    molid = 1
    
    # Sort O indices for deterministic results
    for o_idx in sorted(o_to_h.keys()):
        h_neighbors = o_to_h[o_idx]
        
        if len(h_neighbors) == 2:
            h1_idx, h2_idx = h_neighbors
            
            # Check these atoms haven't already been assigned to water
            if o_idx in water_indices_set or h1_idx in water_indices_set or h2_idx in water_indices_set:
                continue
            
            # Create water molecule atoms
            for idx, role in [(o_idx, 'Ow'), (h1_idx, 'Hw'), (h2_idx, 'Hw')]:
                atom = copy.deepcopy(atoms[idx])
                atom['molid'] = molid
                atom['type'] = role
                atom['resname'] = 'SOL'
                water_atoms.append(atom)
                water_indices_set.add(idx)
            
            molid += 1
    
    # Create non-water atoms list
    non_water_atoms = [copy.deepcopy(atoms[i]) for i in range(len(atoms)) 
                       if i not in water_indices_set]
    
    # Update indices for non-water atoms
    for i, atom in enumerate(non_water_atoms):
        atom['index'] = i + 1
    
    # Update indices for water atoms
    for i, atom in enumerate(water_atoms):
        atom['index'] = i + 1
        # Remove temporary original index marker
        if '_orig_index' in atom:
            del atom['_orig_index']
    
    n_water = len(water_atoms) // 3
    print(f"Found {n_water} water molecules ({len(water_atoms)} atoms)")
    
    return water_atoms, non_water_atoms


def _declash_solvent(solv_atoms, box_dim, heavy_cut=2.0, h_cut=1.0):
    """Drop solvent molecules that overlap OTHER solvent molecules.

    The water template is replicated and sliced to fill the box, so molecules from
    neighbouring tiles (or across the periodic boundary) can land on top of each other
    — these clashes produce near-infinite forces that make MD explode ("water cannot be
    settled") unless minimized first.

    Overlap is judged per pair: an inter-molecular pair is a clash if it is closer than
    ``h_cut`` (default 1.0 Å) when either atom is a hydrogen, or ``heavy_cut`` (default
    2.0 Å) otherwise. Using the smaller cutoff for H preserves genuine hydrogen bonds
    (O···H ≈ 1.8 Å); only true overlaps are removed. Greedy: for each too-close
    inter-molecular pair, drop one of the two molecules.

    ``solvate`` always derives these from its general ``min_distance`` knob (O–O =
    min_distance, O–H/H–H = min_distance/2), whether or not a solute is present, so the
    knob tightens/loosens the water packing consistently.
    """
    if not solv_atoms:
        return solv_atoms
    from .distances import get_neighbor_list
    heavy_cut = float(heavy_cut)
    h_cut = float(h_cut)
    search_cut = max(heavy_cut, h_cut)
    try:
        i_idx, j_idx, dists, *_ = get_neighbor_list(solv_atoms, box_dim, cutoff=search_cut)
    except Exception:
        return solv_atoms
    molid = [a.get('molid') for a in solv_atoms]
    is_h = [str(a.get('type', '')).upper().startswith('H') for a in solv_atoms]
    removed = set()
    for a, b, d in zip(i_idx, j_idx, dists):
        ma, mb = molid[a], molid[b]
        if ma == mb:
            continue
        thr = h_cut if (is_h[a] or is_h[b]) else heavy_cut
        if d < thr and ma not in removed and mb not in removed:
            removed.add(max(ma, mb))
    if removed:
        kept = [a for a in solv_atoms if a.get('molid') not in removed]
        print(f"  Removed {len(removed)} overlapping solvent molecule(s) "
              f"(< {heavy_cut:g} Å heavy / {h_cut:g} Å H; tile-seam/PBC clashes)")
        return kept
    return solv_atoms


def solvate(limits, density=1000.0, min_distance=2.0, max_solvent: Union[str, int] = 'max',
           solute_atoms=None, Box=None, solvent_type='spce', custom_solvent=None, custom_box=None,
           include_solute=False):
    """
    Solvate a structure or region with water or other solvent molecules.
    
    Performance Note
    ----------------
    This function uses the central dispatcher `get_neighbor_list` for shell distance 
    calculations, automatically switching to a memory-efficient sparse neighbor list 
    for systems larger than `config.SPARSE_THRESHOLD` to prevent O(N^2) memory bottlenecks.
    
    Parameters
    ----------
    limits : list of float
        Region limits [xlo, ylo, zlo, xhi, yhi, zhi] or [xhi, yhi, zhi] 
        (the latter assumes xlo=ylo=zlo=0)
    density : float, optional
        Solvent density in kg/m³, default is 1000.0 (water)
    min_distance : float or list of float, optional
        Minimum distance between solute and solvent atoms. 
        Can be a single value or [larger, smaller]
        where smaller applies to hydrogens and larger to other atoms.
        Default is 2.0 Å.
    max_solvent : int or str, optional
        Maximum number of solvent molecules, or 'max' for all possible,
        or 'shell' for a shell around solute. If 'shell', can be one of
        'shell10', 'shell15', 'shell20', 'shell25', or 'shell30' for different
        shell thicknesses in Å. Default is 'max'.
    solute_atoms : list of dict, optional
        Solute atoms to be solvated. Default is None.
    solvent_type : str, optional
        Type of solvent ('spce', 'tip3p', etc.). Default is 'spce'.
    custom_solvent : list of dict, optional
        Custom solvent atoms structure. Default is None.
    custom_box : list of float, optional
        Box dimensions for the custom solvent. Default is None.
    include_solute : bool, optional
        If True, return solute + solvent; if False (default), return solvent only.
        
    Returns
    -------
    list of dict
        Solvated structure (solvent only or solute + solvent)
        
    Notes
    -----
    - If solute_atoms is provided, atoms from the solvent that overlap with 
      the solute will be removed.
    - The 'shell' option creates a solvent shell of specified thickness around the solute.
    - Atom types in water molecules are typically 'OW', 'HW1', and 'HW2'.
    
    Examples
    --------
    # Solvate a box region:
    water = ap.solvate([0, 0, 0, 30, 30, 30])
    
    # Solvate around a protein with shell of 10 Å:
    water = ap.solvate([0, 0, 0, 60, 60, 60], solute_atoms=protein, max_solvent='shell10')
    """
    # Import functions here to avoid circular imports
    from .build import merge, slice as build_slice
    from .distances import dist_matrix, get_neighbor_list
    from .cell_utils import Cell2Box_dim
    from . import import_conf
    
    # Standardize limits. len 3 = [xhi,yhi,zhi] (orthogonal, lo=0); len 6 =
    # [xlo,ylo,zlo,xhi,yhi,zhi] (orthogonal sub-region); len 9 = a full triclinic cell in
    # GROMACS Box_dim format [lx,ly,lz, 0,0,xy, 0,xz,yz] (origin at 0, whole cell only).
    is_triclinic = False
    tri_box = None
    if len(limits) == 3:
        xlo, ylo, zlo = 0, 0, 0
        xhi, yhi, zhi = limits
    elif len(limits) == 6:
        xlo, ylo, zlo, xhi, yhi, zhi = limits
    elif len(limits) == 9:
        tri_box = [float(v) for v in limits]
        # Only a non-zero xy/xz/yz tilt needs the triclinic fill; a zero-tilt 9-vector is
        # just an orthogonal box and takes the faster rectangular path below.
        is_triclinic = any(abs(tri_box[i]) > 1e-9 for i in (5, 7, 8))
        xlo, ylo, zlo = 0.0, 0.0, 0.0
        xhi, yhi, zhi = tri_box[0], tri_box[1], tri_box[2]
        if Box is None:
            Box = tri_box  # PBC for declash / solute-merge uses the real triclinic cell
    else:
        raise ValueError(
            "Limits must be length 3 [xhi,yhi,zhi], 6 [xlo,ylo,zlo,xhi,yhi,zhi], or 9 "
            "(triclinic Box_dim [lx,ly,lz, 0,0,xy, 0,xz,yz])"
        )

    # Box dimensions (diagonal). A triclinic GROMACS box is lower-triangular, so its cell
    # volume is still lx*ly*lz — this is the correct fill volume for the density estimate.
    box_dim = [xhi - xlo, yhi - ylo, zhi - zlo]
    
    # Parse shell thickness if provided
    shell_thickness = None
    if isinstance(max_solvent, str) and max_solvent.startswith('shell'):
        if max_solvent == 'shell10':
            shell_thickness = 10.0
        elif max_solvent == 'shell15':
            shell_thickness = 15.0
        elif max_solvent == 'shell20':
            shell_thickness = 20.0
        elif max_solvent == 'shell25':
            shell_thickness = 25.0
        elif max_solvent == 'shell30':
            shell_thickness = 30.0
        elif max_solvent == 'shell':
            shell_thickness = 10.0  # Default
        else:
            try:
                # Try to extract thickness from string like 'shell12.5'
                shell_thickness = float(max_solvent[5:])
            except ValueError:
                shell_thickness = 10.0  # Default if parsing fails
    
    # Load solvent structure
    if custom_solvent is not None and custom_box is not None:
        solvent_atoms = copy.deepcopy(custom_solvent)
        solvent_box = custom_box
    else:
        # Load standard solvent
        solvent_atoms, solvent_box = _load_solvent(solvent_type)
    
    # Find how many atoms per solvent molecule (e.g., 3 for SPC water)
    unique_molids = set(atom['molid'] for atom in solvent_atoms)
    atoms_per_molecule = len(solvent_atoms) / len(unique_molids)

    # Scale the pre-equilibrated solvent template to the requested density. The template
    # is at ~1 g/cm³; to hit `density` (kg/m³) we keep the molecule count fixed and
    # rescale the VOLUME by rho_template/density, i.e. an isotropic linear factor
    # (rho_template/density)**(1/3). So density 1050 kg/m³ compresses the template (and
    # its box) by ~1.6 %/axis, density 950 expands it — making the density knob actually
    # change the packing instead of being ignored above ~1 g/cm³.
    from .transform import scale as _scale
    _M_water = 18.01528          # g/mol
    _NA = 6.02214076e23
    _v_template_m3 = (solvent_box[0] * solvent_box[1] * solvent_box[2]) * 1e-30  # Å³ -> m³
    if _v_template_m3 > 0:
        _rho_template = (len(unique_molids) * _M_water / _NA / 1000.0) / _v_template_m3  # kg/m³
        if density and density > 0 and _rho_template > 0 and abs(density - _rho_template) / _rho_template > 1e-3:
            _s = (_rho_template / float(density)) ** (1.0 / 3.0)
            solvent_atoms, solvent_box = _scale(solvent_atoms, solvent_box, _s)
            print(f"  Solvent template scaled by {_s:.4f}/axis -> target density {density/1000.0:.3f} g/cm³ "
                  f"(template ~{_rho_template/1000.0:.3f} g/cm³)")

    # Calculate how many solvent molecules are needed to fill the Box
    # For water, 1000 kg/m³ is about 33.3 molecules per nm³
    volume_nm3 = (box_dim[0] / 10) * (box_dim[1] / 10) * (box_dim[2] / 10)
    molecules_per_nm3 = density / 30  # Approximate for water
    n_molecules_needed = int(volume_nm3 * molecules_per_nm3)

    # An explicit integer max_solvent is a TARGET count, not just an upper bound:
    # don't let a low density estimate under-fill it (the box-fill branch below would
    # otherwise place only the density-derived number). A shortfall is reported after
    # placement (raises below) so the caller knows the box couldn't hold the request.
    requested_count = max_solvent if isinstance(max_solvent, int) else None
    if requested_count is not None:
        n_molecules_needed = requested_count
    
    if is_triclinic:
        # Fill a triclinic parallelepiped: tile the template across the cell's orthogonal
        # bounding box, then keep whole molecules whose reference atom has fractional
        # coordinates inside the cell (0 <= s < 1). Seam / periodic-image clashes are
        # cleaned by the triclinic-aware declash below.
        lx, ly, lz = box_dim[0], box_dim[1], box_dim[2]
        xy, xz, yz = tri_box[5], tri_box[7], tri_box[8]
        H = np.array([[lx, 0.0, 0.0], [xy, ly, 0.0], [xz, yz, lz]], dtype=float)  # rows = cell vectors
        Hinv = np.linalg.inv(H)
        corners = np.array([a * H[0] + b * H[1] + c * H[2]
                            for a in (0.0, 1.0) for b in (0.0, 1.0) for c in (0.0, 1.0)])
        bb_min = corners.min(axis=0)
        bb_max = corners.max(axis=0)
        ntx = int(np.ceil((bb_max[0] - bb_min[0]) / solvent_box[0]))
        nty = int(np.ceil((bb_max[1] - bb_min[1]) / solvent_box[1]))
        ntz = int(np.ceil((bb_max[2] - bb_min[2]) / solvent_box[2]))
        full_solvent = []
        _tile = 0
        for ix in range(ntx):
            for iy in range(nty):
                for iz in range(ntz):
                    ox = bb_min[0] + ix * solvent_box[0]
                    oy = bb_min[1] + iy * solvent_box[1]
                    oz = bb_min[2] + iz * solvent_box[2]
                    for atom in solvent_atoms:
                        new_atom = atom.copy()
                        new_atom['x'] = atom['x'] + ox
                        new_atom['y'] = atom['y'] + oy
                        new_atom['z'] = atom['z'] + oz
                        new_atom['molid'] = atom['molid'] + _tile * len(unique_molids)
                        full_solvent.append(new_atom)
                    _tile += 1
        # Keep molecules whose first atom is inside the cell (0 <= fractional < 1).
        _mols = {}
        for a in full_solvent:
            _mols.setdefault(a['molid'], []).append(a)
        _eps = 1e-6
        sliced_solvent = []
        for _mol in _mols.values():
            _s = np.array([_mol[0]['x'], _mol[0]['y'], _mol[0]['z']]) @ Hinv
            if (-_eps <= _s[0] < 1.0) and (-_eps <= _s[1] < 1.0) and (-_eps <= _s[2] < 1.0):
                sliced_solvent.extend(_mol)
    else:
        # Orthogonal box / sub-region: tile then rectangular-slice.
        nx = int(np.ceil((xhi - xlo) / solvent_box[0]))
        ny = int(np.ceil((yhi - ylo) / solvent_box[1]))
        nz = int(np.ceil((zhi - zlo) / solvent_box[2]))
        full_solvent = []
        for ix in range(nx):
            for iy in range(ny):
                for iz in range(nz):
                    for atom in solvent_atoms:
                        new_atom = atom.copy()
                        new_atom['x'] = atom['x'] + ix * solvent_box[0] + xlo
                        new_atom['y'] = atom['y'] + iy * solvent_box[1] + ylo
                        new_atom['z'] = atom['z'] + iz * solvent_box[2] + zlo
                        # Adjust molid to keep track of different molecules
                        new_atom['molid'] = atom['molid'] + (ix * ny * nz + iy * nz + iz) * len(unique_molids)
                        full_solvent.append(new_atom)
        # Slice to get only atoms within the target region
        sliced_solvent = build_slice(full_solvent, [xlo, ylo, zlo, xhi, yhi, zhi])

    # Remove solvent-solvent overlaps (replicated-template tile-seam / PBC clashes) UP
    # FRONT, so the molecule-count selection below draws from a clash-free pool (and an
    # explicit count isn't undercut by clashes removed after selection).
    #
    # `min_distance` is the single, general spacing knob: it governs BOTH solvent↔solvent
    # declashing here AND solute↔solvent clearance (via merge, below), whether or not a
    # solute is present. Heavy–heavy pairs use min_distance; any pair involving an H uses
    # min_distance/2 (H---H / O---H sites can sit closer, preserving genuine H-bonds).
    # A [heavy, h] list is honoured as-is. The default min_distance=2.0 reproduces the
    # legacy 2.0 Å / 1.0 Å thresholds, so default results are unchanged.
    if isinstance(min_distance, (list, tuple)):
        _declash_heavy = float(min_distance[0])
        _declash_h = float(min_distance[1]) if len(min_distance) > 1 else float(min_distance[0]) / 2.0
    else:
        _declash_heavy = float(min_distance)
        _declash_h = float(min_distance) / 2.0
    # Use the REAL cell for PBC in the solvent-solvent neighbour search, not the sliced
    # sub-region: when `limits` is a slab inside a larger `Box` (e.g. one interlayer),
    # box_dim would wrap the slab's own faces together and declash molecules that aren't
    # actually periodic neighbours. Box == box_dim for a full-box solvation, so this is a
    # no-op there.
    _declash_box = Box if Box is not None else box_dim
    sliced_solvent = _declash_solvent(sliced_solvent, _declash_box, heavy_cut=_declash_heavy, h_cut=_declash_h)

    # Randomize the order of molecules for unbiased selection
    unique_molids = set(atom['molid'] for atom in sliced_solvent)
    molid_list = list(unique_molids)
    random.shuffle(molid_list)
    
    if shell_thickness is None and solute_atoms is None:
        # Just fill the Box
        if n_molecules_needed < len(molid_list):
            # Take only the required number of molecules
            selected_molids = set(molid_list[:n_molecules_needed])
            solvent_result = [atom for atom in sliced_solvent 
                             if atom['molid'] in selected_molids]
        else:
            solvent_result = sliced_solvent
    elif shell_thickness is not None and solute_atoms is not None:
        # Create a shell of solvent around the solute
        # First, merge solvent with solute, removing overlaps
        non_overlapping = merge(solute_atoms, sliced_solvent, Box if Box is not None else box_dim, 
                               atom_label=['HW1', 'HW2'], 
                               min_distance=[min_distance, min_distance/2])
        
        # Now create a shell by keeping only molecules within the shell distance
        # but not closer than min_distance
        combined = solute_atoms + non_overlapping
        
        # Use the central dispatcher to get neighbors and distances
        # It automatically handles Direct vs Sparse based on config.SPARSE_THRESHOLD
        #
        # KNOWN LIMITATION (triclinic shell mode): this shell-distance neighbour search
        # passes `box_dim` (the orthogonal diagonal [lx,ly,lz]), NOT the real triclinic
        # `Box`. For a tilted cell the PBC minimum image is therefore approximated as
        # orthogonal, so molecules near the sheared faces may be included/excluded slightly
        # wrongly at the shell boundary. The full-box fill, declash, and solute-merge paths
        # all use the true triclinic Box and are correct; only this 'shellNN' cutoff is
        # affected. Harmless for orthogonal cells (Box == box_dim). If triclinic shell
        # solvation is ever needed, pass `Box if Box is not None else box_dim` here (and
        # verify get_neighbor_list's triclinic min-image against a known reference).
        n_solute = len(solute_atoms)
        i_idx, j_idx, dists, _, _, _ = get_neighbor_list(
            combined,
            box_dim,
            cutoff=shell_thickness,
            dm_method=None
        )
        
        # Filter for solute-solvent pairs
        # i_idx < j_idx holds in the dispatcher. We need i < n_solute and j >= n_solute
        mask = (i_idx < n_solute) & (j_idx >= n_solute)
        
        # Map which solvent indices are within reach
        shell_indices = set(j_idx[mask] - n_solute)
        shell_molids = set(non_overlapping[idx]['molid'] for idx in shell_indices)
        
        # Keep only molecules in the shell
        solvent_result = [atom for atom in non_overlapping 
                         if atom['molid'] in shell_molids]
        
        # If max_solvent is a number, limit the molecules
        if isinstance(max_solvent, int):
            unique_shell_molids = list(shell_molids)
            random.shuffle(unique_shell_molids)
            if max_solvent < len(unique_shell_molids):
                selected_molids = set(unique_shell_molids[:max_solvent])
                solvent_result = [atom for atom in solvent_result 
                                 if atom['molid'] in selected_molids]
    else:
        # Just remove overlapping solvent molecules
        solvent_result = merge(solute_atoms, sliced_solvent, Box if Box is not None else box_dim,
                              atom_label=['HW1', 'HW2'],
                              min_distance=[min_distance, min_distance/2])
        
        # If max_solvent is a number, limit the molecules
        if isinstance(max_solvent, int):
            unique_molids = set(atom['molid'] for atom in solvent_result)
            molid_list = list(unique_molids)
            random.shuffle(molid_list)
            if max_solvent < len(molid_list):
                selected_molids = set(molid_list[:max_solvent])
                solvent_result = [atom for atom in solvent_result 
                                 if atom['molid'] in selected_molids]
    
    # Calculate statistics for output
    n_solvent_molecules = len(set(atom['molid'] for atom in solvent_result))
    n_solvent_atoms = len(solvent_result)

    # If the caller requested an EXACT number of molecules (box fill / overlap removal,
    # not a 'shell' cap) and the box couldn't hold them, fail loudly rather than
    # silently returning fewer. The water template is pre-equilibrated at ~1 g/cm³, so a
    # higher density cannot pack more — the limit is the box volume / min_distance.
    if requested_count is not None and shell_thickness is None and n_solvent_molecules < requested_count:
        raise ValueError(
            f"solvate: could only place {n_solvent_molecules} of the requested {requested_count} "
            f"solvent molecules in the {box_dim[0]:.1f} x {box_dim[1]:.1f} x {box_dim[2]:.1f} A box "
            f"(min_distance={min_distance} A). Water packs at ~1 g/cm3, so increasing the density "
            f"cannot add more. Enlarge the box, lower the requested count, or reduce min_distance."
        )

    # Calculate subvolume
    volume_angstrom3 = box_dim[0] * box_dim[1] * box_dim[2]
    
    # Print the solvation statistics
    print(f"  Subvolume: {box_dim[0]:.2f} x {box_dim[1]:.2f} x {box_dim[2]:.2f} Å³ = {volume_angstrom3:.2f} Å³")
    print(f"  Added {n_solvent_molecules} water molecules ({n_solvent_atoms} atoms)")
    
    # Renumber the selected solvent molecules to CONTIGUOUS molids. The replicated/
    # sliced template leaves sparse, non-consecutive molids (e.g. 2,3,5,…,1728 for 1000
    # molecules); downstream writers group residues by molid, so sparse ids corrupt the
    # .gro/.top (mismatched molecule count -> grompp/settle failure). Start after the
    # real solute (if any), else at 1.
    start_molid = 1
    if solute_atoms:  # a real, non-empty solute
        start_molid = max((a['molid'] for a in solute_atoms), default=0) + 1
    _remap = {}
    _next = start_molid
    for atom in solvent_result:
        om = atom['molid']
        if om not in _remap:
            _remap[om] = _next
            _next += 1
        atom['molid'] = _remap[om]

    # Combine solute + solvent (only a real, non-empty solute is prepended).
    if solute_atoms and include_solute:
        result = list(solute_atoms) + solvent_result
    else:
        result = solvent_result

    return result


def _load_solvent(solvent_type='spce'):
    """
    Load a pre-equilibrated solvent Box.
    
    Parameters
    ----------
    solvent_type : str, optional
        Type of solvent ('spce', 'tip3p', etc.)
        
    Returns
    -------
    tuple
        (solvent_atoms, solvent_box)
    """
    from . import import_conf
    from .cell_utils import Cell2Box_dim
    
    # Define path to solvent structures
    base_path = os.path.dirname(os.path.abspath(__file__))
    structures_path = os.path.join(base_path, 'structures', 'water')
    
    # Map solvent type to filename
    solvent_files = {
        'spce': '864_spce.pdb',
        'spc': '864_spce.pdb',  # Alias
        'tip3p': '864_tip3p.pdb',
        'opc3': '864_tip3p.pdb',  # OPC3 shares TIP3P 3-site structure template
        'tip4p': '864_tip4p.pdb',
        'opc': '864_tip4p.pdb',   # OPC shares TIP4P 4-site structure template
    }
    
    # Get the appropriate solvent file
    if solvent_type.lower() in solvent_files:
        solvent_file = os.path.join(structures_path, solvent_files[solvent_type.lower()])
    else:
        raise ValueError(f"Unsupported solvent type: {solvent_type}")
    
    # Import the solvent structure
    atoms, Cell = import_conf.pdb(solvent_file)
    box_dim = Cell2Box_dim(Cell)
    
    return atoms, box_dim

def spc2tip4p(atoms, Box=None, om_dist=0.15):
    """
    Convert SPC water molecules to TIP4P model by adding an M dummy site.
    
    Parameters
    ----------
    atoms : list of dict
        List of atom dictionaries (should already be identified as water).
    Box : list of float, optional
        Box dimensions for PBC handling.
    om_dist : float, optional
        Distance from oxygen to the M site along the bisector (default 0.15 A).
        
    Returns
    -------
    list of dict
        New list of atoms with M sites added.
    """
    # Group by molid
    molids = np.array([a['molid'] for a in atoms])
    unique_molids = np.unique(molids)
    
    new_atoms = []
    
    for mid in unique_molids:
        mol_indices = np.where(molids == mid)[0]
        if len(mol_indices) != 3:
            # Not a 3-site water molecule, copy as is
            for idx in mol_indices:
                new_atoms.append(copy.deepcopy(atoms[idx]))
            continue
            
        # Find O and Hs
        o_idx = -1
        h_indices = []
        for idx in mol_indices:
            atom = atoms[idx]
            atype = atom.get('type', '').upper()
            element = atom.get('element', '').upper()
            if 'O' in atype or element == 'O':
                o_idx = idx
            elif 'H' in atype or element == 'H':
                h_indices.append(idx)
        
        if o_idx == -1 or len(h_indices) != 2:
            # Not a standard water structure
            for idx in mol_indices:
                new_atoms.append(copy.deepcopy(atoms[idx]))
            continue
            
        o_atom = copy.deepcopy(atoms[o_idx])
        h1_atom = copy.deepcopy(atoms[h_indices[0]])
        h2_atom = copy.deepcopy(atoms[h_indices[1]])
        
        # Add O and Hs to new list
        new_atoms.extend([o_atom, h1_atom, h2_atom])
        
        # Calculation of M site
        o_pos = np.array([o_atom['x'], o_atom['y'], o_atom['z']])
        h1_pos = np.array([h1_atom['x'], h1_atom['y'], h1_atom['z']])
        h2_pos = np.array([h2_atom['x'], h2_atom['y'], h2_atom['z']])
        
        v1 = h1_pos - o_pos
        v2 = h2_pos - o_pos
        
        if Box is not None:
            # PBC correction for vectors
            if len(Box) == 3:
                v1 -= np.round(v1 / np.array(Box)) * np.array(Box)
                v2 -= np.round(v2 / np.array(Box)) * np.array(Box)
            elif len(Box) == 9:
                lx, ly, lz = Box[0], Box[1], Box[2]
                v1[0] -= lx * np.round(v1[0] / lx)
                v1[1] -= ly * np.round(v1[1] / ly)
                v1[2] -= lz * np.round(v1[2] / lz)
                v2[0] -= lx * np.round(v2[0] / lx)
                v2[1] -= ly * np.round(v2[1] / ly)
                v2[2] -= lz * np.round(v2[2] / lz)

        # Normalized vectors
        v1_u = v1 / np.linalg.norm(v1)
        v2_u = v2 / np.linalg.norm(v2)
        
        # Bisector
        bisector = v1_u + v2_u
        bisector_u = bisector / np.linalg.norm(bisector)
        
        m_pos = o_pos + om_dist * bisector_u
        
        # Create M atom
        m_atom = copy.deepcopy(o_atom)
        m_atom['x'], m_atom['y'], m_atom['z'] = m_pos
        m_atom['type'] = 'MW'
        m_atom['element'] = 'M'
        m_atom['charge'] = -1.0484  # Example charge for TIP4P
        m_atom['index'] = 0 # Will be updated by update()
        
        # Adjust O and H charges for TIP4P if needed, 
        # but usually the user handles forcefield assignment separately.
        # However, for convenience we can set some defaults.
        o_atom['charge'] = 0.0
        h1_atom['charge'] = 0.5242
        h2_atom['charge'] = 0.5242
        
        new_atoms.append(m_atom)
        
    from .build import update
    return update(new_atoms)

def tip3p2tip4p(atoms, Box=None):
    """Alias for spc2tip4p using default TIP4P distance."""
    return spc2tip4p(atoms, Box, om_dist=0.15)

