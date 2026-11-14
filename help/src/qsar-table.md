# QSAR: descriptor table

**Tools > QSAR Table...** makes one row per calculation of a folder made by
[Batch Add from SMILES](batch-from-smiles.md): the measured values, descriptors
of the molecule and the results of the calculation. The table is shown in
ECCE and exported as CSV, for ECCE's own PCA and PLS or for other programs.

## Making the table

1. Select the project folder (or some of its calculations) in the Organizer.
2. Choose **Tools > QSAR Table...** and tick the descriptor groups.
3. The table opens. Click a column header to sort. Rows with a problem are
   shown in red and the `problems` column says what: a SMILES that cannot be
   read, a calculation without a stored SMILES, an optimisation that did not
   converge, an abnormal termination.
4. **Export CSV...** writes the table.

## Columns

Hover over any column header for its meaning, unit, group and reference.
**Key...** opens a resizable descriptor key for every column of the current
table. Select text to copy it, or use **Copy All** to copy the complete key.
Sorting the table does not change the key.

Measured/user columns retain their CSV header and any unit in trailing
parentheses, for example `bp (degC)`. Their text says **measured, from <file>**.
New Batch Add imports retain the input filename; pasted tables are labelled
`pasted text`. For older calculations without provenance the key says
`unknown source (not recorded)`. A column can list several source files.

The following key is generated from `scripts/ecce_qsar_chem.py`; the descriptor
test checks the complete block against that source. Dimensionless counts
are numbers of atoms, bonds, rings or violations as described.

<!-- BEGIN QSAR DESCRIPTOR KEY -->
| Column | Meaning, unit, group and reference |
|---|---|
| `name` | Calculation or molecule name; unit: none (identity) |
| `state` | Stored calculation run state; unit: none (QM) |
| `opt_converged` | Geometry optimisation convergence from calculation metadata progress (opt stage) and state; '-' when unavailable; unit: none (QM) |
| `weight` | Molecular weight including hydrogens (sum of standard atomic weights); unit: g/mol (counts) |
| `a_count` | Number of atoms including hydrogens; unit: dimensionless (counts) |
| `a_heavy` | Number of heavy (non-hydrogen) atoms; unit: dimensionless (counts) |
| `a_nH` | Number of hydrogen atoms; unit: dimensionless (counts) |
| `a_nC` | Number of carbon atoms; unit: dimensionless (counts) |
| `a_nN` | Number of nitrogen atoms; unit: dimensionless (counts) |
| `a_nO` | Number of oxygen atoms; unit: dimensionless (counts) |
| `a_nF` | Number of fluorine atoms; unit: dimensionless (counts) |
| `a_nCl` | Number of chlorine atoms; unit: dimensionless (counts) |
| `a_nBr` | Number of bromine atoms; unit: dimensionless (counts) |
| `a_nI` | Number of iodine atoms; unit: dimensionless (counts) |
| `a_nS` | Number of sulfur atoms; unit: dimensionless (counts) |
| `a_nP` | Number of phosphorus atoms; unit: dimensionless (counts) |
| `a_nB` | Number of boron atoms; unit: dimensionless (counts) |
| `a_nSi` | Number of silicon atoms; unit: dimensionless (counts) |
| `a_aro` | Number of aromatic atoms; unit: dimensionless (counts) |
| `b_count` | Number of bonds including bonds to hydrogens; unit: dimensionless (counts) |
| `b_heavy` | Number of bonds between heavy atoms; unit: dimensionless (counts) |
| `b_ar` | Number of aromatic bonds; unit: dimensionless (counts) |
| `b_single` | Number of non-aromatic single bonds between heavy atoms; unit: dimensionless (counts) |
| `b_double` | Number of non-aromatic double bonds between heavy atoms; unit: dimensionless (counts) |
| `b_triple` | Number of triple bonds between heavy atoms; unit: dimensionless (counts) |
| `b_rotN` | Number of rotatable bonds: single, acyclic, between non-terminal heavy atoms, excluding bonds at a triple bond and N-C bonds of N-H amides; unit: dimensionless (counts) |
| `b_rotR` | Rotatable bond count divided by the number of heavy-atom bonds; unit: dimensionless (counts) |
| `rings` | Number of rings in the perceived smallest ring basis; unit: dimensionless (counts) |
| `aro_rings` | Number of perceived rings whose atoms are all aromatic; unit: dimensionless (counts) |
| `chiral` | Number of atoms with four topologically different substituents; unit: dimensionless (counts) |
| `FCharge` | Total formal charge of the molecule; unit: e (counts) |
| `lip_don` | Hydrogen-bond donor count: N or O atoms bearing hydrogen; unit: dimensionless (counts) |
| `lip_acc` | Hydrogen-bond acceptor proxy: count of N and O atoms; unit: dimensionless (counts) |
| `lip_violation` | Number of Lipinski rule violations: weight > 500, SlogP > 5, donors > 5, acceptors > 10; unit: dimensionless (counts) |
| `weinerPath` | Wiener path index: sum of shortest-path distances over connected pairs of heavy atoms; unit: dimensionless (topology) |
| `weinerPol` | Wiener polarity index: number of heavy-atom pairs separated by three bonds; unit: dimensionless (topology) |
| `diameter` | Graph diameter: largest finite shortest-path distance between heavy atoms; unit: dimensionless (topology) |
| `radius` | Graph radius: smallest heavy-atom eccentricity, using finite distances; unit: dimensionless (topology) |
| `petitjean` | Petitjean shape index: (diameter - radius) / radius, or zero when radius is zero; unit: dimensionless (topology) |
| `zagreb1` | First Zagreb index: sum of squared heavy-atom degrees; unit: dimensionless (topology) |
| `zagreb2` | Second Zagreb index: sum of products of endpoint degrees over heavy-atom bonds; unit: dimensionless (topology) |
| `chi0` | Zero-order connectivity index: sum of inverse square roots of heavy-atom degrees; unit: dimensionless (topology); reference: Hall and Kier |
| `chi0v` | Zero-order valence connectivity index using Hall-Kier valence deltas; unit: dimensionless (topology); reference: Hall and Kier |
| `chi1` | First-order connectivity index: sum of inverse square roots of endpoint degree products over heavy-atom bonds; unit: dimensionless (topology); reference: Hall and Kier |
| `chi1v` | First-order valence connectivity index using Hall-Kier valence deltas; unit: dimensionless (topology); reference: Hall and Kier |
| `balabanJ` | Balaban J index: distance-sum connectivity normalised by bond count and cyclomatic number; unit: dimensionless (topology); reference: Balaban, Chem. Phys. Lett. 89 (1982) 399 |
| `Kier1` | Order-1 Kier kappa shape index of the heavy-atom graph; unit: dimensionless (topology); reference: Hall and Kier |
| `Kier2` | Order-2 Kier kappa shape index of the heavy-atom graph; unit: dimensionless (topology); reference: Hall and Kier |
| `Kier3` | Order-3 Kier kappa shape index of the heavy-atom graph; unit: dimensionless (topology); reference: Hall and Kier |
| `KierA1` | Order-1 Kier kappa shape index of the heavy-atom graph; alpha correction from covalent radii, without a hybridisation term; unit: dimensionless (topology); reference: Hall and Kier |
| `KierA2` | Order-2 Kier kappa shape index of the heavy-atom graph; alpha correction from covalent radii, without a hybridisation term; unit: dimensionless (topology); reference: Hall and Kier |
| `KierA3` | Order-3 Kier kappa shape index of the heavy-atom graph; alpha correction from covalent radii, without a hybridisation term; unit: dimensionless (topology); reference: Hall and Kier |
| `KierFlex` | Kier flexibility index: KierA1 times KierA2 divided by heavy-atom count; unit: dimensionless (topology) |
| `ic0` | Topological information content: Shannon entropy of heavy-atom symmetry classes; unit: bits (topology) |
| `SlogP` | Wildman-Crippen octanol/water partition coefficient model (log10 P); unit: dimensionless (models); reference: Wildman and Crippen, J. Chem. Inf. Comput. Sci. 39 (1999) 868 |
| `SMR` | Wildman-Crippen molar refractivity model; unit: cm³/mol (models); reference: Wildman and Crippen, J. Chem. Inf. Comput. Sci. 39 (1999) 868 |
| `TPSA` | Topological polar surface area from nitrogen and oxygen fragments only; unit: Å² (models); reference: Ertl et al., J. Med. Chem. 43 (2000) 3714 |
| `LabuteASA` | Labute approximate molecular surface area including the hydrogen correction; unit: Å² (models); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464 |
| `Q_PC+` | Sum of positive heavy-atom Gasteiger charges (PEOE, 12 iterations); unit: e (charges); reference: Gasteiger and Marsili, Tetrahedron 36 (1980) 3219 |
| `Q_PC-` | Sum of negative heavy-atom Gasteiger charges (PEOE, 12 iterations); unit: e (charges); reference: Gasteiger and Marsili, Tetrahedron 36 (1980) 3219 |
| `Q_RPC+` | Largest positive heavy-atom Gasteiger charge divided by the positive charge sum (zero if absent); unit: dimensionless (charges); reference: Gasteiger and Marsili, Tetrahedron 36 (1980) 3219 |
| `Q_RPC-` | Most negative heavy-atom Gasteiger charge divided by the negative charge sum (zero if absent); unit: dimensionless (charges); reference: Gasteiger and Marsili, Tetrahedron 36 (1980) 3219 |
| `Q_max` | Maximum heavy-atom Gasteiger charge (PEOE, 12 iterations); unit: e (charges); reference: Gasteiger and Marsili, Tetrahedron 36 (1980) 3219 |
| `Q_min` | Minimum heavy-atom Gasteiger charge (PEOE, 12 iterations); unit: e (charges); reference: Gasteiger and Marsili, Tetrahedron 36 (1980) 3219 |
| `PEOE_VSA_POS` | Labute heavy-atom surface area with Gasteiger charge > 0; unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Gasteiger and Marsili, Tetrahedron 36 (1980) 3219 |
| `PEOE_VSA_NEG` | Labute heavy-atom surface area with Gasteiger charge < 0; unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Gasteiger and Marsili, Tetrahedron 36 (1980) 3219 |
| `PEOE_VSA_PPOS` | Labute heavy-atom surface area with Gasteiger charge > 0.2 e; unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Gasteiger and Marsili, Tetrahedron 36 (1980) 3219 |
| `PEOE_VSA_PNEG` | Labute heavy-atom surface area with Gasteiger charge < -0.2 e; unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Gasteiger and Marsili, Tetrahedron 36 (1980) 3219 |
| `PEOE_VSA_HYD` | Labute heavy-atom surface area with Gasteiger absolute charge <= 0.2 e; unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Gasteiger and Marsili, Tetrahedron 36 (1980) 3219 |
| `PEOE_VSA_FPOS` | Labute heavy-atom surface area with Gasteiger charge > 0, divided by LabuteASA (including its hydrogen correction); unit: dimensionless (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Gasteiger and Marsili, Tetrahedron 36 (1980) 3219 |
| `PEOE_VSA_FNEG` | Labute heavy-atom surface area with Gasteiger charge < 0, divided by LabuteASA (including its hydrogen correction); unit: dimensionless (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Gasteiger and Marsili, Tetrahedron 36 (1980) 3219 |
| `PEOE_VSA1` | Van der Waals surface area (Labute approximation) of heavy atoms whose Gasteiger charge (e; PEOE, 12 iterations) is in bin 1 (x < -0.3); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Gasteiger and Marsili, Tetrahedron 36 (1980) 3219 |
| `PEOE_VSA2` | Van der Waals surface area (Labute approximation) of heavy atoms whose Gasteiger charge (e; PEOE, 12 iterations) is in bin 2 (-0.3 <= x < -0.25); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Gasteiger and Marsili, Tetrahedron 36 (1980) 3219 |
| `PEOE_VSA3` | Van der Waals surface area (Labute approximation) of heavy atoms whose Gasteiger charge (e; PEOE, 12 iterations) is in bin 3 (-0.25 <= x < -0.2); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Gasteiger and Marsili, Tetrahedron 36 (1980) 3219 |
| `PEOE_VSA4` | Van der Waals surface area (Labute approximation) of heavy atoms whose Gasteiger charge (e; PEOE, 12 iterations) is in bin 4 (-0.2 <= x < -0.15); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Gasteiger and Marsili, Tetrahedron 36 (1980) 3219 |
| `PEOE_VSA5` | Van der Waals surface area (Labute approximation) of heavy atoms whose Gasteiger charge (e; PEOE, 12 iterations) is in bin 5 (-0.15 <= x < -0.1); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Gasteiger and Marsili, Tetrahedron 36 (1980) 3219 |
| `PEOE_VSA6` | Van der Waals surface area (Labute approximation) of heavy atoms whose Gasteiger charge (e; PEOE, 12 iterations) is in bin 6 (-0.1 <= x < -0.05); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Gasteiger and Marsili, Tetrahedron 36 (1980) 3219 |
| `PEOE_VSA7` | Van der Waals surface area (Labute approximation) of heavy atoms whose Gasteiger charge (e; PEOE, 12 iterations) is in bin 7 (-0.05 <= x < 0); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Gasteiger and Marsili, Tetrahedron 36 (1980) 3219 |
| `PEOE_VSA8` | Van der Waals surface area (Labute approximation) of heavy atoms whose Gasteiger charge (e; PEOE, 12 iterations) is in bin 8 (0 <= x < 0.05); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Gasteiger and Marsili, Tetrahedron 36 (1980) 3219 |
| `PEOE_VSA9` | Van der Waals surface area (Labute approximation) of heavy atoms whose Gasteiger charge (e; PEOE, 12 iterations) is in bin 9 (0.05 <= x < 0.1); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Gasteiger and Marsili, Tetrahedron 36 (1980) 3219 |
| `PEOE_VSA10` | Van der Waals surface area (Labute approximation) of heavy atoms whose Gasteiger charge (e; PEOE, 12 iterations) is in bin 10 (0.1 <= x < 0.15); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Gasteiger and Marsili, Tetrahedron 36 (1980) 3219 |
| `PEOE_VSA11` | Van der Waals surface area (Labute approximation) of heavy atoms whose Gasteiger charge (e; PEOE, 12 iterations) is in bin 11 (0.15 <= x < 0.2); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Gasteiger and Marsili, Tetrahedron 36 (1980) 3219 |
| `PEOE_VSA12` | Van der Waals surface area (Labute approximation) of heavy atoms whose Gasteiger charge (e; PEOE, 12 iterations) is in bin 12 (0.2 <= x < 0.25); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Gasteiger and Marsili, Tetrahedron 36 (1980) 3219 |
| `PEOE_VSA13` | Van der Waals surface area (Labute approximation) of heavy atoms whose Gasteiger charge (e; PEOE, 12 iterations) is in bin 13 (0.25 <= x < 0.3); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Gasteiger and Marsili, Tetrahedron 36 (1980) 3219 |
| `PEOE_VSA14` | Van der Waals surface area (Labute approximation) of heavy atoms whose Gasteiger charge (e; PEOE, 12 iterations) is in bin 14 (x >= 0.3); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Gasteiger and Marsili, Tetrahedron 36 (1980) 3219 |
| `SlogP_VSA1` | Van der Waals surface area (Labute approximation) of heavy atoms whose Wildman-Crippen logP contribution is in bin 1 (x < -0.4); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Wildman and Crippen, J. Chem. Inf. Comput. Sci. 39 (1999) 868 |
| `SlogP_VSA2` | Van der Waals surface area (Labute approximation) of heavy atoms whose Wildman-Crippen logP contribution is in bin 2 (-0.4 <= x < -0.2); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Wildman and Crippen, J. Chem. Inf. Comput. Sci. 39 (1999) 868 |
| `SlogP_VSA3` | Van der Waals surface area (Labute approximation) of heavy atoms whose Wildman-Crippen logP contribution is in bin 3 (-0.2 <= x < 0); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Wildman and Crippen, J. Chem. Inf. Comput. Sci. 39 (1999) 868 |
| `SlogP_VSA4` | Van der Waals surface area (Labute approximation) of heavy atoms whose Wildman-Crippen logP contribution is in bin 4 (0 <= x < 0.1); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Wildman and Crippen, J. Chem. Inf. Comput. Sci. 39 (1999) 868 |
| `SlogP_VSA5` | Van der Waals surface area (Labute approximation) of heavy atoms whose Wildman-Crippen logP contribution is in bin 5 (0.1 <= x < 0.15); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Wildman and Crippen, J. Chem. Inf. Comput. Sci. 39 (1999) 868 |
| `SlogP_VSA6` | Van der Waals surface area (Labute approximation) of heavy atoms whose Wildman-Crippen logP contribution is in bin 6 (0.15 <= x < 0.2); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Wildman and Crippen, J. Chem. Inf. Comput. Sci. 39 (1999) 868 |
| `SlogP_VSA7` | Van der Waals surface area (Labute approximation) of heavy atoms whose Wildman-Crippen logP contribution is in bin 7 (0.2 <= x < 0.25); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Wildman and Crippen, J. Chem. Inf. Comput. Sci. 39 (1999) 868 |
| `SlogP_VSA8` | Van der Waals surface area (Labute approximation) of heavy atoms whose Wildman-Crippen logP contribution is in bin 8 (0.25 <= x < 0.3); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Wildman and Crippen, J. Chem. Inf. Comput. Sci. 39 (1999) 868 |
| `SlogP_VSA9` | Van der Waals surface area (Labute approximation) of heavy atoms whose Wildman-Crippen logP contribution is in bin 9 (0.3 <= x < 0.4); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Wildman and Crippen, J. Chem. Inf. Comput. Sci. 39 (1999) 868 |
| `SlogP_VSA10` | Van der Waals surface area (Labute approximation) of heavy atoms whose Wildman-Crippen logP contribution is in bin 10 (0.4 <= x < 0.5); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Wildman and Crippen, J. Chem. Inf. Comput. Sci. 39 (1999) 868 |
| `SlogP_VSA11` | Van der Waals surface area (Labute approximation) of heavy atoms whose Wildman-Crippen logP contribution is in bin 11 (0.5 <= x < 0.6); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Wildman and Crippen, J. Chem. Inf. Comput. Sci. 39 (1999) 868 |
| `SlogP_VSA12` | Van der Waals surface area (Labute approximation) of heavy atoms whose Wildman-Crippen logP contribution is in bin 12 (x >= 0.6); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Wildman and Crippen, J. Chem. Inf. Comput. Sci. 39 (1999) 868 |
| `SMR_VSA1` | Van der Waals surface area (Labute approximation) of heavy atoms whose Wildman-Crippen molar refractivity contribution (cm³/mol) is in bin 1 (x < 1.29); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Wildman and Crippen, J. Chem. Inf. Comput. Sci. 39 (1999) 868 |
| `SMR_VSA2` | Van der Waals surface area (Labute approximation) of heavy atoms whose Wildman-Crippen molar refractivity contribution (cm³/mol) is in bin 2 (1.29 <= x < 1.82); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Wildman and Crippen, J. Chem. Inf. Comput. Sci. 39 (1999) 868 |
| `SMR_VSA3` | Van der Waals surface area (Labute approximation) of heavy atoms whose Wildman-Crippen molar refractivity contribution (cm³/mol) is in bin 3 (1.82 <= x < 2.24); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Wildman and Crippen, J. Chem. Inf. Comput. Sci. 39 (1999) 868 |
| `SMR_VSA4` | Van der Waals surface area (Labute approximation) of heavy atoms whose Wildman-Crippen molar refractivity contribution (cm³/mol) is in bin 4 (2.24 <= x < 2.45); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Wildman and Crippen, J. Chem. Inf. Comput. Sci. 39 (1999) 868 |
| `SMR_VSA5` | Van der Waals surface area (Labute approximation) of heavy atoms whose Wildman-Crippen molar refractivity contribution (cm³/mol) is in bin 5 (2.45 <= x < 2.75); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Wildman and Crippen, J. Chem. Inf. Comput. Sci. 39 (1999) 868 |
| `SMR_VSA6` | Van der Waals surface area (Labute approximation) of heavy atoms whose Wildman-Crippen molar refractivity contribution (cm³/mol) is in bin 6 (2.75 <= x < 3.05); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Wildman and Crippen, J. Chem. Inf. Comput. Sci. 39 (1999) 868 |
| `SMR_VSA7` | Van der Waals surface area (Labute approximation) of heavy atoms whose Wildman-Crippen molar refractivity contribution (cm³/mol) is in bin 7 (3.05 <= x < 3.63); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Wildman and Crippen, J. Chem. Inf. Comput. Sci. 39 (1999) 868 |
| `SMR_VSA8` | Van der Waals surface area (Labute approximation) of heavy atoms whose Wildman-Crippen molar refractivity contribution (cm³/mol) is in bin 8 (3.63 <= x < 3.8); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Wildman and Crippen, J. Chem. Inf. Comput. Sci. 39 (1999) 868 |
| `SMR_VSA9` | Van der Waals surface area (Labute approximation) of heavy atoms whose Wildman-Crippen molar refractivity contribution (cm³/mol) is in bin 9 (3.8 <= x < 4); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Wildman and Crippen, J. Chem. Inf. Comput. Sci. 39 (1999) 868 |
| `SMR_VSA10` | Van der Waals surface area (Labute approximation) of heavy atoms whose Wildman-Crippen molar refractivity contribution (cm³/mol) is in bin 10 (x >= 4); unit: Å² (VSA); reference: Labute, J. Mol. Graph. Model. 18 (2000) 464; Wildman and Crippen, J. Chem. Inf. Comput. Sci. 39 (1999) 868 |
| `qm_E` | Total electronic energy from calculation property TE (last value); unit: Hartree (QM) |
| `qm_HOMO` | Highest occupied molecular orbital energy from ORBENG and ORBOCC (occupation > 0.5), converted from Hartree; unit: eV (QM) |
| `qm_LUMO` | Lowest unoccupied molecular orbital energy from ORBENG and ORBOCC (occupation <= 0.5), converted from Hartree; unit: eV (QM) |
| `qm_gap` | HOMO-LUMO energy gap from calculation properties ORBENG and ORBOCC (LUMO minus HOMO); unit: eV (QM) |
| `qm_dipole` | Magnitude of the three-component calculation property DIPOLE; unit: Debye (QM) |
| `qm_qmin` | Minimum atomic Mulliken charge from calculation property MULLIKEN; unit: e (QM) |
| `qm_qmax` | Maximum atomic Mulliken charge from calculation property MULLIKEN; unit: e (QM) |
| `problems` | Reasons a row is incomplete or flagged by the calculation checks or SMILES reader; unit: none (QM) |
<!-- END QSAR DESCRIPTOR KEY -->

The descriptors are computed from the SMILES that was stored with the
calculation, so they do not depend on the geometry. They follow the published
definitions and agree with RDKit's values for SlogP, SMR and TPSA; they are
not MOE's: MOE's own logP and logS models and its pKa-based descriptors are not
reproduced, and a column named like a MOE column is the same quantity only
where the definition above says so.

## Command

```
ecce-qsar table <folder|calc>... [--groups counts,topology,models,charges,vsa,qm]
                [--csv FILE|-] [--json] [--no-qm] [--describe] [--key-csv FILE]
ecce-qsar table --from-csv FILE [--name-col N] [--smiles-col N] ...
ecce-qsar groups
```

`--from-csv` reads a table of names and SMILES (other numeric columns become
measured values) and computes the descriptors without any calculation.
### Describing columns

`ecce-qsar table --describe` prints the built-in descriptor key without needing
an input. Add calculation paths or `--from-csv FILE` to describe just the
columns of that table, including measured values and their provenance.
`--groups` and `--no-qm` select columns as for a normal table.
`--describe --json` returns a `columns` array of objects with `name`,
`description`, `unit`, `group` and `reference` fields; an empty reference means
none is specified. `--describe --csv FILE` writes these fields as CSV.

### Exporting the key with a table

`--key-csv FILE` also writes a UTF-8 CSV of each current column name and the
formatted key text. This is the same text used by the Organizer's header
tooltips and Key dialog.

Test hook of the Organizer: `qsartable <out.csv> <url>,<url>... [groups]`
(also writes `<out.csv>.key` and `<out.csv>.tips` for GUI checks).
