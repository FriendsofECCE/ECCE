"""
atomipy.topology.builder — bridge from raw atoms/box lists to a fully typed Topology.

This module provides `build_topology_from_atoms`, which applies MINFF/CLAYFF implicit
parameter rules (such as O-H bond distances and dynamic metal-oxygen angles) to build
a complete generic `Topology` graph ready for export via `write_topology.py`.
"""

from typing import Any, Dict, List, Optional
import numpy as np

from . import units as U
from .. import import_topology as itop
from .model import Topology, Bond, Angle
from . import functional_forms as ff
# Single source of truth for the MINFF/CLAYFF angle model lives in write_top
# (the lowest-level module that needs it), so importing it here does not create
# a topology<->write_top load-order cycle.
from ..write_top import angle_parameters, cluster_angles

def harmonize_topology_angles(top: Topology, threshold: float = 30.0) -> None:
    """
    Cluster angles of the same triplet type into bimodal or unimodal groups.
    Updates the theta0 values in the Topology in-place to the cluster means.
    """
    from collections import defaultdict
    
    # We group angles by the triplet of atom types: type_i - type_j - type_k
    # Note: j is the central atom in Angle(i, j, k)
    type_map = {a.id: a.type or 'X' for a in top.atoms}
    
    angle_by_type = defaultdict(list)
    angle_refs = defaultdict(list)
    
    # Only harmonize metal angles
    for angle in top.angles:
        t_i = type_map[angle.i]
        t_j = type_map[angle.j]
        t_k = type_map[angle.k]
        
        # MINFF metal angles don't involve Hydrogen
        if t_i.startswith('H') or t_j.startswith('H') or t_k.startswith('H'):
            continue
            
        t_ik = tuple(sorted([t_i, t_k]))
        triplet = f"{t_ik[0]}-{t_j}-{t_ik[1]}"
        
        # Canonical unit for ANGLE is degrees
        theta0_deg = angle.params.get('theta0', 0.0)
        angle_by_type[triplet].append(theta0_deg)
        angle_refs[triplet].append(angle)
        
    for triplet, values in angle_by_type.items():
        if len(values) < 4:
            # Not enough stats, use global mean
            mean_deg = sum(values) / len(values) if values else 0.0
            for a in angle_refs[triplet]:
                a.params['theta0'] = mean_deg
            continue
            
        sorted_vals = sorted(values)
        spread = sorted_vals[-1] - sorted_vals[0]
        
        if spread <= threshold:
            mean_deg = sum(values) / len(values)
            for a in angle_refs[triplet]:
                a.params['theta0'] = mean_deg
            continue
            
        # Bimodal split check
        max_gap = 0
        split_idx = 0
        for idx in range(len(sorted_vals) - 1):
            gap = sorted_vals[idx + 1] - sorted_vals[idx]
            if gap > max_gap:
                max_gap = gap
                split_idx = idx + 1
                
        if max_gap > threshold / 2:
            cluster1 = sorted_vals[:split_idx]
            cluster2 = sorted_vals[split_idx:]
            mean1_deg = sum(cluster1) / len(cluster1)
            mean2_deg = sum(cluster2) / len(cluster2)
            
            # Assign each angle reference to the closest mean
            for a in angle_refs[triplet]:
                val_deg = a.params['theta0']
                if abs(val_deg - mean1_deg) < abs(val_deg - mean2_deg):
                    a.params['theta0'] = mean1_deg
                else:
                    a.params['theta0'] = mean2_deg
        else:
            mean_deg = sum(values) / len(values)
            for a in angle_refs[triplet]:
                a.params['theta0'] = mean_deg


def build_topology_from_atoms(
    atoms: List[Dict[str, Any]], 
    box: Optional[List[float]],
    KANGLE: float = 500.0,
    max_angle: Optional[float] = None,
    harmonize_angles: bool = False,
    bimodal_threshold: float = 30.0
) -> Topology:
    """
    Build a fully parameterized Topology from a standard atomipy atoms list (e.g. from minff() or clayff()).
    This replicates the implicit bond/angle rules formerly hardcoded in `write_top.py`.
    
    Args:
        atoms: The atomipy list of dictionaries (must have 'bonds' and 'angles' populated by bond_angle()).
        box: The atomipy box dimensions.
        KANGLE: The generic force constant for M-O-M / O-M-O angles.
        max_angle: Optional cutoff to ignore angles larger than this.
        harmonize_angles: If True, uses k-means clustering to replace bimodal angles with their cluster mean.
        bimodal_threshold: Minimum angular spread in degrees to split a cluster.
        
    Returns:
        A Topology object.
    """
    # Defensive check: if the first atom doesn't have 'bonds', we probably received raw atoms.
    # Run bond_angle() dynamically to ensure topology can be built. Use `any`
    # (not just atoms[0]) so a mixed system — where the mineral went through
    # minff()/clayff() (has bonds) but water/ions were appended afterwards
    # (no bonds) — still gets its solvent/ion connectivity computed.
    if atoms and any('bonds' not in a for a in atoms):
        from atomipy.bond_angle import bond_angle
        # Fallback box dimension if none is provided
        box_vec = list(box) if box is not None and len(box) >= 3 else [50.0, 50.0, 50.0]
        try:
            atoms, _, _ = bond_angle(atoms, box_vec, rmaxH=1.2, rmaxM=2.45, same_element_bonds=False, same_molecule_only=True)
        except Exception:
            pass

    # 1. Base initialization from positions/charges
    top = itop.from_atoms_box(atoms, box)
    
    # 2. Extract and filter Bonds
    # MINFF/CLAYFF specifically only parameterizes certain bonds in topologies.
    # Other interactions are handled via nonbonded terms.
    BONDED_PAIRS = {
        frozenset(['Oh',   'H']),
        frozenset(['Ob',   'H']),
        frozenset(['Ohmg', 'H']),
        frozenset(['Oalh', 'H']),
        frozenset(['Oalhh','H']),
        frozenset(['Osih', 'H']),
        frozenset(['Alo',  'Oalh']),
        frozenset(['Alo',  'Oalhh']),
        frozenset(['Sit',  'Osih']),
    }
    
    for i, a in enumerate(atoms):
        for bond in a.get('bonds', []):
            j, dist = bond[0], bond[1]
            if i < j: # Only add each bond once
                t_i = a.get('type', a.get('fftype', 'X'))
                t_j = atoms[j].get('type', atoms[j].get('fftype', 'X'))
                pair = frozenset([t_i, t_j])
                
                if pair in BONDED_PAIRS:
                    # Determine b0 based on the pair
                    if 'H' in pair:
                        b0 = 0.09572
                    elif 'Sit' in pair:
                        b0 = 0.160
                    else: # Alo edge bonds
                        b0 = 0.195
                        
                    # k = 441050 kJ/(mol nm^2) in GROMACS
                    params_gmx = {"b0": b0, "k": 441050.0}
                    top.bonds.append(Bond(
                        i=i, j=j, 
                        form="harmonic",
                        params=ff.from_backend("bond", "harmonic", params_gmx, "gromacs")
                    ))
                    
    # 3. Extract and filter Angles
    for i, a in enumerate(atoms):
        # 'angles' contains: ((neigh1_idx, neigh2_idx), angle_val)
        for angle_data in a.get('angles', []):
            (j, k), angle_val = angle_data
            
            # Skip duplicate angles (only process when the central atom is the current atom `i`, 
            # and j <= k to avoid double counting A-B-C and C-B-A).
            # Note: j == k is allowed because periodic images of the same atom can form angles!
            if j <= k:
                if max_angle is not None and angle_val > max_angle:
                    continue
                    
                type_j = atoms[j].get('type', 'X')
                type_i = a.get('type', 'X')
                type_k = atoms[k].get('type', 'X')
                
                theta0, k_theta, _ = angle_parameters(type_j, type_i, type_k, angle_val, KANGLE)
                
                params_gmx = {"theta0": theta0, "k": k_theta}
                top.angles.append(Angle(
                    i=j, j=i, k=k,
                    form="harmonic",
                    params=ff.from_backend("angle", "harmonic", params_gmx, "gromacs")
                ))

    # Apply harmonized clustering if requested
    if harmonize_angles:
        harmonize_topology_angles(top, threshold=bimodal_threshold)

    return top
