# QSAR: PCA and PLS

**Tools > QSAR Analysis...** analyses the [descriptor table](qsar-table.md) of
the selected folder. Everything it does is also done by `ecce-qsar`, so an
analysis can be repeated from a script.

## PCA

Choose the scaling and the number of components and click **Run PCA**. The
descriptors are the table's numeric columns; the measured values are not part of
the model. Columns with missing values or no variance are left out, and
calculations whose optimisation did not converge are left out; the report lists
both.

- **Scaling**: `uv` (each column divided by its standard deviation, as SIMCA's
  unit-variance scaling), `pareto` (by the square root of it), `ctr` (centred
  only), `none`.
- **Components**: 0 lets cross-validation decide: components are added while
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

Choose the measured value (a column of the table that is not a descriptor), the
scaling and the number of components (0: by cross-validation), and click **Run
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
TABLE.csv` lists them. The test hook of the Organizer is `qsarpca <outdir>
<url>,<url>... [scaling] [components] [png]` and `qsarpls <outdir>
<url>,<url>... <y> [scaling] [components] [test%] [permutations] [png]`.
