"""
atomipy: The atom Toolbox in Python

This package provides tools for working with molecular structures, particularly
focused on mineral systems with periodic boundary conditions. It supports both
orthogonal and triclinic simulation cells, and provides efficient distance
calculations, bond/angle detection, structure manipulation, and X-ray diffraction
simulation functions.

Key features:
- Import/export of common molecular file formats (PDB, GRO, XYZ, CIF/mmCIF)
- Topology file generation for molecular dynamics simulations (GROMACS, NAMD, LAMMPS)
- GROMACS n2t file generation for atom name to type conversions (gmx x2top)
  with periodic minimum-image distances and environment deduplication
- Distance calculations with efficient algorithms for periodic systems
- Bond/angle/dihedral/1-4 pair detection
- Coordinate transformations between Cartesian and fractional representations
- Support for both orthogonal and triclinic simulation cells
- Structure manipulation (replication, translation)
- Force field implementations for mineral systems (CLAYFF, MINFF)
- High-performance X-ray diffraction pattern simulation with vectorized calculations
- Professional XRD plotting with Miller indices and multiplicities
- Automatic ionic scattering factor selection and MATLAB compatibility
"""

# ===== File I/O functions =====
# Import/export functions
from . import import_conf
import_pdb = import_conf.pdb
import_gro = import_conf.gro
import_xyz = import_conf.xyz
import_cif = import_conf.cif
import_mmcif = import_conf.cif  # alias — cif() handles both CIF and mmCIF
import_cjson = import_conf.cjson
import_pqr = import_conf.pqr
import_poscar = import_conf.poscar
import_traj = import_conf.import_traj
import_auto = import_conf.auto

# Optional .xtc/.trr support via libxdrfile (used by import_traj; see docs/libxdrfile.md).
from . import xdrfile
have_xdrfile = xdrfile.have_xdrfile

from . import write_conf
write_pdb = write_conf.pdb
write_gro = write_conf.gro
write_xyz = write_conf.xyz
write_cif = write_conf.cif
write_cjson = write_conf.cjson
write_pqr = write_conf.pqr

from . import molecule_library
list_molecules = molecule_library.list_molecules
load_molecule = molecule_library.load_molecule
molecule_categories = molecule_library.molecule_categories

from . import crystal_library
list_crystals = crystal_library.list_crystals
load_crystal = crystal_library.load_crystal
crystal_categories = crystal_library.crystal_categories
write_poscar = write_conf.poscar
write_sdf = write_conf.sdf
write_traj = write_conf.write_traj
write_auto = write_conf.auto

# Topology file generation
# NOTE: `write_top` stays the *module* (atomipy.write_top). The function that
# writes a complete GROMACS system .top is exposed as `write_gmx_top` to avoid
# shadowing the module name.
from .write_top import itp as write_itp, psf as write_psf, lmp as write_lmp, top as write_gmx_top
# New robust Topology Hub and Writers
try:
    from . import topology
    from . import write_topology
except ImportError:
    pass
# Itp file import helper
from .import_top import import_itp as import_itp_topology
# GAFF / ACPYPE topology import
try:
    from .import_top import import_gaff_top, import_gro_coords
except ImportError:
    pass
# N-way topology merger (GAFF + MINFF/CLAYFF)
try:
    from .merge_top import merge_top, merge_top_files, write_merged_top
except ImportError:
    pass

# ===== Atom property functions =====
from .element import element

from .radius import radius

try:
    from .mass import mass, set_atomic_masses, com
except ImportError:
    pass

# ===== Structure analysis functions =====
from .distances import dist_matrix, get_neighbor_list, cell_list_dist_matrix
from . import config
from . import analysis
unwrap_coordinates = analysis.unwrap_coordinates
calculate_rdf = analysis.calculate_rdf
coordination_number = analysis.coordination_number
closest_atom = analysis.closest_atom
min_distances = analysis.min_distances
density_profile = analysis.density_profile
rdf_frames = analysis.rdf_frames
density_frames = analysis.density_frames
msd = analysis.msd
displacement_distribution = analysis.displacement_distribution
find_hbonds = analysis.find_hbonds
hbonds_frames = analysis.hbonds_frames
vacf = analysis.vacf
from .bond_angle import bond_angle, bond_angle_dihedral

# ===== Cell and coordinate transformation functions =====
from . import cell_utils
normalize_box = cell_utils.normalize_box
Box_dim2Cell = cell_utils.Box_dim2Cell
Cell2Box_dim = cell_utils.Cell2Box_dim

# Coordinate transformation module
from . import transform
cartesian_to_fractional = transform.cartesian_to_fractional
fractional_to_cartesian = transform.fractional_to_cartesian
wrap_coordinates = transform.wrap_coordinates
wrap = transform.wrap
triclinic_to_orthogonal = transform.triclinic_to_orthogonal
orthogonal_to_triclinic = transform.orthogonal_to_triclinic
get_orthogonal_box = transform.get_orthogonal_box
get_cell_vectors = transform.get_cell_vectors
direct_cartesian_to_fractional = transform.direct_cartesian_to_fractional
direct_fractional_to_cartesian = transform.direct_fractional_to_cartesian

from . import replicate
replicate_system = replicate.replicate_system

from . import move
translate = move.translate
rotate = move.rotate
place = move.place
center = move.center
bend = move.bend

from .build import update

from .transform import scale

# ===== Build functions =====
from . import build
substitute = build.substitute
molecule = build.molecule
merge = build.merge
slice = build.slice
fit_box = build.fit_box
remove = build.remove
delete_sites = build.delete_sites
fuse_atoms = build.fuse_atoms
ionize = build.ionize
insert = build.insert
add_H_atom = build.add_H_atom
adjust_H_atom = build.adjust_H_atom
adjust_Hw_atom = build.adjust_Hw_atom
is_centrosymmetric_along_z = build.is_centrosymmetric_along_z
reorder = build.reorder
condense = build.condense
create_grid = build.create_grid
join_and_reorder = build.join_and_reorder
split_slabs_by_molid = build.split_slabs_by_molid

from . import node_cache
save_node_state = node_cache.save_node_state
load_node_state = node_cache.load_node_state

# ===== Resname functions =====
from .resname import assign_resname

# ===== Composition / system analysis =====
try:
    from .composition import composition, get_mol_sequence, get_mol_sequence_typed
except ImportError:
    pass

# ===== Solvent functions =====
from .solvent import find_H2O, solvate, spc2tip4p, tip3p2tip4p

# ===== Force field functions =====
try:
    from .forcefield import minff, clayff, write_n2t, get_structure_stats
except ImportError:
    pass

# ===== Forcefield parameters loading =====
try:
    from .ffparams import load_forcefield, list_blocks as list_ff_blocks, get_ffparams_dir
except ImportError:
    pass

# ===== Charge functions =====
try:
    from .charge import charge_minff, charge_clayff, balance_charges, assign_formal_charges, get_formal_charge, get_half_formal_charge
except ImportError:
    pass

# ===== Bond valence functions =====
try:
    from .bond_valence import compute_bvs, global_instability_index, load_bv_params, load_shannon_radii, bond_valence, summarize_bvs
except ImportError:
    pass
try:
    from .bond_valence import analyze_bvs, conf2bvs, add_hydrogens_bvs
except ImportError:
    pass

# ===== Oxidation-state guessing (rules + charge balance) =====
try:
    from .oxidation import guess_oxidation_states, PAULING_EN
except ImportError:
    pass

# ===== Frozen dummy mineral / Dummy FF (inorganics not covered by MINFF/CLAYFF) =====
try:
    from .dummy_mineral import (assign_dummy_mineral_params, write_dummy_mineral_itp,
                                write_dummy_system_top, pauling_effective_charge,
                                uff_lj, UFF_VDW,
                                MINFF_LJ_SITES, MINFF_FRAMEWORK_ELEMENTS)
except ImportError:
    pass
try:
    from .radius import get_radius, bond_distance
except ImportError:
    pass

# ===== Diffraction functions =====
try:
    from .diffraction import xrd, occupancy_atom, atomic_scattering_factors, calculate_multiplicity, bragg_law
except ImportError:
    pass

# ===== Miller-plane geometry =====
from .miller import miller_planes, d_spacing, cut_miller, cut_planes, hkil_to_hkl

# ===== Lattice / unit-cell builder =====
from .lattice import build_cell, make_lattice, lattice_types

# ===== Geometric region cuts (nanoparticle / nanowire) =====
from .shapes import cut_sphere, cut_cylinder

# ===== Surface / slab builder =====
from .surface import make_slab

# ===== OpenMM Interface =====
try:
    from .openmm_interface import load_minff_into_openmm
except ImportError:
    pass

# Version information
__version__ = "0.97"

# Expose key functions at the package level
__all__ = [
    'import_pdb', 'import_gro', 'import_xyz', 'import_cif', 'import_mmcif', 'import_cjson', 'import_pqr', 'import_poscar', 'import_traj', 'import_auto',
    'write_pdb', 'write_gro', 'write_xyz', 'write_cif', 'write_cjson', 'write_pqr', 'write_poscar', 'write_sdf', 'write_traj', 'write_auto',
    'list_molecules', 'load_molecule', 'molecule_categories',
    'list_crystals', 'load_crystal', 'crystal_categories',
    'write_itp', 'write_gmx_top', 'write_psf', 'write_lmp', 'import_itp_topology',
    'topology', 'write_topology',
    'import_gaff_top', 'import_gro_coords',
    'merge_top', 'merge_top_files', 'write_merged_top',
    'element', 'radius', 'mass', 'set_atomic_masses', 'com',
    'dist_matrix', 'cell_list_dist_matrix', 'config', 'bond_angle', 'bond_angle_dihedral', 'find_H2O',
    'normalize_box','Box_dim2Cell', 'Cell2Box_dim',
    'cartesian_to_fractional', 'fractional_to_cartesian', 'wrap', 'wrap_coordinates',
    'triclinic_to_orthogonal', 'orthogonal_to_triclinic', 'get_orthogonal_box', 'get_cell_vectors',
    'direct_cartesian_to_fractional', 'direct_fractional_to_cartesian',
    'replicate_system', 'translate', 'rotate', 'place', 'center', 'update', 'scale', 'bend',
    'substitute', 'molecule', 'merge', 'slice', 'fit_box', 'remove', 'delete_sites', 'fuse_atoms', 'solvate', 'ionize', 'insert',
    'add_H_atom', 'adjust_H_atom', 'adjust_Hw_atom', 'reorder', 'condense', 'create_grid', 'join_and_reorder', 'split_slabs_by_molid',
    'save_node_state', 'load_node_state',
    'is_centrosymmetric_along_z',
    'assign_resname', 'spc2tip4p', 'tip3p2tip4p',
    'composition', 'get_mol_sequence', 'get_mol_sequence_typed',
    'minff', 'clayff', 'write_n2t', 'get_structure_stats',
    'load_forcefield', 'list_ff_blocks', 'get_ffparams_dir',
    'charge_minff', 'charge_clayff', 'balance_charges', 'assign_formal_charges', 'get_formal_charge', 'get_half_formal_charge',
    'load_minff_into_openmm',
    'compute_bvs', 'global_instability_index', 'load_bv_params', 'load_shannon_radii', 'bond_valence', 'summarize_bvs',
    'analyze_bvs', 'conf2bvs', 'add_hydrogens_bvs',
    'guess_oxidation_states', 'PAULING_EN',
    'assign_dummy_mineral_params', 'write_dummy_mineral_itp', 'write_dummy_system_top', 'pauling_effective_charge', 'uff_lj', 'UFF_VDW', 'MINFF_LJ_SITES', 'MINFF_FRAMEWORK_ELEMENTS',
    'get_radius', 'bond_distance',
    'unwrap_coordinates', 'calculate_rdf', 'coordination_number', 'closest_atom', 'min_distances',
    'density_profile', 'rdf_frames', 'density_frames', 'msd', 'displacement_distribution',
    'find_hbonds', 'hbonds_frames', 'vacf',
    'xrd', 'occupancy_atom', 'atomic_scattering_factors', 'calculate_multiplicity', 'bragg_law',
    'miller_planes', 'd_spacing', 'cut_miller', 'cut_planes', 'hkil_to_hkl',
    'build_cell', 'make_lattice', 'lattice_types',
    'cut_sphere', 'cut_cylinder',
    'make_slab',
]
