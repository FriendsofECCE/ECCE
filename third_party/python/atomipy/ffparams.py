"""
Forcefield parameters loading and conversion utilities.

This module provides functions to load forcefield parameters from JSON files
and convert them for use with different molecular dynamics packages.

The ffparams directory is bundled with the atomipy package at atomipy/ffparams/.

Examples
--------
import atomipy as ap

# Load MINFF parameters for LAMMPS (uses bundled data)
ff = ap.load_forcefield(
    'GMINFF/gminff_opc3_hfe_lm_k500.json',
    blocks=['GMINFF_k500']
)

# Use with LAMMPS writer
ap.write_lmp(atoms, Box, 'system.data', forcefield=ff)
"""

import json
import os
from pathlib import Path

# Unit conversion constants
NM_TO_ANGSTROM = 10.0
KJ_TO_KCAL = 1.0 / 4.184

# Default type aliases: atomipy type name -> JSON forcefield type name
# These map common atomipy-assigned types to matching forcefield JSON keys
DEFAULT_TYPE_ALIASES = {
    # OPC3 water (default water model)
    'Ow': 'OW_opc3',
    'Hw': 'HW_opc3',
    # Common monovalent ions
    'Na': 'Na+',
    'K': 'K+',
    'Cl': 'Cl-',  # all ions are signed (ASCII +/-) in the JSON/itp; charge also in the charge column
    'Li': 'Li+',
    'Rb': 'Rb+',
    'Cs': 'Cs+',
    'F': 'F-',
    'Br': 'Br-',
    'I': 'I-',
    # Common divalent ions
    'Ca': 'Ca2+',
    'Mg': 'Mg2+',
    'Ba': 'Ba2+',
    'Sr': 'Sr2+',
    'Zn': 'Zn2+',
    'Fe': 'Fe2+',
    'Cu': 'Cu2+',
}

# Package data directory
_FFPARAMS_DIR = Path(__file__).parent / 'ffparams'


def get_ffparams_dir():
    """
    Get the path to the bundled ffparams directory.
    
    Returns:
        Path: Absolute path to the ffparams directory bundled with atomipy.
    
    Examples
    --------
    ffdir = get_ffparams_dir()
    print(ffdir)  # /path/to/atomipy/ffparams
    """
    return _FFPARAMS_DIR


def load_json(json_path):
    """
    Load a forcefield JSON file.
    
    Args:
        json_path: Path to the JSON file. Can be:
                   - Absolute path
                   - Relative to bundled ffparams (e.g., 'GMINFF/gminff_opc3_hfe_lm_k500.json')
                   - Relative to current directory
    
    Returns:
        dict: The parsed JSON data containing forcefield parameters.
    
    Raises:
        FileNotFoundError: If the JSON file does not exist.
    
    Examples
    --------
    # Using bundled data (recommended)
    data = load_json('GMINFF/gminff_opc3_hfe_lm_k500.json')
    
    # Using absolute path
    data = load_json('/path/to/custom.json')
    """
    path = Path(json_path)
    
    # If absolute path, use directly
    if path.is_absolute():
        if not path.exists():
            raise FileNotFoundError(f"Forcefield JSON file not found: {json_path}")
        with open(path, 'r') as f:
            return json.load(f)
    
    # Try bundled ffparams directory first
    bundled_path = _FFPARAMS_DIR / json_path
    if bundled_path.exists():
        with open(bundled_path, 'r') as f:
            return json.load(f)
    
    # Fall back to relative path from current directory
    if path.exists():
        with open(path, 'r') as f:
            return json.load(f)
    
    # Try legacy path (ffparams/ at project root)
    legacy_path = Path(__file__).parent.parent / 'ffparams' / json_path
    if legacy_path.exists():
        with open(legacy_path, 'r') as f:
            return json.load(f)
    
    raise FileNotFoundError(
        f"Forcefield JSON file not found: {json_path}\n"
        f"Searched in:\n"
        f"  - {bundled_path}\n"
        f"  - {path.absolute()}\n"
        f"  - {legacy_path}"
    )


def get_atomtypes(json_data, blocks=None):
    """
    Extract atomtypes from JSON data, merging specified blocks with common types.
    
    Args:
        json_data: Parsed JSON data from load_json().
        blocks: List of block names to include (e.g., ['GMINFF_k500', 'OPC3_HFE_LM']).
                If None, includes all available blocks.
    
    Returns:
        dict: Merged atomtypes dictionary with atom type names as keys.
    
    Examples
    --------
    data = load_json('GMINFF/gminff_opc3_hfe_lm_k500.json')
    atomtypes = get_atomtypes(data, blocks=['GMINFF_k500'])
    print(atomtypes['Sit'])  # {'mass': 28.085, 'sigma': 0.093467, 'epsilon': 0.458196, ...}
    """
    atomtypes = {}
    
    # First add common atomtypes (unconditional entries like water models)
    if 'common_atomtypes' in json_data:
        atomtypes.update(json_data['common_atomtypes'])
    
    # Then add atomtypes from specified blocks
    if 'nonbonded_blocks' in json_data:
        nb_blocks = json_data['nonbonded_blocks']
        
        if blocks is None:
            # Include all blocks
            blocks = list(nb_blocks.keys())
        
        for block_name in blocks:
            if block_name in nb_blocks:
                block_data = nb_blocks[block_name]
                if 'atomtypes' in block_data:
                    atomtypes.update(block_data['atomtypes'])
            else:
                available = list(nb_blocks.keys())
                raise ValueError(
                    f"Block '{block_name}' not found. Available blocks: {available}"
                )
    
    return atomtypes


def to_lammps_units(atomtypes):
    """
    Convert atomtype parameters from GROMACS to LAMMPS real units.
    
    Conversions:
    - sigma: nm → Å (multiply by 10)
    - epsilon: kJ/mol → kcal/mol (divide by 4.184)
    
    Args:
        atomtypes: Dictionary of atomtypes with sigma/epsilon in GROMACS units.
    
    Returns:
        dict: New dictionary with values converted to LAMMPS real units.
    
    Examples
    --------
    # GROMACS: sigma=0.093467 nm, epsilon=0.458196 kJ/mol
    # LAMMPS:  sigma=0.93467 Å,   epsilon=0.1095 kcal/mol
    lammps_params = to_lammps_units(atomtypes)
    """
    converted = {}
    
    for atype, params in atomtypes.items():
        new_params = params.copy()
        
        # Convert sigma from nm to Angstrom
        if 'sigma' in new_params:
            new_params['sigma'] = new_params['sigma'] * NM_TO_ANGSTROM
        
        # Convert epsilon from kJ/mol to kcal/mol
        if 'epsilon' in new_params:
            new_params['epsilon'] = new_params['epsilon'] * KJ_TO_KCAL
        
        converted[atype] = new_params
    
    return converted


def load_forcefield(json_path, blocks=None, units='lammps'):
    """
    Load forcefield parameters from JSON with automatic unit conversion.
    
    This is the main convenience function for loading forcefield parameters.
    
    Args:
        json_path: Path to the JSON file.
        blocks: List of block names to include. If None, all blocks are included.
        units: Target units - 'lammps' (real units: Å, kcal/mol) or 'gromacs' (nm, kJ/mol).
    
    Returns:
        dict: Forcefield parameters suitable for use with write_lmp() or other functions.
    
    Examples
    --------
    import atomipy as ap
    
    # Load MINFF with OPC3 water for LAMMPS (uses bundled data)
    ff = ap.load_forcefield(
        'GMINFF/gminff_opc3_hfe_lm_k500.json',
        blocks=['GMINFF_k500', 'OPC3_HFE_LM']
    )
    
    # Write LAMMPS data file with Pair Coeffs
    ap.write_lmp(atoms, Box, 'system.data', forcefield=ff)
    """
    # Load the JSON data
    json_data = load_json(json_path)
    
    # Extract atomtypes from specified blocks
    atomtypes = get_atomtypes(json_data, blocks=blocks)
    
    # Convert units if needed
    if units == 'lammps':
        atomtypes = to_lammps_units(atomtypes)
    elif units != 'gromacs':
        raise ValueError(f"Unknown units '{units}'. Use 'lammps' or 'gromacs'.")
    
    return atomtypes


def list_blocks(json_path):
    """
    List available blocks in a forcefield JSON file.
    
    Args:
        json_path: Path to the JSON file.
    
    Returns:
        list: Names of available blocks.
    
    Examples
    --------
    blocks = list_blocks('GMINFF/gminff_opc3_hfe_lm_k500.json')
    print(blocks)  # ['GMINFF_k500', 'OPC3_HFE_LM']
    """
    json_data = load_json(json_path)
    
    if 'nonbonded_blocks' in json_data:
        return list(json_data['nonbonded_blocks'].keys())
    
    return []
