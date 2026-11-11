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

## Command

```
ecce-qsar pca TABLE.csv [--y COL]... [--exclude COL]... [--scale uv|pareto|ctr|none]
              [--components N] [--cv G] [--keep-unconverged] [--outdir DIR] [--json]
```

`--y` names the measured columns (not descriptors); `ecce-qsar columns
TABLE.csv` lists them. The test hook of the Organizer is `qsarpca <outdir>
<url>,<url>... [scaling] [components] [png]`.
