# QSAR

**Tools > QSAR...** opens one window for the selected folder (the calculations
made by [Batch Add from SMILES](batch-from-smiles.md)). Its title and the line
at the top say which folder and how many calculations are used;
**Change folder...** takes the folder now selected in the Organizer. The window
has three tabs, **Table**, **PCA** and **PLS**. PCA and PLS use the table of the
first tab, so ticking other descriptor groups there updates what they analyse.
A line under the tabs says what the last run found, for example "PCA done: 2
components explain 66 % of the variation (cross-validated Q2 0.30)". Everything
the window does is also done by `ecce-qsar`, so an analysis can be repeated from
a script.

## Table

1. Select the project folder (or some of its calculations) in the Organizer.
2. Choose **Tools > QSAR...** and tick the descriptor groups.
3. Click a column header to sort. A row with a problem says what in the
   `problems` column: a SMILES that cannot be read, a calculation without a
   stored SMILES, an optimisation that did not converge, an abnormal termination.
4. **Export CSV...** writes the table, for ECCE's own PCA and PLS or for other
   programs.

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

## PCA

On the **PCA** tab choose the scaling and the number of components and click **Run PCA**. The
descriptors are the table's numeric columns; the measured values are not part of
the model. Columns with missing values or no variance are left out, and
calculations whose optimisation did not converge are left out; the report lists
both.

- **Scaling**: **Autoscale** (each column centred and divided by its standard deviation,
  SIMCA's unit-variance scaling), **Pareto** (divided by the square root of it),
  **Centre only**.
- **Components**: Automatic lets cross-validation decide: components are added while
  the cross-validated Q2 of the next component exceeds 0.05. The
  cross-validation is the element-wise scheme of Wold in 7 groups.
- **Report**: for each component the eigenvalue, R2X (share of the variance),
  and Q2; the limits of Hotelling's T2 and of DModX (95 %), and the molecules
  outside them. T2 measures the distance within the model plane, DModX the
  distance from it.
- **Plot**: t1 against t2, with R2X of each component below.
- **Save Results...** writes `table.csv`, `pca_scores.csv` (scores, T2, DModX),
  `pca_loadings.csv` and `pca_components.csv`.

## PLS

On the **PLS** tab choose the property to model (a column of the table that is not a descriptor), the
scaling and the number of components (Automatic: by cross-validation), and click **Run
PLS**. One measured value is modelled at a time. X and y are scaled alike.

- **Test set %**: a random share of the molecules is kept out of the fit and
  predicted afterwards (RMSEP, external Q2). Name molecules instead with
  `--test-names` on the command.
- **Cross-validation**: every 7th molecule is left out in turn, and the scaling
  is redone on the rest, so Q2 does not use the left-out molecules' means. A
  component is kept while PRESS falls by at least 10 % (Q2 of the component at
  least 0.0975, as in SIMCA).
- **Report**: R2X, R2Y, Q2 and RMSECV per component; RMSEE (training error,
  N-A-1 degrees of freedom); the largest VIP values; the limits of T2 and
  DModX. The files hold VIP and the coefficients (scaled and in the original
  units), the scores with T2 and DModX, and observed against predicted
  (`train`, `test`, and the cross-validated prediction).
- **Permutations**: the measured values are shuffled and the model refitted;
  the lines of R2Y and Q2 against the correlation of the shuffled with the true
  values are extended to zero correlation. An R2Y intercept below 0.3-0.4 and
  a Q2 intercept below 0.05 mean the model is not a chance result.
- **Plot**: observed against predicted, red for molecules not in the fit.

A saved model (`pls_model.json`, written by **Save Results...**) predicts new
molecules: `ecce-qsar predict pls_model.json new_table.csv`. Each row gets the
prediction, T2 and DModX, and `in_domain` says whether both are within the 95 %
limits; a prediction for a molecule outside them is an extrapolation.

## Command

```
ecce-qsar pca TABLE.csv [--y COL]... [--exclude COL]... [--scale uv|pareto|ctr|none]
              [--components N] [--cv G] [--keep-unconverged] [--outdir DIR] [--json]
```

```
ecce-qsar pls TABLE.csv --y COL [--exclude COL]... [--scale uv|pareto|ctr|none]
              [--components N] [--cv G] [--test-names A,B | --test-fraction F --seed S]
              [--permutations N] [--save-model FILE] [--outdir DIR] [--json]
ecce-qsar predict MODEL.json TABLE.csv [--csv FILE]
```

`--y` names the measured columns (not descriptors); `ecce-qsar columns
TABLE.csv` lists them. The test hooks of the Organizer are `qsartable <out.csv> <url>,<url>...
[groups] [png]`, `qsarpca <outdir> <url>,<url>... [scaling] [components] [png]`
and `qsarpls <outdir> <url>,<url>... <y> [scaling] [components] [test%]
[permutations] [png]`.

Reading the results: the tables use words with the symbol after, and the column
to read is in bold. **R2X** is the share of the variation between the molecules
that a component holds, **R2Y** the share of the variation in the property that
the model reproduces, **Q2** the same for molecules left out of the fit (the
number to judge a model by), **VIP** how much a descriptor matters (above 1 is
more than average), **T2** and **DModX** how far a molecule lies from the others
within and away from the model plane. A descriptor that is itself a model
estimate of the property (SlogP when modelling logP) is flagged in the PLS tab,
with a tick box to leave it out. Plots show the property in the title, the
explained share on the PCA axes, and the units of the column header; hover over
a point for its name and values.

