# Forcefield Parameters (ffparams)

## Structure and Formats

This directory provides force field data in two distinct formats:

1.  **GROMACS Format (`min.ff/`)**: The `min.ff` directory contains the original force field files (`.itp`, `.atp`) in GROMACS format. These are the source files used by GROMACS directly.
2.  **Generic JSON Format**: The `.json` files in the root of this directory (`GMINFF/` and `TMINFF/`) are generic, structured exports of the force field data. These are designed to be software-agnostic and are used by `atomipy` to generate input files for other simulation engines, such as **LAMMPS** (via `ap.write_lmp`).

The JSON files include atomtype definitions, non-bonded parameters (sigma, epsilon), and bonded parameters (bondtypes, angletypes) for the [**MINFF**](https://github.com/mholmboe/minff) forcefield, TMINFF, ion parameters, and water models.

## Usage with atomipy

To fetch parameters, you must specify the blocks you want to load (e.g., a mineral block, an ion set, and a water model).

```python
import atomipy as ap

# Load GMINFF parameters (auto-converts to LAMMPS real units)
# Example: GMINFF k500 + OPC3 water + OPC3 HFE ions
ff = ap.load_forcefield(
    'GMINFF/gminff_all.json', 
    blocks=['GMINFF_k500', 'OPC3', 'OPC3_HFE_LM']
)

# Load TMINFF parameters
# Example: Kaolinite k500 + OPC3 water + OPC3 HFE ions
ff_tminff = ap.load_forcefield(
    'TMINFF/tminff_k500_all.json', 
    blocks=['Kaolinite_k500', 'OPC3', 'OPC3_HFE_LM']
)

# Write LAMMPS data file with Pair Coeffs
ap.write_lmp(atoms, Box, 'system.data', forcefield=ff)

# List available blocks in a JSON file
blocks = ap.list_ff_blocks('GMINFF/gminff_all.json')
print(blocks)
```

## Available Parameter Files

- `GMINFF/gminff_all.json`: Contains all General MINFF blocks and common water/ion parameters.
- `TMINFF/tminff_k*.json`: Contains Tailored MINFF blocks for specific minerals (k0, k250, k500, k1500) and common water/ion parameters.

## Available Blocks

### General Minerals (GMINFF)
Used in `GMINFF/gminff_all.json`.
- `GMINFF_k0`, `GMINFF_k250`, `GMINFF_k500`, `GMINFF_k1500`
- `CLAYFF_2004`
- `CLAYFF_EXT`

### Tailored Minerals (TMINFF)
Used in `TMINFF/tminff_k*.json`.
- **Phyllosilicates**: `Kaolinite_k*`, `Pyrophyllite_k*`, `Talc_k*`, `Montmorillonite_k*`, `Muscovite_k*`, `Dickite_k*`, `Nacrite_k*`, `Hectorite-F_k*`, `Hectorite-H_k*`
- **Oxides/Hydroxides**: `Gibbsite_k*`, `Boehmite_k*`, `Diaspore_k*`, `Brucite_k*`, `Periclase_k*`, `Corundum_k*`, `Hematite_k*`, `Goethite_k*`, `Lepidocrocite_k*`, `Wustite_k*`, `Magnetite_k*`, `Anatase_k*`, `Rutile_k*`, `Li2O_k*`, `CaO_k*`, `Portlandite_k*`, `Akdalaite_k*`
- **Silica**: `Quartz_k*`, `Coesite_k*`, `Cristobalite_k*`, `Imogolite_k*`
- **Other**: `Forsterite_k*`, `CaF2_k*`

*(Replace `k*` with `k0`, `k250`, `k500`, or `k1500` depending on the file loaded)*

### Water Models
- `SPC`, `SPCE`
- `TIP3P`, `TIP3P_FB`
- `TIP4P`, `TIP4PEW`, `TIP4P2005`, `TIP4P_FB`
- `OPC`, `OPC3`
- `TIP5P`

### Ion Sets
Multiple parameter sets are available for ions, often tailored for specific water models.
- **Cheatham et al. (JC)**: `SPC_JC`, `SPCE_JC`, `TIP3P_JC`, `TIP4P_JC`
- **Åqvist**: `SPC_AQVIST`
- **Babu & Lim**: `TIP3P_BL`
- **Li & Merz (LM)**:
    - **HFE (Hydration Free Energy)**: `*_HFE_LM` (e.g., `OPC3_HFE_LM`, `SPCE_HFE_LM`, `TIP3PFB_HFE_LM`)
    - **IOD (Ion-Oxygen Distance)**: `*_IOD_LM`
    - **CM (Compromise)**: `*_CM_LM` (divalent ions)

## Bonded Parameters

Bonded parameters are sparse in MINFF as it is primarily a non-bonded force field.

**Bonds (`func=1`):**
- `Oh-H`, `Ob-H`, `Ohmg-H`, `Oalh-H`, `Oalhh-H`, `Osih-H`, `Alo-Oalh`, `Alo-Oalhh`, `Sit-Osih`
- Parameters: $k_b = 441050 \text{ kJ/mol/nm}^2, b_0 = 0.09572 \text{ nm}$ (for O-H bonds)

**Angles (`func=1`):**
- **Note**: For M-O-H angles, MINFF v1.0 uses the angle parameters from *Pouvreau, Greathouse, Cygan and Kalinichev, J. Phys. Chem. C 2019, 123, 11628−11638*.

## Units

**JSON files (GROMACS units):**
- sigma: nm
- epsilon: kJ/mol

**After loading with `units='lammps'` (default in atomipy):**
- sigma: Å (×10)
- epsilon: kcal/mol (÷4.184)

## Type Aliases

Atomipy automatically maps common atom type names to JSON forcefield keys:
- `Ow` → `OW_opc3`, `Hw` → `HW_opc3`
- `Na` → `Na+`, `K` → `K+`, `Cl` → `Cl−`

## Generating JSON Files (gmx2json.py)

The source `.itp` files are in `min.ff/`. Use `gmx2json.py` to generate JSON exports.

```bash
# Export ALL blocks to a single JSON
python gmx2json.py -nb min.ff/ffnonbonded_gminff.itp -b min.ff/ffbonded.itp -o gminff_all.json
```
