"""
Forcefield utilities for atom typing and statistics (MINFF, CLAYFF).


Examples
--------
import atomipy as ap
atoms, Box = ap.import_gro("structure.gro")
atoms = ap.minff(atoms, Box)  # Assign MINFF types
atoms = ap.clayff(atoms, Box) # Assign CLAYFF types
"""

import numpy as np
from .bond_angle import bond_angle
from .cell_utils import Box_dim2Cell, Cell2Box_dim, normalize_box
from .charge import charge_minff, charge_clayff, assign_formal_charges
from .element import element  # Correct function name is 'element' not 'set_element'
from .mass import set_atomic_masses

def get_structure_stats(atoms, Box=None, total_charge=None, log_file='output.log', ffname='minff'):
    """Generate statistics about atom types, coordination, and charges in the structure.
    
    This function analyzes atom types, their coordination environment, charges,
    coordination numbers, bond distances, and angles, and outputs a formatted report.
    The report can be written to a log file and/or returned as a string.
    
    Args:
        atoms: List of atom dictionaries containing 'type', 'neigh', 'charge', etc. keys.
        Box: Optional simulation cell dimensions. Accepts 1x3 (orthogonal),
             1x6 (Cell parameters), or 1x9 (triclinic GROMACS-style) arrays.
        total_charge: Total charge of the system. If None, it will be calculated.
        log_file: Path to the output log file (default: 'output.log').
        ffname: The name of the forcefield used, e.g., 'minff' or 'clayff' (default: 'minff').
    
    Returns:
        A string containing the structure statistics.
    """
    if Box is not None:
        from .bond_angle import bond_angle
        # Check if neighbor data is present by sampling the first 10 atoms
        if atoms and not any(atoms[i].get('neigh') for i in range(min(len(atoms), 10))):
            print(f"  No coordination data found. Auto-calculating bonds for {ffname} report...")
            # Use bond_angle to populate neigh, bonds, and angles
            atoms, _, _ = bond_angle(atoms, Box, calculate_coordination=True)

    # Check if charges are missing or all zero
    has_charges = atoms and any(atom.get('charge') is not None for atom in atoms)
    if not has_charges or sum(abs(float(atom.get('charge', 0) or 0)) for atom in atoms) < 1e-10:
        from .charge import assign_formal_charges
        print("  No pre-defined charges found. Auto-assigning formal charges for report...")
        atoms = assign_formal_charges(atoms)

    if total_charge is None:
        total_charge = sum(float(a.get('charge', 0) or 0) for a in atoms)
    
    import numpy as np
    from collections import defaultdict
    import math
    # Conversion factor for density calculation: amu/Å³ to g/cm³
    # 1 amu = 1.66053886e-24 g and 1 Å³ = 1e-24 cm³
    AMU_TO_G_PER_CM3 = 1.66053886
    
    # Normalize Box input
    Box_dim = None
    Cell = None
    if Box is not None:
        Box_dim, Cell = normalize_box(Box)
    
    # Initialize statistics storage
    all_neighbor_str_list = []
    atom_type_counts = {}
    coord_nums = defaultdict(list)  # Store coordination numbers by atom type
    bond_dists = defaultdict(list)  # Store bond distances by atom type
    angles = defaultdict(list)      # Store angles by atom type
    h_involved_angles = defaultdict(list)  # Store angles where hydrogen is an outer atom
    
    # Additional statistics storage for atom type pairs and triplets
    bond_type_pairs = defaultdict(list)  # Store bond distances by atom type pair (e.g., H-Oh)
    angle_type_triplets = defaultdict(list)  # Store angles by atom type triplet (e.g., H-Oh-Alo)
    
    # Gather coordination information
    for i, atom in enumerate(atoms):
        atom_type = atom.get('type', 'X')
        
        # Count atom types
        if atom_type in atom_type_counts:
            atom_type_counts[atom_type] += 1
        else:
            atom_type_counts[atom_type] = 1
        
        # Get neighbor atom types for coordination environment
        neighbor_indices = atom.get('neigh', [])
        neighbor_types = [atoms[neigh_idx].get('type', 'X') for neigh_idx in neighbor_indices]
        
        # Collect coordination number
        cn = len(neighbor_indices)
        coord_nums[atom_type].append(cn)
        
        # Collect bond distances if available
        if 'bonds' in atom:
            for bond in atom['bonds']:
                # Bond format is (neighbor_idx, distance)
                neighbor_idx, distance = bond
                bond_dists[atom_type].append(distance)
                
                # Collect bond type pairs
                neighbor_type = atoms[neighbor_idx].get('type', 'X')
                # Use alphabetical ordering of atom types for consistency
                bond_pair = tuple(sorted([atom_type, neighbor_type]))
                bond_type_pairs[bond_pair].append(distance)
        
        # Collect angles if available
        if 'angles' in atom:
            for angle_data in atom['angles']:
                # angle_data format is ((neigh1_idx, neigh2_idx), angle_value)
                (neigh1_idx, neigh2_idx), angle_value = angle_data
                angles[atom_type].append(angle_value)
                
                # Special handling for hydrogen-involved angles
                # If either of the outer atoms is hydrogen, record this angle for that hydrogen type
                neigh1_type = atoms[neigh1_idx].get('type', '')
                neigh2_type = atoms[neigh2_idx].get('type', '')
                
                if neigh1_type.startswith('H'):
                    h_involved_angles[neigh1_type].append(angle_value)
                if neigh2_type.startswith('H'):
                    h_involved_angles[neigh2_type].append(angle_value)
                    
                # Collect angle type triplets
                # Create a unique representation for the angle triplet
                # For A-B-C, where B is the center atom, order as (min(A,C), B, max(A,C))
                # This ensures we group equivalent angles like A-B-C and C-B-A together
                if neigh1_type <= neigh2_type:
                    angle_triplet = (neigh1_type, atom_type, neigh2_type)
                else:
                    angle_triplet = (neigh2_type, atom_type, neigh1_type)
                    
                angle_type_triplets[angle_triplet].append(angle_value)
        
        # Create a sorted neighbor string
        all_neighbor_str = ''.join(sorted(neighbor_types))
        all_neighbor_str_list.append((atom_type, all_neighbor_str, atom.get('charge', 0.0)))
    
    # Create a consolidated dictionary of unique atom types, their coordination, and charges
    unique_patterns = {}
    for atom_type, neighbor_str, charge in all_neighbor_str_list:
        key = (atom_type, neighbor_str)
        
        if key in unique_patterns:
            unique_patterns[key]['count'] += 1
            unique_patterns[key]['charges'].append(charge)
        else:
            unique_patterns[key] = {
                'count': 1,
                'charges': [charge]
            }
    
    # Format output
    output = []
    
    # Calculate total mass for density calculation
    total_mass = sum(atom.get('mass', 0.0) or 0.0 for atom in atoms)
    
    # Add Box dimensions, volume, and density information
    volume = None
    if Box_dim is not None or Cell is not None:
        output.append("System Dimensions and Properties")
        output.append("-" * 80)
        
        # Box_dim representation
        if Box_dim is not None:
            output.append("Box_dim (Å):")
            if len(Box_dim) == 3:
                output.append(f"  Orthogonal Box: [{Box_dim[0]:.4f}, {Box_dim[1]:.4f}, {Box_dim[2]:.4f}]")
                volume = Box_dim[0] * Box_dim[1] * Box_dim[2]
                output.append(f"  Volume: {volume:.4f} Å³")
            elif len(Box_dim) == 6:
                output.append(f"  Triclinic Box: [{Box_dim[0]:.4f}, {Box_dim[1]:.4f}, {Box_dim[2]:.4f}, {Box_dim[3]:.4f}, {Box_dim[4]:.4f}, {Box_dim[5]:.4f}]")
            elif len(Box_dim) == 9:
                output.append(f"  Full matrix: [{Box_dim[0]:.4f}, {Box_dim[1]:.4f}, {Box_dim[2]:.4f}, {Box_dim[3]:.4f}, {Box_dim[4]:.4f}, {Box_dim[5]:.4f}, {Box_dim[6]:.4f}, {Box_dim[7]:.4f}, {Box_dim[8]:.4f}]")
        
        # Cell representation
        if Cell is not None:
            a, b, c = Cell[0], Cell[1], Cell[2]
            alpha, beta, gamma = Cell[3], Cell[4], Cell[5]
            output.append("Cell parameters:")
            output.append(f"  a, b, c (Å): {a:.4f}, {b:.4f}, {c:.4f}")
            output.append(f"  α, β, γ (°): {alpha:.4f}, {beta:.4f}, {gamma:.4f}")
            
            # Calculate volume from Cell parameters
            if abs(alpha - 90) < 1e-6 and abs(beta - 90) < 1e-6 and abs(gamma - 90) < 1e-6:
                # Orthogonal Box
                volume = a * b * c
            else:
                # Triclinic Box, use the general formula
                alpha_rad = math.radians(alpha)
                beta_rad = math.radians(beta)
                gamma_rad = math.radians(gamma)
                volume = a * b * c * math.sqrt(1 - math.cos(alpha_rad)**2 - math.cos(beta_rad)**2 - 
                                              math.cos(gamma_rad)**2 + 
                                              2 * math.cos(alpha_rad) * math.cos(beta_rad) * math.cos(gamma_rad))
            output.append(f"  Volume: {volume:.4f} Å³")
            
        # Calculate and display density
        # Only calculate if we have a volume and total mass is non-zero
        if volume is not None and total_mass > 0:
            # Convert from amu/Å³ to g/cm³
            density = total_mass / volume * AMU_TO_G_PER_CM3
            output.append("System properties:")
            output.append(f"  Total mass: {total_mass:.4f} amu")
            output.append(f"  Density: {density:.4f} g/cm³")
        
        # Explanation of variables
        output.append("\nBox_dim and Cell explanations:")
        output.append("  Box_dim: A 1D array of Box dimensions, typically in Angstroms.")
        output.append("          For orthogonal boxes: [Lx, Ly, Lz]")
        output.append("          For triclinic boxes: [Lx, Ly, Lz, xy, xz, yz] or [Lx, Ly, Lz, α, β, γ]")
        output.append("  Cell: A 1×6 array with Cell parameters [a, b, c, α, β, γ]")
        output.append("        where a, b, c are lengths and α, β, γ are angles in degrees.")
        output.append("-" * 80)
        output.append("")
    
    total_charge = float(total_charge) if total_charge is not None else 0.0
    output.append(f"Total charge: {total_charge:.6f}\n")
    
    if abs(round(total_charge) - total_charge) > 1e-5:
        output.append("Warning: Non-integer total charge. Adjusting to nearest integer.")
        target_charge = round(total_charge)
        output.append(f"Final total charge: {sum(atom.get('charge', 0) for atom in atoms):.6f} (target was {target_charge})")
    
    output.append("\nUnique Atom Types and Their Coordination Environment")
    output.append("-" * 80)
    output.append(f"{'Type':<10} {'Count':<6} {'Neighbors':<20} {'Charge':>15}")
    output.append("-" * 80)
    
    # Sort by atom type for a more organized display
    for key in sorted(unique_patterns.keys()):
        atom_type, neighbor_pattern = key
        count = unique_patterns[key]['count']
        charges = unique_patterns[key]['charges']
        
        # Find unique charge values (with some tolerance for floating point comparison)
        unique_charges = []
        for raw_charge in charges:
            # Cast to float to avoid TypeError if charges are strings
            try:
                charge = float(raw_charge)
            except (ValueError, TypeError):
                charge = 0.0
                
            # Only add if this is a new unique charge (accounting for floating point precision)
            if not any(abs(charge - uc) < 1e-6 for uc in unique_charges):
                unique_charges.append(charge)
        
        # If there's only one unique charge, display it as before
        if len(unique_charges) == 1:
            charge_str = f"{unique_charges[0]:.6f}"
        else:
            # Otherwise, display all unique charges separated by commas
            charge_str = ', '.join([f"{c:.6f}" for c in sorted(unique_charges)])
        
        output.append(f"{atom_type:<10} {count:<6} {neighbor_pattern:<20} {charge_str:>15}")
    
    output.append("-" * 80)
    
    # Add detailed statistics for average coordination, bond distances, and angles
    output.append("\nDetailed Atom Type Statistics with Standard Deviations")
    output.append("-" * 80)
    header = f"{'Type':<10} {'Count':<6} {'Coord#':<16} {'Bond Dist (Å)':<20} {'Angle (°)':<20}"
    output.append(header)
    output.append("-" * 80)
    
    for atom_type, count in sorted(atom_type_counts.items()):
        # Calculate average coordination number and std dev
        cn_data = coord_nums[atom_type]
        avg_cn = np.mean(cn_data) if cn_data else 0
        std_cn = np.std(cn_data) if len(cn_data) > 1 else 0
        cn_str = f"{avg_cn:.2f} ± {std_cn:.2f}" if cn_data else "N/A"
        
        # Calculate average bond distance and std dev
        dist_data = bond_dists[atom_type]
        avg_dist = np.mean(dist_data) if dist_data else 0
        std_dist = np.std(dist_data) if len(dist_data) > 1 else 0
        dist_str = f"{avg_dist:.4f} ± {std_dist:.4f}" if dist_data else "N/A"
        
        # Calculate average angle and std dev
        angle_data = angles[atom_type]
        # For hydrogen atom types with no angles (as center atom), use the angles where H is involved
        if not angle_data and atom_type.startswith('H') and atom_type in h_involved_angles:
            h_angle_data = h_involved_angles[atom_type]
            avg_angle = np.mean(h_angle_data) if h_angle_data else 0
            std_angle = np.std(h_angle_data) if len(h_angle_data) > 1 else 0
            angle_str = f"{avg_angle:.3f} ± {std_angle:.3f} (H-O-M)" if h_angle_data else "N/A"
        else:
            avg_angle = np.mean(angle_data) if angle_data else 0
            std_angle = np.std(angle_data) if len(angle_data) > 1 else 0
            angle_str = f"{avg_angle:.3f} ± {std_angle:.3f}" if angle_data else "N/A"
        
        # Format the output line
        output.append(f"{atom_type:<10} {count:<6} {cn_str:<16} {dist_str:<20} {angle_str:<20}")
    
    output.append("-" * 80)
    
    # Add bond statistics for unique atom type pairs
    output.append("\nBond Statistics for Unique Atom Type Pairs")
    output.append("-" * 80)
    output.append(f"{'Bond Pair':<25} {'Count':<8} {'Distance (Å)':<20}")
    output.append("-" * 80)
    
    # Sort by bond pair for a more organized display
    for bond_pair, distances in sorted(bond_type_pairs.items()):
        type1, type2 = bond_pair
        pair_str = f"{type1}-{type2}"
        count = len(distances)
        avg_dist = np.mean(distances)
        std_dist = np.std(distances) if len(distances) > 1 else 0
        
        output.append(f"{pair_str:<25} {count:<8} {avg_dist:.4f} ± {std_dist:.4f}")
    
    output.append("-" * 80)
    
    # Add angle statistics for unique atom type triplets
    output.append("\nAngle Statistics for Unique Atom Type Triplets")
    output.append("-" * 80)
    output.append(f"{'Angle Triplet':<25} {'Count':<8} {'Angle (°)':<20}")
    output.append("-" * 80)
    
    # Sort by angle triplet for a more organized display
    for angle_triplet, angles_data in sorted(angle_type_triplets.items()):
        type1, type2, type3 = angle_triplet
        triplet_str = f"{type1}-{type2}-{type3}"
        count = len(angles_data)
        avg_angle = np.mean(angles_data)
        std_angle = np.std(angles_data) if len(angles_data) > 1 else 0
        
        output.append(f"{triplet_str:<25} {count:<8} {avg_angle:.3f} ± {std_angle:.3f}")
    
    output.append("-" * 80)
    
    # Join everything into a string
    result = "\n".join(output)
    
    # Write to log file if specified
    if log_file:
        with open(log_file, 'w') as f:
            f.write(result + "\n")
    
    return result

_WATER_RESNAMES = {'SOL', 'HOH', 'WAT', 'H2O', 'TIP', 'TIP3', 'TIP4', 'TIP5',
                   'TIP3P', 'TIP4P', 'TIP5P', 'SPC', 'SPCE', 'OPC', 'OPC3'}
_ION_ELEMENTS = {'Na', 'K', 'Li', 'Cs', 'Rb', 'Cl', 'F', 'Br', 'I',
                 'Ca', 'Mg', 'Sr', 'Ba', 'Zn'}


def _is_solvent_or_ion(atom):
    """True for water molecules and monatomic counter-ions — atoms that must NOT count
    toward a mineral framework atom's coordination during MINFF/CLAYFF typing.

    Because the framework's coordination is detected geometrically (``same_molecule_only=
    False``), an interlayer cation (Na-O ~2.3 Å, inside rmaxM) or a nearby water would
    otherwise inflate a surface oxygen's coordination and mistype it (e.g. ``Op`` -> ``O_ov``).

    Water is matched by residue name. An ion is matched only when its element is a common
    counter-ion AND its residue is named for that ion (e.g. resname ``Na``), so a dual-use
    element in the framework (octahedral Mg/Ca/Li inside resname ``MIN``/``MMT``) is NOT
    excluded. molid is deliberately not used, so this stays correct even after a PDB
    round-trip has turned molid per-atom.
    """
    rn = str(atom.get('resname', '') or '').strip()
    if rn.upper() in _WATER_RESNAMES:
        return True
    el = str(atom.get('element', '') or atom.get('type', '') or '').strip()
    el_cap = el.capitalize()
    if el_cap in _ION_ELEMENTS and rn.upper() in {el.upper(), 'ION', 'IONS'}:
        return True
    return False


def minff(atoms, Box, ffname='minff', rmaxlong=2.45, rmaxH=1.2, log=False, log_file=None, dm_method=None):
    """Assign MINFF forcefield specific atom types to atoms based on their coordination environment.
    
    This function updates the 'fftype' field based on the atom's element and its bonding environment,
    using a two-pass approach to first determine coordination numbers and then assign types based on
    structural environment.
    
    Water molecules (residue 'SOL') and Ions (residue 'ION') are treated specially: their atom types
    are standardized (e.g., 'Ow', 'Hw', 'Na') and preserved, skipping the structural type assignment.
    
    For details, see the MINFF forcefield documentation at https://github.com/mholmboe/minff.
    
    Args:
        atoms: A list of atom dictionaries, each atom is expected to have position coordinates
              and element/type information.
        Box: a 1x3, 1x6 or 1x9 list representing Cell dimensions (in Angstroms):
            - For orthogonal boxes, a 1x3 list [lx, ly, lz] where Box = Box_dim, and Cell would be [lx, ly, lz, 90, 90, 90]
            - For Cell parameters, a 1x6 list [a, b, c, alpha, beta, gamma] (Cell format)
            - For triclinic boxes, a 1x9 list [lx, ly, lz, 0, 0, xy, 0, xz, yz] (GROMACS Box_dim format)
        ffname: The forcefield name, default is 'minff'.
        rmaxlong: Maximum bond distance for non-hydrogen bonds, default is 2.45 Å.
        rmaxH: Maximum bond distance for hydrogen bonds, default is 1.2 Å.
        log: If True, generate statistics and write to a log file, default is False.
        log_file: Optional path to a log file. If provided and log is True, statistics will be
                 written to this file instead of the default filename.
    
    Returns:
        The updated atoms list with 'fftype' fields assigned.

    Examples
    --------
    import atomipy as ap
    atoms, Box = ap.import_gro("structure.gro")
    atoms = ap.minff(atoms, Box)
    atoms = ap.minff(atoms, [50, 50, 50], log=True, log_file="minff_stats.log")
    """
    # Set the atoms chemical element names
    atoms = element(atoms)  # Use correct function name 'element'

    # First assign formal charges to all atoms (especially for ions and water)
    # This sets appropriate charges based on atom types and residue names
    atoms = assign_formal_charges(atoms)

    # Set atom masses using the mass.py module
    atoms = set_atomic_masses(atoms)

    # Determine Box format and convert as needed
    if len(Box) == 9:
        # Triclinic Box in GROMACS format [lx, ly, lz, 0, 0, xy, 0, xz, yz]
        Box_dim = Box
        Cell = Box_dim2Cell(Box_dim)
    elif len(Box) == 6:
        # Cell parameters [a, b, c, alpha, beta, gamma]
        Cell = Box
        Box_dim = Cell2Box_dim(Cell)
    elif len(Box) == 3:
        # Simple orthogonal Box [lx, ly, lz]
        Box_dim = Box
        Cell = list(Box) + [90.0, 90.0, 90.0]
    else:
        raise ValueError("Box must be length 3, 6, or 9")
    
    # Run the entire process twice to ensure all atoms have proper typing
    # This is especially important for oxygen atoms which need to know
    # whether their metal neighbors are tetrahedral or octahedral
    for _ in range(2):  # Run the typing process twice
        # First, ensure all atoms have element types defined
        for atom in atoms:
            #if 'element' not in atom:
            # Try to extract element from atom type
            atom_type = atom.get('type', 'X')
            
            # Convert to lowercase for case-insensitive comparison
            atom_type_lower = atom_type.lower()
            
            # Map atom types to elements based on first 1-3 characters
            if atom.get('resname') == 'SOL':
                if atom_type_lower.startswith('o'):
                    atom['element'] = 'Ow'
                elif atom_type_lower.startswith('h'):
                    atom['element'] = 'Hw'
            elif atom.get('resname') == 'ION':
                if atom_type_lower.startswith('sod') or atom_type_lower.startswith('na'):
                    atom['element'] = 'Na'
                elif atom_type_lower.startswith('cla') or atom_type_lower.startswith('cl'):
                    atom['element'] = 'Cl'
                elif atom_type_lower.startswith('pot') or atom_type_lower.startswith('k'):
                    atom['element'] = 'K'
            elif atom_type_lower.startswith('si'):  
                atom['element'] = 'Si'
            elif atom_type_lower.startswith('sc'):  
                atom['element'] = 'Si'
            elif atom_type_lower.startswith('ale'): 
                atom['element'] = 'Ale'
            elif atom_type_lower.startswith('alt'): 
                atom['element'] = 'Alt'
            elif atom_type_lower.startswith('al'):  
                atom['element'] = 'Al'
            elif atom_type_lower.startswith('mg'):  
                atom['element'] = 'Mg'
            elif atom_type_lower.startswith('ca'):  
                atom['element'] = 'Ca'
            elif atom_type_lower.startswith('fee'): 
                atom['element'] = 'Fee'
            elif atom_type_lower.startswith('fet'): 
                atom['element'] = 'Fet'
            elif atom_type_lower.startswith('fe'):  
                atom['element'] = 'Fe'
            elif atom_type_lower.startswith('f'):   
                atom['element'] = 'F'
            elif atom_type_lower.startswith('li'):  
                atom['element'] = 'Li'
            elif atom_type_lower.startswith('ow'):  
                atom['element'] = 'Ow'
            elif atom_type_lower.startswith('hw'):  
                atom['element'] = 'Hw'
            elif atom_type_lower.startswith('o'):   
                atom['element'] = 'O'
            elif atom_type_lower.startswith('h'):   
                atom['element'] = 'H'
            elif atom_type_lower.startswith('ti'):   
                atom['element'] = 'Ti'
            else:
                atom['element'] = atom_type
        
        # Initialize atom types and fftypes to match element type
        for atom in atoms:
            atom['type'] = atom['element']
            atom['fftype'] = atom['element']
        
        # Only calculate bonds in the first pass
        if _ == 0:
            # Get bonds and angles using bond_angle function (this also calculates coordination numbers).
            # same_molecule_only=False: mineral framework coordination is purely geometric and must
            # NOT depend on molid. Imported/uploaded structures (and replication) may carry per-atom or
            # per-residue molids, which would otherwise make the default same-molecule filter find ZERO
            # bonds, leaving every atom untyped (e.g. 'Al' instead of 'Alo' -> downstream KeyError).
            atoms, bond_index, angle_index = bond_angle(atoms, Box, rmaxH=rmaxH, rmaxM=rmaxlong,
                                                        same_molecule_only=False)

            # Drop water/ion neighbours from each FRAMEWORK atom's coordination before typing.
            # bond_angle above is geometric, so an interlayer cation or nearby water would
            # otherwise be counted as bonded to a surface oxygen and mistype it (e.g. Op -> O_ov).
            # This is molid-independent (robust to per-atom molids from a PDB round-trip) and
            # cheap: an O(N) classification + an O(bonds) neighbour filter. bond_angle itself
            # is unchanged, so no extra neighbour-search cost is added.
            _nonframework = {i for i, a in enumerate(atoms) if _is_solvent_or_ion(a)}
            if _nonframework:
                for i, atom in enumerate(atoms):
                    if i in _nonframework:
                        continue
                    _neigh = atom.get('neigh', [])
                    _kept = [j for j in _neigh if j not in _nonframework]
                    if len(_kept) != len(_neigh):
                        atom['neigh'] = _kept
                        atom['cn'] = len(_kept)
                        _bonds = atom.get('bonds', [])
                        if _bonds:
                            atom['bonds'] = [b for b in _bonds if b[0] not in _nonframework]

            # Store bond information and prepare for atom typing
            for i, atom in enumerate(atoms):
                # Skip water and ion residues if present
                if _is_solvent_or_ion(atom):
                    continue
                    
                # Get neighbors from the bonds
                neighbors = atom.get('neigh', [])
                if not neighbors:
                    continue
                    
                # Use the coordination number already calculated by bond_angle
                atom['coord_num'] = atom.get('cn', 0)
                
                # For Fe atoms, calculate average Fe-O bond distance to determine oxidation state
                if atom['element'] == 'Fe':
                    # Get bonds to this atom
                    bonds = atom.get('bonds', [])
                    if bonds:
                        # Extract distances and calculate average
                        bond_distances = [dist for _, dist in bonds]
                        avg_bond_distance = sum(bond_distances) / len(bond_distances)
                        atom['avg_bond_dist'] = avg_bond_distance
        
        # Assign atom types based on coordination and bond information
        for i, atom in enumerate(atoms):
            # Skip water and ion residues
            if _is_solvent_or_ion(atom):
                continue
                
            # Get neighbors from the bonds
            neighbors = atom.get('neigh', [])
            if not neighbors:
                continue
                
            # Get neighbor types for pattern matching
            neighbor_types = [atoms[neigh_idx]['element'] for neigh_idx in neighbors]
            neighbor_types.sort()
            neighbors_str = ''.join(neighbor_types)
            
            # Number of neighbors (coordination number)
            coord_num = atom.get('coord_num', 0)
            
            # Determine fftype based on element and coordination environment
            
            # Lithium assignments
            if atom.get('type', '').lower().startswith('li'):
                if coord_num == 6:
                    atom['fftype'] = 'Lio'
                elif coord_num == 4:
                    atom['fftype'] = 'Lio'
                elif coord_num > 6:
                    atom['fftype'] = 'Lio_ov'  # Over-coordinated
                elif 4 < coord_num < 6:
                    atom['fftype'] = 'Lio_un'  # Under-coordinated
            
            # Silicon assignments
            elif atom.get('type', '').lower().startswith('si'):
                o_neighbors = neighbor_types.count('O')
                if o_neighbors == 4:
                    atom['fftype'] = 'Sit'
                elif o_neighbors == 3:
                    atom['fftype'] = 'Site'  # As in Stishovite
                elif o_neighbors == 6:
                    atom['fftype'] = 'Sio'   # As in Stishovite
                elif coord_num > 4:
                    atom['fftype'] = 'Si_ov'  # Over-coordinated
                elif coord_num < 4:
                    atom['fftype'] = 'Si_un'  # Under-coordinated
            
            # Aluminum assignments
            elif atom.get('type', '').lower().startswith('al'):
                o_neighbors = neighbor_types.count('O')
                if o_neighbors == 6:
                    atom['fftype'] = 'Alo'     # Octahedral Al
                elif o_neighbors == 5:
                    atom['fftype'] = 'Ale'    # 5-coordinated Al
                elif o_neighbors == 4:
                    atom['fftype'] = 'Alt'    # Tetrahedral Al
                elif coord_num > 6:
                    atom['fftype'] = 'Al_ov'  # Over-coordinated
                elif coord_num < 4:
                    atom['fftype'] = 'Al_un'  # Under-coordinated
            
            # Magnesium assignments
            elif atom.get('type', '').lower().startswith('mg'):
                if coord_num == 6:

                    atom['fftype'] = 'Mgo'

                    # Check if there are more Mg than Si (e.g. in forsterite)
                    h_count = sum(1 for a in atoms if a.get('element') == 'H')
                    mg_count = sum(1 for a in atoms if a.get('element') == 'Mg')
                    si_count = sum(1 for a in atoms if a.get('element') == 'Si')
                    al_count = sum(1 for a in atoms if a.get('element') == 'Al')
                    fe_count = sum(1 for a in atoms if a.get('element') == 'Fe')
                    
                    if mg_count > si_count:
                        atom['fftype'] = 'Mgo'  # E.g. in forsterite
                        if mg_count < h_count:
                            atom['fftype'] = 'Mgh'  # Ex. Brucite
                    elif mg_count <= si_count:
                        atom['fftype'] = 'Mgh'  # E.g. in Talc, Hectorite
                        if al_count > mg_count or fe_count > mg_count:
                            atom['fftype'] = 'Mgo'  # Ex. Mica, Smectite

                elif coord_num > 6:
                    atom['fftype'] = 'Mg_ov'  # Over-coordinated
                elif coord_num < 6:
                    atom['fftype'] = 'Mg_un'  # Under-coordinated

            # Titanium assignments
            elif atom.get('type', '').lower().startswith('ti'):
                o_neighbors = neighbor_types.count('O')
                if coord_num == 6:
                    atom['fftype'] = 'Tio'  # Rutile/anatase type (TiO2)
                elif coord_num == 4:
                    atom['fftype'] = 'Tit'  # Tetrahedral Ti
                elif coord_num > 6:
                    atom['fftype'] = 'Ti_ov'  # Over-coordinated
                elif coord_num < 4:
                    atom['fftype'] = 'Ti_un'  # Under-coordinated
                    
            # Calcium assignments
            elif atom.get('type', '').lower().startswith('ca'):
                o_neighbors = neighbor_types.count('O')
                f_neighbors = neighbor_types.count('F')
                
                if o_neighbors == 6:
                    atom['fftype'] = 'Cao'  # Octahedral Ca
                elif o_neighbors == 4:
                    atom['fftype'] = 'Cah'  # 4-coordinated Ca
                elif f_neighbors == 8:
                    # Likely in fluorite (CaF2) structure
                    print(f"Ca in CaF2 Fluorite? (atom index: {atom.get('index', '?')})")
                    atom['fftype'] = 'Cah'
                elif coord_num > 6:
                    print(f"Ca atom over coordinated (atom index: {atom.get('index', '?')})")
                    print(f"Neighbors: {neighbor_types}")
                    atom['fftype'] = 'Cao_ov'  # Over-coordinated
                elif coord_num < 6:
                    print(f"Ca atom under coordinated (atom index: {atom.get('index', '?')})")
                    print(f"Neighbors: {neighbor_types}")
                    atom['fftype'] = 'Cao_un'  # Under-coordinated
                else:
                    # Fall back for other cases
                    print(f"Ca with unusual coordination (atom index: {atom.get('index', '?')})")
                    print(f"Neighbors: {neighbor_types}")
            
            # Iron assignments with Fe3+/Fe2+ distinction based on bond distance
            elif atom.get('type', '').lower().startswith('fe'):
                o_neighbors = neighbor_types.count('O')
                avg_bond_dist = atom.get('avg_bond_dist', 0)
                
                if o_neighbors == 6:  # Octahedral Fe
                    if avg_bond_dist < 2.07:  # Fe3+ site
                        atom['fftype'] = 'Feo3'
                    else:  # Fe2+ site
                        atom['fftype'] = 'Feo2'
                elif o_neighbors == 5:  # Edge Fe
                    if avg_bond_dist < 2.0:  # Fe3+ site
                        atom['fftype'] = 'Fee3'
                    else:  # Fe2+ site
                        atom['fftype'] = 'Fee2'
                elif o_neighbors == 4:  # Tetrahedral Fe
                    if avg_bond_dist < 2.0:  # Fe3+ site (typical distance cutoff for tetrahedral)
                        atom['fftype'] = 'Fet3'
                    else:  # Fe2+ site
                        atom['fftype'] = 'Fet2'
                        print("Do you really have a tetrahedral Fe2+ site?")
                elif coord_num > 6:
                    atom['fftype'] = 'Fe_ov'  # Over-coordinated
                elif coord_num < 4:
                    atom['fftype'] = 'Fe_un'  # Under-coordinated
            
            # Fluoride assignments
            elif atom.get('type', '').lower().startswith('f'):
                if coord_num == 3:
                    atom['fftype'] = 'Fs'
                elif coord_num == 4:
                    print(f"Fs atom as in CaF2 - Fluorite? (atom index: {atom.get('index', '?')})")
                    atom['fftype'] = 'Fs'
                elif coord_num > 4:
                    print(f"Fs atom over coordinated (atom index: {atom.get('index', '?')})")
                    print(f"Neighbors: {neighbor_types}")
                    atom['fftype'] = 'Fs_ov'  # Over-coordinated
                elif coord_num < 3:
                    print(f"Fs atom under coordinated (atom index: {atom.get('index', '?')})")
                    print(f"Neighbors: {neighbor_types}")
                    atom['fftype'] = 'Fs_un'  # Under-coordinated
                else:
                    print(f"F with unusual coordination (atom index: {atom.get('index', '?')})")
                    print(f"Neighbors: {neighbor_types}")
            
            # Hydrogen assignments
            elif atom.get('type', '').lower().startswith('h'):
                if coord_num == 1:
                    atom['fftype'] = 'H'
                elif coord_num > 1:
                    atom['fftype'] = 'H_ov'  # Over-coordinated
            
            # Oxygen assignments - based on neighbor pattern
            elif atom.get('type', '').lower().startswith('o'):
                # Begin with basic cases based on key neighbor patterns
                if neighbors_str == 'AlAlAl' or neighbors_str == 'AlAlAlAl':
                    atom['fftype'] = 'Ob'
                elif neighbors_str == 'AlAlAlAlt':
                    atom['fftype'] = 'Obt'
                elif neighbors_str == 'AlAlAlH':
                    atom['fftype'] = 'Oh'
                elif neighbors_str == 'AlAlAlt' or neighbors_str == 'AlAlt':
                    atom['fftype'] = 'Ops'
                elif neighbors_str == 'AlAlFe' or neighbors_str == 'AlAlFeo':
                    atom['fftype'] = 'Ob'
                elif neighbors_str == 'AlAlH':
                    atom['fftype'] = 'Oh'
                elif neighbors_str == 'AlAlSi':
                    atom['fftype'] = 'Op'
                elif neighbors_str == 'AlAlSiSi':
                    atom['fftype'] = 'Oz'
                elif neighbors_str == 'AlAleH' or neighbors_str == 'AleH':
                    atom['fftype'] = 'Oh'
                elif neighbors_str == 'AlAleSi' or neighbors_str == 'AleSi':
                    atom['fftype'] = 'Ob'
                elif neighbors_str == 'AlAltH':
                    atom['fftype'] = 'Ops'
                elif neighbors_str == 'AlFeFe' or neighbors_str == 'AlFeoFeo' or neighbors_str == 'AltFeFe' or neighbors_str == 'AltFeoFeo':
                    atom['fftype'] = 'Ops'
                elif neighbors_str == 'AlFeFet' or neighbors_str == 'AlFeoFet':
                    atom['fftype'] = 'Ops'
                elif neighbors_str == 'AlAltFe' or neighbors_str == 'AlAltFeo':
                    atom['fftype'] = 'Ops'
                elif neighbors_str == 'AlFeH' or neighbors_str == 'AlFeoH':
                    atom['fftype'] = 'Oh'
                elif neighbors_str == 'AlFeSi' or neighbors_str == 'AlFeoSi':
                    atom['fftype'] = 'Op'
                elif neighbors_str == 'AlFet' or neighbors_str == 'AlAlFet':
                    atom['fftype'] = 'Ops'
                elif neighbors_str == 'AlH':
                    atom['fftype'] = 'Oalh'  # Al-O-H or Al-O-Si
                elif neighbors_str == 'AlHH':
                    atom['fftype'] = 'Oalhh'
                elif neighbors_str == 'AlHSi':
                    atom['fftype'] = 'Oahs'  # Al-OH-Si for acidic edge
                elif neighbors_str.startswith('AlHMg'):
                    atom['fftype'] = 'Ohmg'
                elif neighbors_str == 'AlMgSi' or neighbors_str == 'AlMgoSi':
                    atom['fftype'] = 'Omg'
                elif neighbors_str == 'AlOmg':
                    atom['fftype'] = 'Odsub'
                elif neighbors_str == 'AltH':
                    atom['fftype'] = 'Oh'
                elif neighbors_str.startswith('AltMgh') or neighbors_str == 'AltMgoMgoMgo':
                    atom['fftype'] = 'Ops'
                elif neighbors_str == 'AltSi':
                    atom['fftype'] = 'Obs'
                # Calcium environments
                elif neighbors_str == 'CaCaCaCaCaCa' or neighbors_str == 'CaoCaoCaoCaoCaoCao':
                    atom['fftype'] = 'Ob'
                elif neighbors_str == 'CaCaCaH' or neighbors_str == 'CahCahCahH' or neighbors_str == 'CaoCaoCaoH':
                    atom['fftype'] = 'Oh'
                # Iron environments
                elif neighbors_str == 'Fe2Fe2Fe2Fe2Fe2Fe2' or neighbors_str == 'FeFeFeFeFeFe':
                    atom['fftype'] = 'Op'
                elif neighbors_str == 'FeFe' or neighbors_str == 'FeoFeo':
                    atom['fftype'] = 'Ob'
                elif neighbors_str == 'FeFeFe' or neighbors_str == 'FeoFeoFeo':
                    atom['fftype'] = 'Ob'
                elif neighbors_str == 'FeFeFet' or neighbors_str == 'FeoFeoFet':
                    atom['fftype'] = 'Ob'
                elif neighbors_str == 'FeFeFeFet' or neighbors_str == 'FeoFeoFeoFet':
                    atom['fftype'] = 'Obt'
                elif neighbors_str == 'FeFeFeFe' or neighbors_str == 'FeoFeoFeoFeo':
                    atom['fftype'] = 'Ob'
                elif neighbors_str == 'FeFeFeH' or neighbors_str == 'FeoFeoFeoH':
                    atom['fftype'] = 'Oh'
                elif neighbors_str == 'FeFeH' or neighbors_str == 'FeoFeoH':
                    atom['fftype'] = 'Oh'
                elif neighbors_str == 'FeFeSi' or neighbors_str == 'FeoFeoSi':
                    atom['fftype'] = 'Op'
                elif neighbors_str == 'FeH' or neighbors_str == 'FeoH':
                    atom['fftype'] = 'Oh'
                elif neighbors_str.startswith('FeHMg') or neighbors_str.startswith('FeoHMg'):
                    atom['fftype'] = 'Ohmg'
                elif neighbors_str == 'FeMgSi' or neighbors_str.startswith('FeMgoSi') or neighbors_str.startswith('FeoMgSi'):
                    atom['fftype'] = 'Omg'
                elif neighbors_str == 'FeSi' or neighbors_str == 'FeoSi' or neighbors_str == 'FetSi':
                    atom['fftype'] = 'Oalt'
                elif neighbors_str == 'FetFet' or neighbors_str == 'FetFetH' or neighbors_str == 'FetH':
                    atom['fftype'] = 'Oh'
                # Lithium/Magnesium environments
                elif neighbors_str == 'HLiMgMg' or neighbors_str == 'HLiMgoMgo' or neighbors_str == 'HLioMghMgh':
                    atom['fftype'] = 'Ohli'
                elif neighbors_str == 'HHMg' or neighbors_str == 'HHMgh' or neighbors_str.startswith('HHMgo'):
                    atom['fftype'] = 'Omhh'
                elif neighbors_str == 'HMg' or neighbors_str == 'HMgh':
                    atom['fftype'] = 'Ome'
                elif neighbors_str == 'HMgMg' or neighbors_str == 'HMghMgh':
                    atom['fftype'] = 'Ohmg'
                elif neighbors_str == 'HMgMgMg' or neighbors_str == 'HMghMghMgh':
                    atom['fftype'] = 'Oh'
                elif neighbors_str == 'HSi':
                    atom['fftype'] = 'Osih'
                elif neighbors_str.startswith('LiLiLiLi'):
                    atom['fftype'] = 'Oli'
                elif neighbors_str == 'LiMgMgSi' or neighbors_str == 'LioMgMgSi' or neighbors_str == 'LioMgoMgoSi':
                    atom['fftype'] = 'Oli'
                # More magnesium environments
                elif neighbors_str == 'MgMgMgMgMgMg' or neighbors_str == 'MghMghMghMghMghMgh' or neighbors_str == 'MgoMgoMgoMgoMgoMgo':
                    atom['fftype'] = 'Ob'
                elif neighbors_str == 'MgMgMgSi' or neighbors_str == 'MghMghMghSi' or neighbors_str == 'MgoMgoMgoSi':
                    atom['fftype'] = 'Op'
                elif neighbors_str == 'MgMgSi' or neighbors_str == 'MgoMgoSi':
                    atom['fftype'] = 'Odsub'
                elif neighbors_str == 'MgSi':
                    atom['fftype'] = 'Omg'
                # Silicon and Titanium environments
                elif neighbors_str == 'SiSi' or neighbors_str == 'SiSiSi':
                    atom['fftype'] = 'Ob'
                elif neighbors_str == 'TiTiTi' or neighbors_str == 'TioTioTio':
                    atom['fftype'] = 'Ob'
                # Special case for AlSi with nested condition
                elif neighbors_str == 'AlSi':
                    # Check if any neighbor is Alt type
                    has_alt_neighbor = False
                    for neigh_idx in atom.get('neigh', []):
                        if atoms[neigh_idx].get('fftype') == 'Alt':
                            has_alt_neighbor = True
                            break
                    
                    if has_alt_neighbor:
                        atom['fftype'] = 'Oalt'
                    else:
                        atom['fftype'] = 'Oalsi'  # Special zeolite case
                # Water molecule 
                elif neighbors_str == 'HH':
                    atom['fftype'] = 'Ow'
                    print(f"Water molecule detected (atom index: {atom.get('index', '?')})")
                # Over/under coordination cases
                elif coord_num > 2:
                    print(f"O atom overcoordinated (atom index: {atom.get('index', '?')})")
                    print(f"Neighbors: {neighbor_types}")
                    atom['fftype'] = 'O_ov'
                elif coord_num == 1 or neighbors_str == 'AlAl' or neighbors_str == 'AlMg':
                    if neighbors_str == 'Si':
                        atom['fftype'] = 'Osi'
                    elif neighbors_str.startswith('AlAl'):
                        atom['fftype'] = 'Oal'
                    else:
                        print(f"O atom undercoordinated (atom index: {atom.get('index', '?')})")
                        print(f"Neighbors: {neighbor_types}")
                        atom['fftype'] = 'O_un'
                # Basic silicon case
                elif neighbors_str == 'Si':
                    atom['fftype'] = 'Osi'
                elif neighbors_str == 'SiH':
                    atom['fftype'] = 'Osih'
                # If nothing matched, assign a basic O type
                else:
                    atom['fftype'] = 'Osi'
        
        # Update atom types to match their new fftype
        for atom in atoms:
            atom['type'] = atom['fftype']
    
    # Apply charges based on the MINFF forcefield after atom typing is complete
    atom_labels = ['Alo', 'Alt', 'Ale', 'Tio', 'Feo3', 'Fet3', 'Fee3', 'Feo2', 'Fet2', 'Fee2', 'Fs',
                  'Na', 'K', 'Cs', 'Mgo', 'Mgh', 'Mge', 'Cao', 'Cah', 'Ca', 'Sit', 
                  'Sio', 'Site', 'Lio', 'H', 'Cl']
    charges = [1.782, 1.782, 1.985, 2.48, 1.5, 1.5, 1.75, 1.184, 1.184, 1.32, -0.76,
               1.0, 1.0, 1.0, 1.562, 1.74, 1.635, 1.66, 1.52, 2.0, 1.884, 
               1.884, 2.413, 0.86, 0.4, -1.0]
    # By default, apply charges to all atoms (set resname=None)
    # To limit charge assignment to specific residues, provide a resname (e.g., 'MIN')
    atoms = charge_minff(atoms, Box, atom_labels, charges, resname=None)
    
    # Find unique types of atomtypes and their neighbors
    # This is equivalent to the MATLAB code for finding unique types after atom type assignment
    all_neighbors = []
    
    # Gather atom types and neighbor information
    for atom in atoms:
        if atom.get('neigh', []) and 'fftype' in atom:
            atom_type = atom['fftype']
            
            # Get all neighbor atom types (not just elements)
            neighbor_types = [atoms[neigh_idx].get('fftype', atoms[neigh_idx].get('element', '')) for neigh_idx in atom.get('neigh', [])]
            
            # Create a concatenated string of all neighbor atom types in sorted order
            all_neighbor_str = ''.join(sorted(neighbor_types))
            
            # For compatibility with existing code, still compute unique elements and counts
            # (used for consolidation logic)
            neighbor_elements = [atoms[neigh_idx]['element'] for neigh_idx in atom.get('neigh', [])]
            neighbor_counts = {}
            for n_type in neighbor_elements:
                if n_type in neighbor_counts:
                    neighbor_counts[n_type] += 1
                else:
                    neighbor_counts[n_type] = 1
                    
            # Sort neighbors by element name for consistent ordering (for consolidation)
            sorted_neighbors = sorted(neighbor_counts.items())
            neighbor_str_with_counts = ''.join([f"{n_type}{count}" for n_type, count in sorted_neighbors])
            
            # Get charge if available
            charge = atom.get('charge', 0)
            
            # Combine atom type, neighbors, and charge - store both versions of neighbor info
            all_neighbors.append([atom_type, neighbor_str_with_counts, all_neighbor_str, 1, None, charge])
    
    # Consolidate duplicate entries (same atom type with same neighbor pattern)
    i = 0
    while i < len(all_neighbors) - 1:
        # If current and next row have same atom type and neighbor pattern
        if (all_neighbors[i][0] == all_neighbors[i+1][0] and 
            all_neighbors[i][1] == all_neighbors[i+1][1]):
            # Increment count in the current row
            all_neighbors[i][3] += 1
            # Remove the duplicate row
            all_neighbors.pop(i+1)
        else:
            i += 1
    
    # Note: Removed suffix numbers for duplicate atom types since we removed the Label column
    
    # Convert charges to unique values per atom type
    # Pre-compute a dictionary of charges by atom type
    charges_by_type = {}
    for row in all_neighbors:
        atom_type = row[0]
        if atom_type not in charges_by_type:
            charges_by_type[atom_type] = []
        charges_by_type[atom_type].append(row[5])  # Updated index for charges

    # Then update each row with unique charges
    for i, row in enumerate(all_neighbors):
        atom_type = row[0]
        try:
            # Try direct set conversion
            unique_charges = list(set(charges_by_type[atom_type]))
        except TypeError:
            # Fall back to string conversion if needed
            unique_charges = list(set(str(c) if isinstance(c, (list, dict, set)) else c 
                                  for c in charges_by_type[atom_type]))
        all_neighbors[i][5] = unique_charges
    
    # Create a dictionary to consolidate truly unique atom types, neighbor patterns, and their counts
    unique_patterns = {}
    for row in all_neighbors:
        atom_type = row[0]
        count = row[3]
        neighbor_pattern = row[2]  # Use neighbor atom types for display
        charges = row[5]
        
        # Create a key using atom type and neighbor pattern
        key = (atom_type, neighbor_pattern)
        
        if key in unique_patterns:
            # If this pattern already exists, add to its count
            unique_patterns[key]['count'] += count
        else:
            # First time seeing this pattern
            unique_patterns[key] = {
                'count': count,
                'charges': charges
            }
    
    # Print a compact table of truly unique atom types and their neighbors
    print("\nUnique Atom Types and Their Coordination Environment")
    print("-" * 70)
    print(f"{'Type':<10} {'Count':<6} {'Neighbors':<25} {'Charge':>15}")
    print("-" * 70)
    
    # Sort by atom type for a more organized display
    for key in sorted(unique_patterns.keys()):
        atom_type, neighbor_pattern = key
        count = unique_patterns[key]['count']
        charges = unique_patterns[key]['charges']
        charge_str = ', '.join([f"{c:.6f}" if isinstance(c, float) else str(c) for c in charges])
        print(f"{atom_type:<10} {count:<6} {neighbor_pattern:<25} {charge_str:>15}")
    print("-" * 70)
    
    # Calculate total charge for statistics
    total_charge = sum(atom.get('charge', 0) for atom in atoms)
    
    # Generate and output structure statistics if log is enabled
    if log:
        # Use provided log_file path or generate default name if not provided
        log_path = log_file if log_file is not None else f"{ffname}_structure_stats.log"
        # Cell is already available from earlier in the function
        get_structure_stats(atoms, Box=Box, total_charge=total_charge, log_file=log_path, ffname=ffname)
        print(f"Structure statistics written to {log_path}")
    
    return atoms # , all_neighbors



def clayff(atoms, Box, ffname='clayff', rmaxlong=2.45, rmaxH=1.2, log=False, log_file=None):
    """Assign CLAYFF forcefield specific atom types to atoms based on their coordination environment.
    
    This function updates the 'fftype' field based on the atom's element and its bonding environment,
    using a two-pass approach to first determine coordination numbers and then assign types based on
    structural environment.
    
    Water molecules (residue 'SOL') and Ions (residue 'ION') are treated specially: their atom types
    are standardized (e.g., 'Ow', 'Hw', 'Na') and preserved, skipping the structural type assignment.
    
    For details, see the CLAYFF forcefield documentation at github.com/mholmboe/clayff.
    
    Args:
        atoms: A list of atom dictionaries, each atom is expected to have position coordinates
              and element/type information.
        Box: a 1x3, 1x6 or 1x9 list representing Cell dimensions (in Angstroms):
            - For orthogonal boxes, a 1x3 list [lx, ly, lz] where Box = Box_dim, and Cell would be [lx, ly, lz, 90, 90, 90]
            - For Cell parameters, a 1x6 list [a, b, c, alpha, beta, gamma] (Cell format)
            - For triclinic boxes, a 1x9 list [lx, ly, lz, 0, 0, xy, 0, xz, yz] (GROMACS Box_dim format)
        ffname: The forcefield name, default is 'clayff'.
        rmaxlong: Maximum bond distance for non-hydrogen bonds, default is 2.45 Å.
        rmaxH: Maximum bond distance for hydrogen bonds, default is 1.2 Å.
    
    Returns:
        The updated atoms list with 'fftype' fields assigned.

    Examples
    --------
    import atomipy as ap
    atoms, Box = ap.import_gro("clay_structure.gro")
    atoms = ap.clayff(atoms, Box)
    atoms = ap.clayff(atoms, [40, 40, 40], log=True)
    """
    # Set the atoms chemical element names
    atoms = element(atoms)  # Use correct function name 'element'

   # First assign formal charges to all atoms (especially for ions and water)
    # This sets appropriate charges based on atom types and residue names
    atoms = assign_formal_charges(atoms)

    # Set atom masses using the mass.py module
    atoms = set_atomic_masses(atoms)

    # Determine Box format and convert as needed
    if Box is None:
        raise ValueError(
            "clayff requires a simulation Box; received None. "
            "Make sure a mineral/box source is connected upstream."
        )
    if len(Box) == 9:
        # Triclinic Box in GROMACS format [lx, ly, lz, 0, 0, xy, 0, xz, yz]
        Box_dim = Box
        Cell = Box_dim2Cell(Box_dim)
    elif len(Box) == 6:
        # Cell parameters [a, b, c, alpha, beta, gamma]
        Cell = Box
        Box_dim = Cell2Box_dim(Cell)
    elif len(Box) == 3:
        # Simple orthogonal Box [lx, ly, lz]
        Box_dim = Box
        Cell = list(Box) + [90.0, 90.0, 90.0]
    else:
        raise ValueError("Box must be length 3, 6, or 9")
    
    # Run the entire process twice to ensure all atoms have proper typing
    # This is especially important for oxygen atoms which need to know
    # whether their metal neighbors are tetrahedral or octahedral
    for _ in range(2):  # Run the typing process twice
        # First, ensure all atoms have element types defined
        for atom in atoms:
            #if 'element' not in atom:
            # Try to extract element from atom type
            atom_type = atom.get('type', 'X')
            
            # Convert to lowercase for case-insensitive comparison
            atom_type_lower = atom_type.lower()
            
            # Map atom types to elements based on first 1-3 characters
            if atom.get('resname') == 'SOL':
                if atom_type_lower.startswith('o'):
                    atom['element'] = 'Ow'
                elif atom_type_lower.startswith('h'):
                    atom['element'] = 'Hw'
            elif atom.get('resname') == 'ION':
                if atom_type_lower.startswith('sod') or atom_type_lower.startswith('na'):
                    atom['element'] = 'Na'
                elif atom_type_lower.startswith('cla') or atom_type_lower.startswith('cl'):
                    atom['element'] = 'Cl'
                elif atom_type_lower.startswith('pot') or atom_type_lower.startswith('k'):
                    atom['element'] = 'K'
            elif atom_type_lower.startswith('si'):  
                atom['element'] = 'Si'
            elif atom_type_lower.startswith('sc'):  
                atom['element'] = 'Si'
            elif atom_type_lower.startswith('alt'): 
                atom['element'] = 'Alt'
            elif atom_type_lower.startswith('ale'): 
                atom['element'] = 'Ale'
            elif atom_type_lower.startswith('al'):  
                atom['element'] = 'Al'
            elif atom_type_lower.startswith('mg'):  
                atom['element'] = 'Mg'
            elif atom_type_lower.startswith('ca'):   
                atom['element'] = 'Ca'
            elif atom_type_lower.startswith('fe'):  
                atom['element'] = 'Fe'
            elif atom_type_lower.startswith('fet'):  
                atom['element'] = 'Fet'
            elif atom_type_lower.startswith('fe2'):  
                atom['element'] = 'Fe2'
            elif atom_type_lower.startswith('li'):  
                atom['element'] = 'Li'
            elif atom_type_lower.startswith('ow'):  
                atom['element'] = 'Ow'
            elif atom_type_lower.startswith('hw'):  
                atom['element'] = 'Hw'
            elif atom_type_lower.startswith('o'):   
                atom['element'] = 'O'
            elif atom_type_lower.startswith('h'):   
                atom['element'] = 'H'
            else:
                atom['element'] = atom_type
        
        # Initialize atom types and fftypes to match element type
        for atom in atoms:
            atom['type'] = atom['element']
            atom['fftype'] = atom['element']
        
        # Only calculate bonds in the first pass
        if _ == 0:
            # Get bonds and angles using bond_angle function (this also calculates coordination numbers).
            # same_molecule_only=False: mineral framework coordination is purely geometric and must
            # NOT depend on molid. Imported/uploaded structures (and replication) may carry per-atom or
            # per-residue molids, which would otherwise make the default same-molecule filter find ZERO
            # bonds, leaving every atom untyped (e.g. 'Al' instead of 'Alo' -> downstream KeyError).
            atoms, bond_index, angle_index = bond_angle(atoms, Box, rmaxH=rmaxH, rmaxM=rmaxlong,
                                                        same_molecule_only=False)

            # Drop water/ion neighbours from each FRAMEWORK atom's coordination before typing.
            # bond_angle above is geometric, so an interlayer cation or nearby water would
            # otherwise be counted as bonded to a surface oxygen and mistype it (e.g. Op -> O_ov).
            # This is molid-independent (robust to per-atom molids from a PDB round-trip) and
            # cheap: an O(N) classification + an O(bonds) neighbour filter. bond_angle itself
            # is unchanged, so no extra neighbour-search cost is added.
            _nonframework = {i for i, a in enumerate(atoms) if _is_solvent_or_ion(a)}
            if _nonframework:
                for i, atom in enumerate(atoms):
                    if i in _nonframework:
                        continue
                    _neigh = atom.get('neigh', [])
                    _kept = [j for j in _neigh if j not in _nonframework]
                    if len(_kept) != len(_neigh):
                        atom['neigh'] = _kept
                        atom['cn'] = len(_kept)
                        _bonds = atom.get('bonds', [])
                        if _bonds:
                            atom['bonds'] = [b for b in _bonds if b[0] not in _nonframework]

            # Store bond information and prepare for atom typing
            for i, atom in enumerate(atoms):
                # Skip water and ion residues if present
                if _is_solvent_or_ion(atom):
                    continue
                    
                # Get neighbors from the bonds
                neighbors = atom.get('neigh', [])
                if not neighbors:
                    continue
                    
                # Use the coordination number already calculated by bond_angle
                atom['coord_num'] = atom.get('cn', 0)
                
                # For Fe atoms, calculate average Fe-O bond distance to determine oxidation state
                if atom['element'] == 'Fe':
                    # Get bonds to this atom
                    bonds = atom.get('bonds', [])
                    if bonds:
                        # Extract distances and calculate average
                        bond_distances = [dist for _, dist in bonds]
                        avg_bond_distance = sum(bond_distances) / len(bond_distances)
                        atom['avg_bond_dist'] = avg_bond_distance
        
        # Assign atom types based on coordination and bond information
        for i, atom in enumerate(atoms):
            # Skip water and ion residues
            if _is_solvent_or_ion(atom):
                continue
                
            # Get neighbors from the bonds
            neighbors = atom.get('neigh', [])
            if not neighbors:
                continue
                
            # Get neighbor types for pattern matching
            neighbor_types = [atoms[neigh_idx]['element'] for neigh_idx in neighbors]
            neighbor_types.sort()
            neighbors_str = ''.join(neighbor_types)
            
            # Number of neighbors (coordination number)
            coord_num = atom.get('coord_num', 0)
            
            # Determine fftype based on element and coordination environment
            el = atom['element']
            
            # Lithium assignments
            if el == 'Li':
                if coord_num == 6:
                    atom['fftype'] = 'Lio'
                elif coord_num == 4:
                    atom['fftype'] = 'Lio'
                elif coord_num > 6:
                    atom['fftype'] = 'Lio_ov'  # Over-coordinated
                elif 4 < coord_num < 6:
                    atom['fftype'] = 'Lio_un'  # Under-coordinated
            
            # Silicon assignments
            elif el == 'Si':
                o_neighbors = neighbor_types.count('O')
                if o_neighbors == 4:
                    atom['fftype'] = 'Sit'
                elif coord_num > 4:
                    atom['fftype'] = 'Si_ov'  # Over-coordinated
                elif coord_num < 4:
                    atom['fftype'] = 'Si_un'  # Under-coordinated
            
            # Aluminum assignments
            elif el == 'Al':
                o_neighbors = neighbor_types.count('O')
                if o_neighbors == 6:
                    atom['fftype'] = 'Alo'     # Octahedral Al
                elif o_neighbors == 5:
                    atom['fftype'] = 'Ale'    # 5-coordinated Al
                elif o_neighbors == 4:
                    atom['fftype'] = 'Alt'    # Tetrahedral Al
                elif coord_num > 6:
                    atom['fftype'] = 'Al_ov'  # Over-coordinated
                elif coord_num < 4:
                    atom['fftype'] = 'Al_un'  # Under-coordinated
            
            # Magnesium assignments
            elif el == 'Mg':
                if coord_num == 6:

                    atom['fftype'] = 'Mgo'

                    # Check if there are more Mg than Si (e.g. in forsterite)
                    h_count = sum(1 for a in atoms if a.get('element') == 'H')
                    mg_count = sum(1 for a in atoms if a.get('element') == 'Mg')
                    si_count = sum(1 for a in atoms if a.get('element') == 'Si')
                    al_count = sum(1 for a in atoms if a.get('element') == 'Al')
                    fe_count = sum(1 for a in atoms if a.get('element') == 'Fe')
                    
                    if mg_count > si_count:
                        atom['fftype'] = 'Mgo'  # E.g. in forsterite
                        if mg_count < h_count:
                            atom['fftype'] = 'Mgh'  # Ex. Brucite
                    elif mg_count <= si_count:
                        atom['fftype'] = 'Mgh'  # E.g. in Talc, Hectorite
                        if al_count > mg_count or fe_count > mg_count:
                            atom['fftype'] = 'Mgo'  # Ex. Mica, Smectite

                elif coord_num > 6:
                    atom['fftype'] = 'Mg_ov'  # Over-coordinated
                elif coord_num < 6:
                    atom['fftype'] = 'Mg_un'  # Under-coordinated
                    
            # Calcium assignments
            elif el == 'Ca':
                o_neighbors = neighbor_types.count('O')
                f_neighbors = neighbor_types.count('F')
                
                if o_neighbors == 6:
                    atom['fftype'] = 'Cao'  # Octahedral Ca
                elif o_neighbors == 4:
                    atom['fftype'] = 'Cah'  # 4-coordinated Ca
                elif f_neighbors == 8:
                    # Likely in fluorite (CaF2) structure
                    print(f"Ca in CaF2 Fluorite? (atom index: {atom.get('index', '?')})")
                    atom['fftype'] = 'Cah'
                elif coord_num > 6:
                    print(f"Ca atom over coordinated (atom index: {atom.get('index', '?')})")
                    print(f"Neighbors: {neighbor_types}")
                    atom['fftype'] = 'Cao_ov'  # Over-coordinated
                elif coord_num < 6:
                    print(f"Ca atom under coordinated (atom index: {atom.get('index', '?')})")
                    print(f"Neighbors: {neighbor_types}")
                    atom['fftype'] = 'Cao_un'  # Under-coordinated
                else:
                    # Fall back for other cases
                    print(f"Ca with unusual coordination (atom index: {atom.get('index', '?')})")
                    print(f"Neighbors: {neighbor_types}")
            
            # Iron assignments with Fe3+/Fe2+ distinction based on bond distance
            elif atom.get('type', '').lower().startswith('fe'):
                o_neighbors = neighbor_types.count('O')
                avg_bond_dist = atom.get('avg_bond_dist', 0)
                
                if o_neighbors == 6:  # Octahedral Fe
                    # CLAYFF (Cygan 2004) uses a single octahedral Fe3+ type
                    # (Feo, q=+1.575), not a Fe2+/Fe3+ split. CLAYFF_EXT declares Feo.
                    atom['fftype'] = 'Feo'
                elif o_neighbors == 4:  # Tetrahedral Fe
                    if avg_bond_dist < 2.0:  # Fe3+ site (typical distance cutoff for tetrahedral)
                        atom['fftype'] = 'Fe3t'
                    else:  # Fe2+ site
                        atom['fftype'] = 'Fe2t'
                        print("Do you really have a tetrahedral Fe2+ site?")
                elif coord_num > 6:
                    atom['fftype'] = 'Fe_ov'  # Over-coordinated
                elif coord_num < 4:
                    atom['fftype'] = 'Fe_un'  # Under-coordinated
            
            # Hydrogen assignments
            elif el == 'H':
                if coord_num == 1:
                    atom['fftype'] = 'H'
                elif coord_num > 1:
                    atom['fftype'] = 'H_ov'  # Over-coordinated
            
            # Oxygen assignments - based on neighbor pattern
            elif el == 'O':
                # Begin with basic cases based on key neighbor patterns
                if neighbors_str == 'AlAlAl' or neighbors_str == 'AlAlAlAl':
                    atom['fftype'] = 'Ob'
                elif neighbors_str == 'AlAlAlAlt':
                    atom['fftype'] = 'Obt'
                elif neighbors_str == 'AlAlAlH':
                    atom['fftype'] = 'Oh'
                elif neighbors_str == 'AlAlAlt' or neighbors_str == 'AlAlt':
                    atom['fftype'] = 'Ops'
                elif neighbors_str == 'AlAlFe' or neighbors_str == 'AlAlFeo':
                    atom['fftype'] = 'Ob'
                elif neighbors_str == 'AlAlH':
                    atom['fftype'] = 'Oh'
                elif neighbors_str == 'AlAlSi':
                    atom['fftype'] = 'Op'
                elif neighbors_str == 'AlAlSiSi':
                    atom['fftype'] = 'Oz'
                elif neighbors_str == 'AlAleH' or neighbors_str == 'AleH':
                    atom['fftype'] = 'Oh'
                elif neighbors_str == 'AlAleSi' or neighbors_str == 'AleSi':
                    atom['fftype'] = 'Ob'
                elif neighbors_str == 'AlAltH':
                    atom['fftype'] = 'Ops'
                elif neighbors_str == 'AlFeFe' or neighbors_str == 'AlFeoFeo' or neighbors_str == 'AltFeFe' or neighbors_str == 'AltFeoFeo':
                    atom['fftype'] = 'Ops'
                elif neighbors_str == 'AlFeFet' or neighbors_str == 'AlFeoFet':
                    atom['fftype'] = 'Ops'
                elif neighbors_str == 'AlAltFe' or neighbors_str == 'AlAltFeo':
                    atom['fftype'] = 'Ops'
                elif neighbors_str == 'AlFeH' or neighbors_str == 'AlFeoH':
                    atom['fftype'] = 'Oh'
                elif neighbors_str == 'AlFeSi' or neighbors_str == 'AlFeoSi':
                    atom['fftype'] = 'Op'
                elif neighbors_str == 'AlFet' or neighbors_str == 'AlAlFet':
                    atom['fftype'] = 'Ops'
                elif neighbors_str == 'AlH':
                    atom['fftype'] = 'Oalh'  # Al-O-H or Al-O-Si
                elif neighbors_str == 'AlHH':
                    atom['fftype'] = 'Oalhh'
                elif neighbors_str == 'AlHSi':
                    atom['fftype'] = 'Oahs'  # Al-OH-Si for acidic edge
                elif neighbors_str.startswith('AlHMg'):
                    atom['fftype'] = 'Ohmg'
                elif neighbors_str == 'AlMgSi' or neighbors_str == 'AlMgoSi':
                    atom['fftype'] = 'Omg'
                elif neighbors_str == 'AlOmg':
                    atom['fftype'] = 'Odsub'
                elif neighbors_str == 'AltH':
                    atom['fftype'] = 'Oh'
                elif neighbors_str.startswith('AltMgh') or neighbors_str == 'AltMgoMgoMgo':
                    atom['fftype'] = 'Ops'
                elif neighbors_str == 'AltSi':
                    atom['fftype'] = 'Obs'
                # Calcium environments
                elif neighbors_str == 'CaCaCaCaCaCa' or neighbors_str == 'CaoCaoCaoCaoCaoCao':
                    atom['fftype'] = 'Ob'
                elif neighbors_str == 'CaCaCaH' or neighbors_str == 'CahCahCahH' or neighbors_str == 'CaoCaoCaoH':
                    atom['fftype'] = 'Oh'
                # Iron environments
                elif neighbors_str == 'Fe2Fe2Fe2Fe2Fe2Fe2' or neighbors_str == 'FeFeFeFeFeFe':
                    atom['fftype'] = 'Op'
                elif neighbors_str == 'FeFe' or neighbors_str == 'FeoFeo':
                    atom['fftype'] = 'Ob'
                elif neighbors_str == 'FeFeFe' or neighbors_str == 'FeoFeoFeo':
                    atom['fftype'] = 'Ob'
                elif neighbors_str == 'FeFeFet' or neighbors_str == 'FeoFeoFet':
                    atom['fftype'] = 'Ob'
                elif neighbors_str == 'FeFeFeFet' or neighbors_str == 'FeoFeoFeoFet':
                    atom['fftype'] = 'Obt'
                elif neighbors_str == 'FeFeFeFe' or neighbors_str == 'FeoFeoFeoFeo':
                    atom['fftype'] = 'Ob'
                elif neighbors_str == 'FeFeFeH' or neighbors_str == 'FeoFeoFeoH':
                    atom['fftype'] = 'Oh'
                elif neighbors_str == 'FeFeH' or neighbors_str == 'FeoFeoH':
                    atom['fftype'] = 'Oh'
                elif neighbors_str == 'FeFeSi' or neighbors_str == 'FeoFeoSi':
                    atom['fftype'] = 'Op'
                elif neighbors_str == 'FeH' or neighbors_str == 'FeoH':
                    atom['fftype'] = 'Oh'
                elif neighbors_str.startswith('FeHMg') or neighbors_str.startswith('FeoHMg'):
                    atom['fftype'] = 'Ohmg'
                elif neighbors_str == 'FeMgSi' or neighbors_str.startswith('FeMgoSi') or neighbors_str.startswith('FeoMgSi'):
                    atom['fftype'] = 'Omg'
                elif neighbors_str == 'FeSi' or neighbors_str == 'FeoSi' or neighbors_str == 'FetSi':
                    atom['fftype'] = 'Oalt'
                elif neighbors_str == 'FetFet' or neighbors_str == 'FetFetH' or neighbors_str == 'FetH':
                    atom['fftype'] = 'Oh'
                # Lithium/Magnesium environments
                elif neighbors_str == 'HLiMgMg' or neighbors_str == 'HLiMgoMgo' or neighbors_str == 'HLioMghMgh':
                    atom['fftype'] = 'Ohli'
                elif neighbors_str == 'HHMg' or neighbors_str == 'HHMgh' or neighbors_str.startswith('HHMgo'):
                    atom['fftype'] = 'Omhh'
                elif neighbors_str == 'HMg' or neighbors_str == 'HMgh':
                    atom['fftype'] = 'Ome'
                elif neighbors_str == 'HMgMg' or neighbors_str == 'HMghMgh':
                    atom['fftype'] = 'Ohmg'
                elif neighbors_str == 'HMgMgMg' or neighbors_str == 'HMghMghMgh':
                    atom['fftype'] = 'Oh'
                elif neighbors_str == 'HSi':
                    atom['fftype'] = 'Osih'
                elif neighbors_str.startswith('LiLiLiLi'):
                    atom['fftype'] = 'Oli'
                elif neighbors_str == 'LiMgMgSi' or neighbors_str == 'LioMgMgSi' or neighbors_str == 'LioMgoMgoSi':
                    atom['fftype'] = 'Oli'
                # More magnesium environments
                elif neighbors_str == 'MgMgMgMgMgMg' or neighbors_str == 'MghMghMghMghMghMgh' or neighbors_str == 'MgoMgoMgoMgoMgoMgo':
                    atom['fftype'] = 'Ob'
                elif neighbors_str == 'MgMgMgSi' or neighbors_str == 'MghMghMghSi' or neighbors_str == 'MgoMgoMgoSi':
                    atom['fftype'] = 'Op'
                elif neighbors_str == 'MgMgSi' or neighbors_str == 'MgoMgoSi':
                    atom['fftype'] = 'Odsub'
                elif neighbors_str == 'MgSi':
                    atom['fftype'] = 'Omg'
                # Silicon and Titanium environments
                elif neighbors_str == 'SiSi' or neighbors_str == 'SiSiSi':
                    atom['fftype'] = 'Ob'
                elif neighbors_str == 'TiTiTi' or neighbors_str == 'TioTioTio':
                    atom['fftype'] = 'Ob'
                # Special case for AlSi with nested condition
                elif neighbors_str == 'AlSi':
                    # Check if any neighbor is Alt type
                    has_alt_neighbor = False
                    for neigh_idx in atom.get('neigh', []):
                        if atoms[neigh_idx].get('fftype') == 'Alt':
                            has_alt_neighbor = True
                            break
                    
                    if has_alt_neighbor:
                        atom['fftype'] = 'Oalt'
                    else:
                        atom['fftype'] = 'Oalsi'  # Special zeolite case
                # Water molecule 
                elif neighbors_str == 'HH':
                    atom['fftype'] = 'Ow'
                    print(f"Water molecule detected (atom index: {atom.get('index', '?')})")
                # Over/under coordination cases
                elif coord_num > 2:
                    print(f"O atom overcoordinated (atom index: {atom.get('index', '?')})")
                    print(f"Neighbors: {neighbor_types}")
                    atom['fftype'] = 'O_ov'
                elif coord_num == 1 or neighbors_str == 'AlAl' or neighbors_str == 'AlMg':
                    if neighbors_str == 'Si':
                        atom['fftype'] = 'Osi'
                    elif neighbors_str.startswith('AlAl'):
                        atom['fftype'] = 'Oal'
                    else:
                        print(f"O atom undercoordinated (atom index: {atom.get('index', '?')})")
                        print(f"Neighbors: {neighbor_types}")
                        atom['fftype'] = 'O_un'
                # Basic silicon case
                elif neighbors_str == 'Si':
                    atom['fftype'] = 'Osi'
                elif neighbors_str == 'SiH':
                    atom['fftype'] = 'Osih'
                # If nothing matched, assign a basic O type
                else:
                    atom['fftype'] = 'Osi'
        
        # Update atom types to match their new fftype
        for atom in atoms:
            atom['type'] = atom['fftype']
    
    # Apply charges based on the clayff forcefield after atom typing is complete
    # CLAYFF octahedral iron is the single Fe3+ type Feo (q=+1.575); Fe3t kept for the
    # rare tetrahedral case. (Cygan 2004 — no Fe2+/Fe3+ octahedral split.)
    atom_labels = ['Alo', 'Alt', 'Ale', 'Feo', 'Fe3t', 'Mgo', 'Mgh', 'Cao', 'Cah', 'Sit', 'Lio', 'H', 'Na', 'K', 'Cs', 'Ca', 'Cl']
    charges = [1.575, 1.575, 1.8125, 1.575, 1.575, 1.36, 1.05, 1.36, 1.05, 2.1, 0.86, 0.425, 1.0, 1.0, 1.0, 2.0, -1.0]
    # By default, apply charges to all atoms (set resname=None)
    # To limit charge assignment to specific residues, provide a resname (e.g., 'MIN')
    atoms = charge_clayff(atoms, Box, atom_labels, charges, resname=None)
    
    # Find unique types of atomtypes and their neighbors
    # This is equivalent to the MATLAB code for finding unique types after atom type assignment
    all_neighbors = []
    
    # Gather atom types and neighbor information
    for atom in atoms:
        if atom.get('neigh', []) and 'fftype' in atom:
            atom_type = atom['fftype']
            
            # Get all neighbor atom types (not just elements)
            neighbor_types = [atoms[neigh_idx].get('fftype', atoms[neigh_idx].get('element', '')) for neigh_idx in atom.get('neigh', [])]
            
            # Create a concatenated string of all neighbor atom types in sorted order
            all_neighbor_str = ''.join(sorted(neighbor_types))
            
            # For compatibility with existing code, still compute unique elements and counts
            # (used for consolidation logic)
            neighbor_elements = [atoms[neigh_idx]['element'] for neigh_idx in atom.get('neigh', [])]
            neighbor_counts = {}
            for n_type in neighbor_elements:
                if n_type in neighbor_counts:
                    neighbor_counts[n_type] += 1
                else:
                    neighbor_counts[n_type] = 1
                    
            # Sort neighbors by element name for consistent ordering (for consolidation)
            sorted_neighbors = sorted(neighbor_counts.items())
            neighbor_str_with_counts = ''.join([f"{n_type}{count}" for n_type, count in sorted_neighbors])
            
            # Get charge if available
            charge = atom.get('charge', 0)
            
            # Combine atom type, neighbors, and charge - store both versions of neighbor info
            all_neighbors.append([atom_type, neighbor_str_with_counts, all_neighbor_str, 1, None, charge])
    
    # Consolidate duplicate entries (same atom type with same neighbor pattern)
    i = 0
    while i < len(all_neighbors) - 1:
        # If current and next row have same atom type and neighbor pattern
        if (all_neighbors[i][0] == all_neighbors[i+1][0] and 
            all_neighbors[i][1] == all_neighbors[i+1][1]):
            # Increment count in the current row
            all_neighbors[i][3] += 1
            # Remove the duplicate row
            all_neighbors.pop(i+1)
        else:
            i += 1
    
    # Note: Removed suffix numbers for duplicate atom types since we removed the Label column
    
    # Convert charges to unique values per atom type
    # Pre-compute a dictionary of charges by atom type
    charges_by_type = {}
    for row in all_neighbors:
        atom_type = row[0]
        if atom_type not in charges_by_type:
            charges_by_type[atom_type] = []
        charges_by_type[atom_type].append(row[5])  # Updated index for charges

    # Then update each row with unique charges
    for i, row in enumerate(all_neighbors):
        atom_type = row[0]
        try:
            # Try direct set conversion
            unique_charges = list(set(charges_by_type[atom_type]))
        except TypeError:
            # Fall back to string conversion if needed
            unique_charges = list(set(str(c) if isinstance(c, (list, dict, set)) else c 
                                  for c in charges_by_type[atom_type]))
        all_neighbors[i][5] = unique_charges
    
    # Create a dictionary to consolidate truly unique atom types, neighbor patterns, and their counts
    unique_patterns = {}
    for row in all_neighbors:
        atom_type = row[0]
        count = row[3]
        neighbor_pattern = row[2]  # Use neighbor atom types for display
        charges = row[5]
        
        # Create a key using atom type and neighbor pattern
        key = (atom_type, neighbor_pattern)
        
        if key in unique_patterns:
            # If this pattern already exists, add to its count
            unique_patterns[key]['count'] += count
        else:
            # First time seeing this pattern
            unique_patterns[key] = {
                'count': count,
                'charges': charges
            }
    
    # Print a compact table of truly unique atom types and their neighbors
    print("\nUnique Atom Types and Their Coordination Environment")
    print("-" * 70)
    print(f"{'Type':<10} {'Count':<6} {'Neighbors':<25} {'Charge':>15}")
    print("-" * 70)
    
    # Sort by atom type for a more organized display
    for key in sorted(unique_patterns.keys()):
        atom_type, neighbor_pattern = key
        count = unique_patterns[key]['count']
        charges = unique_patterns[key]['charges']
        charge_str = ', '.join([f"{c:.6f}" if isinstance(c, float) else str(c) for c in charges])
        print(f"{atom_type:<10} {count:<6} {neighbor_pattern:<25} {charge_str:>15}")
    print("-" * 70)
    
    # Calculate total charge for statistics
    total_charge = sum(atom.get('charge', 0) for atom in atoms)
    
    # Generate and output structure statistics if log is enabled
    if log:
        # Use provided log_file path or generate default name if not provided
        log_path = log_file if log_file is not None else f"{ffname}_structure_stats.log"
        # Cell is already available from earlier in the function
        get_structure_stats(atoms, Box=Box, total_charge=total_charge, log_file=log_path, ffname=ffname)
        print(f"Structure statistics written to {log_path}")
    
    return atoms #, all_neighbors

def write_n2t(atoms, Box=None, n2t_file=None, verbose=True):
    """Generate a GROMACS-compatible atomname2type (.n2t) file.

    The layout mirrors the MATLAB implementation in ``n2t_atom.m`` and
    aggregates unique coordination environments per (element, atom type,
    neighbor sequence). Neighbor distances are reported in nanometers and
    are obtained from bond data when available, or from minimum-image
    distances when ``Box`` is supplied. Environments that only differ by
    small numerical noise in their neighbor distances are merged.

    Args:
        atoms: List of atom dictionaries.
        Box: Optional simulation Cell in any standard atomipy format. Accepts
             orthogonal ``[lx, ly, lz]`` vectors, 1×6 Cell parameters, or the
             1×9 ``Box_dim`` layout used by GROMACS. When provided, distances
             fall back to periodic minimum-image values.
        n2t_file: Optional output path (defaults to ``atomname2type.n2t``).
        verbose: Emit warnings about inferred data.
    """
    import math
    import os
    from statistics import median

    ANGSTROM_TO_NM = 0.1

    path_like = (str, bytes)
    if hasattr(os, "PathLike"):
        path_like = path_like + (os.PathLike,)

    def _looks_like_box(value):
        if isinstance(value, np.ndarray):
            value = value.tolist()
        return isinstance(value, (list, tuple, np.ndarray)) and len(value) in {3, 6, 9}

    # Backwards compatibility with legacy signature: write_n2t(atoms, n2t_file, Box)
    if isinstance(Box, path_like):
        if n2t_file is None:
            n2t_file, Box = Box, None
        elif _looks_like_box(n2t_file):
            Box, n2t_file = n2t_file, Box

    normalized_box = None
    if Box is not None:
        if isinstance(Box, np.ndarray):
            Box = Box.tolist()
        if not isinstance(Box, (list, tuple)) or len(Box) not in {3, 6, 9}:
            raise ValueError("Box must be a sequence of length 3, 6, or 9")
        if len(Box) == 6:
            normalized_box = Cell2Box_dim(Box)
            if isinstance(normalized_box, np.ndarray):
                normalized_box = normalized_box.tolist()
        elif len(Box) == 9:
            normalized_box = list(Box)
        else:  # len == 3
            normalized_box = list(Box)
    Box = normalized_box

    if not atoms:
        raise ValueError("No atoms supplied to write_n2t")

    n2t_path = n2t_file if n2t_file is not None else "atomname2type.n2t"

    # Ensure element symbols and masses are available
    atoms = element(atoms)
    atoms = set_atomic_masses(atoms)

    # If no neighbor or bond data exist, attempt to infer it from coordinates
    has_neighbors = any(atom.get('neigh') for atom in atoms)
    has_bonds = any(atom.get('bonds') for atom in atoms)

    if not has_neighbors or not has_bonds:
        if Box is not None:
            try:
                atoms, _, _ = bond_angle(atoms, Box=Box, rmaxH=1.2, rmaxM=2.45)
            except Exception as exc:
                if verbose:
                    print(f"Warning: failed to infer bonds with provided Box ({exc})")
        elif all(key in atoms[0] for key in ['x', 'y', 'z']):
            try:
                min_x = min(atom.get('x', 0.0) for atom in atoms) - 5.0
                max_x = max(atom.get('x', 0.0) for atom in atoms) + 5.0
                min_y = min(atom.get('y', 0.0) for atom in atoms) - 5.0
                max_y = max(atom.get('y', 0.0) for atom in atoms) + 5.0
                min_z = min(atom.get('z', 0.0) for atom in atoms) - 5.0
                max_z = max(atom.get('z', 0.0) for atom in atoms) + 5.0
                default_Box = [max_x - min_x, max_y - min_y, max_z - min_z]
                if verbose:
                    print(f"Calculating bonds with default Box {default_Box}")
                atoms, _, _ = bond_angle(atoms, Box=default_Box, rmaxH=1.2, rmaxM=2.45)
            except Exception as exc:
                if verbose:
                    print(f"Warning: failed to infer bonds automatically ({exc})")

    def sorted_unique_indices(indices):
        seen = set()
        ordered = []
        for idx in indices:
            if idx not in seen:
                seen.add(idx)
                ordered.append(idx)
        return ordered

    env_map = {}

    # Instead of a full distance matrix, we'll compute PBC distances on the fly if needed
    H, Hinv = None, None
    if Box is not None:
        try:
            from .cell_utils import normalize_box
            _, Cell = normalize_box(Box)
            if Cell is None:
                raise ValueError("Failed to normalize Box")
            a, b, c = Cell[0], Cell[1], Cell[2]
            alpha, beta, gamma = (Cell[3], Cell[4], Cell[5]) if len(Cell) > 3 else (90, 90, 90)
            ar, br, gr = np.radians([alpha, beta, gamma])
            ax = a
            bx = b * np.cos(gr)
            by = b * np.sin(gr)
            cx = c * np.cos(br)
            cy = c * (np.cos(ar) - np.cos(br) * np.cos(gr)) / np.sin(gr)
            cz = np.sqrt(max(0, c**2 - cx**2 - cy**2))
            H = np.array([[ax, bx, cx], [0, by, cy], [0, 0, cz]])
            Hinv = np.linalg.inv(H)
        except Exception as exc:
            if verbose:
                print(f"Warning: failed to prepare PBC parameters ({exc})")

    for idx, atom in enumerate(atoms):
        center_type = str(atom.get('type', '') or 'X')
        center_el = str(atom.get('element', center_type[:2] or 'X') or 'X')

        neighbor_indices = atom.get('neigh', []) or []
        bond_pairs = atom.get('bonds', []) or []

        # Build a map of bond distances
        bond_distance_map = {}
        for neighbor_idx, distance in bond_pairs:
            if isinstance(neighbor_idx, int):
                bond_distance_map[neighbor_idx] = float(distance)

        # Fall back to bonds when neigh list is empty
        if not neighbor_indices and bond_distance_map:
            neighbor_indices = list(bond_distance_map.keys())

        neighbor_indices = [n for n in neighbor_indices if isinstance(n, int) and 0 <= n < len(atoms) and n != idx]
        neighbor_indices = sorted_unique_indices(neighbor_indices)

        neighbor_data = []
        for neigh_idx in neighbor_indices:
            neigh_atom = atoms[neigh_idx]
            neigh_el = str(neigh_atom.get('element', neigh_atom.get('type', 'X')) or 'X')

            distance = bond_distance_map.get(neigh_idx)
            if distance is None:
                x1, y1, z1 = atom.get('x'), atom.get('y'), atom.get('z')
                x2, y2, z2 = neigh_atom.get('x'), neigh_atom.get('y'), neigh_atom.get('z')
                if None not in (x1, y1, z1, x2, y2, z2):
                    dx = float(x2) - float(x1)
                    dy = float(y2) - float(y1)
                    dz = float(z2) - float(z1)
                    if H is not None and Hinv is not None:
                        # Fractional coordinates for PBC
                        f_vec = Hinv @ np.array([dx, dy, dz])
                        f_vec -= np.round(f_vec)
                        # Back to real space
                        r_vec = H @ f_vec
                        distance = np.linalg.norm(r_vec)
                    else:
                        distance = math.sqrt(dx*dx + dy*dy + dz*dz)

            neighbor_data.append((neigh_el, distance))

        # Sort neighbors by element then by formatted distance for stable ordering
        def neighbor_sort_key(item):
            el, dist = item
            dist_key = f"{dist:12.6f}" if dist is not None and math.isfinite(dist) else ""
            return (el, dist_key)

        neighbor_data.sort(key=neighbor_sort_key)

        cn = len(neighbor_data)
        neighbor_elements = tuple(el for el, _ in neighbor_data)

        env_key = (center_el, center_type, cn, neighbor_elements)
        if env_key not in env_map:
            env_map[env_key] = {
                'center_el': center_el,
                'center_type': center_type,
                'cn': cn,
                'neighbor_elements': neighbor_elements,
                'distance_lists': [list() for _ in range(cn)],
                'charges': [],
                'masses': [],
                'count': 0
            }

        env_entry = env_map[env_key]
        env_entry['count'] += 1

        for pos, (_, dist) in enumerate(neighbor_data):
            if dist is not None and math.isfinite(dist):
                env_entry['distance_lists'][pos].append(float(dist))

        charge = atom.get('charge')
        if charge is not None:
            try:
                env_entry['charges'].append(float(charge))
            except (TypeError, ValueError):
                pass

        mass = atom.get('mass')
        if mass is not None:
            try:
                env_entry['masses'].append(float(mass))
            except (TypeError, ValueError):
                pass

    records = []
    for env_key, env_entry in env_map.items():
        cn = env_entry['cn']
        neighbor_elements = env_entry['neighbor_elements']

        if env_entry['charges']:
            charge_value = sum(env_entry['charges']) / len(env_entry['charges'])
        else:
            charge_value = 0.0

        mass_value = 0.0
        masses = env_entry['masses']
        if masses:
            base_mass = masses[0]
            if any(abs(m - base_mass) > 1e-6 for m in masses[1:]):
                mass_value = median(masses)
                if verbose:
                    print(
                        f"Warning: inconsistent masses for atom type {env_entry['center_type']}"
                        f" ({env_entry['center_el']}); using median {mass_value:.5f}"
                    )
            else:
                mass_value = base_mass

        distances_nm = []
        for dist_list in env_entry['distance_lists']:
            if dist_list:
                distances_nm.append(sum(dist_list) / len(dist_list) * ANGSTROM_TO_NM)
            else:
                distances_nm.append(0.0)

        records.append({
            'element': env_entry['center_el'],
            'atom_type': env_entry['center_type'],
            'charge': charge_value,
            'mass': mass_value,
            'cn': cn,
            'neighbors': list(zip(neighbor_elements, distances_nm)),
            'count': env_entry['count']
        })

    def merge_similar_records(items, tol=0.005):
        merged = []
        for record in items:
            matched = False
            for existing in merged:
                if (record['element'] == existing['element'] and
                        record['atom_type'] == existing['atom_type'] and
                        record['cn'] == existing['cn']):
                    same_neighbors = all(
                        n1[0] == n2[0]
                        for n1, n2 in zip(record['neighbors'], existing['neighbors'])
                    )
                    if same_neighbors:
                        within_tol = all(
                            abs(n1[1] - n2[1]) <= tol
                            for n1, n2 in zip(record['neighbors'], existing['neighbors'])
                        )
                        if within_tol:
                            total = existing['count'] + record['count']
                            w_old = existing['count'] / total
                            w_new = record['count'] / total
                            existing['charge'] = existing['charge'] * w_old + record['charge'] * w_new
                            existing['mass'] = existing['mass'] * w_old + record['mass'] * w_new
                            existing['neighbors'] = [
                                (el_old, dist_old * w_old + dist_new * w_new)
                                for (el_old, dist_old), (_, dist_new) in zip(existing['neighbors'], record['neighbors'])
                            ]
                            existing['count'] = total
                            matched = True
                            break
            if not matched:
                merged.append({
                    'element': record['element'],
                    'atom_type': record['atom_type'],
                    'charge': record['charge'],
                    'mass': record['mass'],
                    'cn': record['cn'],
                    'neighbors': list(record['neighbors']),
                    'count': record['count']
                })
        return merged

    records = merge_similar_records(records)

    records.sort(key=lambda r: (-r['cn'], r['element'], r['atom_type']))

    header_lines = [
        "; atomname2type.n2t generated by n2t_atom()",
        "; Columns: Element  AtomType  Charge  Mass[u]  CN  [NeighborElement  Distance(nm)]",
        "; Element from element_atom(atom) -> element(i).type ; AtomType from original atom(i).type",
        "; Mass from atom(i).mass for the members of each (Element,AtomType,CN,NeighborSeq) environment."
    ]

    lines = []
    for record in records:
        line = f"{record['element']:<2} {record['atom_type']:<16} {record['charge']: .6f} {record['mass']:10.5f} {record['cn']:2d}"
        for neigh_el, dist_nm in record['neighbors']:
            line += f"  {neigh_el:<2} {dist_nm:6.3f}"
        lines.append(line.rstrip())

    contents = "\n".join(header_lines + lines)

    with open(n2t_path, 'w') as handle:
        handle.write(contents + "\n")

    if verbose:
        print(f"N2T file written to {n2t_path}")

    return n2t_path
