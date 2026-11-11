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

- `name`, `state`, `opt_converged` and the **measured values**, under the
  headers they were stored with.
- **counts**: weight, atoms by element, bonds (`b_ar` aromatic, `b_rotN`
  rotatable: single, acyclic, between non-terminal atoms, not at a triple
  bond, not the N-C bond of an N-H amide), rings, `chiral` (atoms with four
  topologically different substituents), `FCharge`, `lip_don`, `lip_acc`,
  `lip_violation` (Lipinski's rules with SlogP).
- **topology** (heavy-atom graph): `weinerPath`, `weinerPol`, `diameter`,
  `radius`, `petitjean`, `zagreb1`, `zagreb2`, `chi0`, `chi1`, `chi0v`, `chi1v`
  (Hall-Kier valence deltas), `balabanJ`, `Kier1-3`, `KierA1-3` (with the
  alpha correction from covalent radii, no hybridisation term), `KierFlex`,
  `ic0` (information content of the topological symmetry classes).
- **models**: `SlogP` and `SMR` (Wildman-Crippen 1999), `TPSA` (Ertl 2000,
  N and O only), `LabuteASA`.
- **charges**: Gasteiger (PEOE, 12 iterations) charges: `Q_PC+`, `Q_PC-`,
  `Q_RPC+`, `Q_RPC-`, `Q_max`, `Q_min`.
- **vsa**: Labute surface area summed by Gasteiger charge (`PEOE_VSA1-14`,
  and the positive, negative, polar and hydrophobic totals), by SlogP
  contribution (`SlogP_VSA1-12`) and by SMR contribution (`SMR_VSA1-10`).
- **From the calculations** (`qm_*`, empty when the calculation has no such
  result): total energy (Hartree), HOMO, LUMO and gap (eV, from the orbital
  energies and occupations), dipole moment (Debye) and the extreme Mulliken
  charges.

The descriptors are computed from the SMILES that was stored with the
calculation, so they do not depend on the geometry. They follow the published
definitions and agree with RDKit's values for SlogP, SMR and TPSA; they are
not MOE's: MOE's own logP and logS models and its pKa-based descriptors are not
reproduced, and a column named like a MOE column is the same quantity only
where the definition above says so.

## Command

```
ecce-qsar table <folder|calc>... [--groups counts,topology,models,charges,vsa,qm]
                [--csv FILE|-] [--json] [--no-qm]
ecce-qsar table --from-csv FILE [--name-col N] [--smiles-col N] ...
ecce-qsar groups
```

`--from-csv` reads a table of names and SMILES (other numeric columns become
measured values) and computes the descriptors without any calculation.
Test hook of the Organizer: `qsartable <out.csv> <url>,<url>... [groups]`.
