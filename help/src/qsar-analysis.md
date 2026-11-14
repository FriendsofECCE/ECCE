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

- **Scaling** defaults to **Unit variance (autoscaling)**: centre each descriptor
  and divide by its standard deviation. Every descriptor then has the same
  variance; this is the usual choice when descriptors have different units.
  **Pareto** centres and divides by the square root of the standard deviation.
  **Centred only** retains the original variances and suits columns in the
  same unit. **None** neither centres nor scales. The choice's tooltip explains
  these transformations; command tokens remain `uv`, `pareto`, `ctr`, `none`.
- **Components**: 0 lets cross-validation decide: components are added while
  the cross-validated Q2 of the next component exceeds 0.05. The
  cross-validation is the element-wise scheme of Wold in 7 groups.
- **Report** starts with the percentage of descriptor variation explained.
  Notes group omitted descriptor or molecule names on one wrapped line for
  each reason, including no variance, missing values and non-convergence.
  A fixed component below the cross-validation significance limit gets an
  explicit warning.
- **Components table** gives eigenvalues, R²X, cumulative R²X, Q², cumulative
  Q² and variance percentages for all computed components. `*` means
  **component kept**, including a component explicitly requested by you;
  it does not claim significance. Select cells and press **Ctrl+C** to copy
  tab-separated values into a spreadsheet; **Copy table** copies the whole
  table with headers.
- **Molecules table** gives scores, Hotelling's T² and DModX. The report
  explains their 95 % limits: high T² means far from the centre within the
  model plane; high DModX means outside the model plane.
- **Plot** shows t1 against t2, with tick values and the variance explained
  in the axis titles. Points have no permanent molecule labels: hover for
  the molecule's name, scores, T² and DModX; click to highlight the point
  and select its molecule-table row.
- **Variance plot** shows a bar for the X variance added by each component
  and a line for cumulative variance, for up to ten computed components.
- **Save Results...** writes the displayed component table as
  `analysis_components.csv`, the wrapped report as `analysis_summary.txt`,
  and the original full precision files `table.csv`, `pca_scores.csv`,
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
- **Report** first explains how much of the measured value's variation the
  model fits (R²Y) and predicts for molecules left out (Q²). Typical errors
  are RMSEE on fitted molecules and RMSECV on molecules left out, in the
  measured column's unit, for example °C for `bp (°C)`. If no unit is stored,
  they are labelled as measured-value units. A much larger RMSECV than RMSEE
  warns that the model fits better than it predicts; few molecules relative
  to the components gets a separate warning. These warnings do not change
  the model or select a different component count.
- **Components table** shows per-component and cumulative R²X, R²Y and Q²,
  percentages and errors with units. Copy and selection work as for PCA.
  The **VIP table** is a short list sorted by decreasing importance, with
  descriptor descriptions from the descriptor key. **VIP > 1** means more
  important than average; it does not establish causation.
- **Molecules table** holds observed, fitted/test and cross-validated values
  and diagnostics. The result files also retain VIP and coefficients, in
  scaled and original units.
- **Permutations**: the measured values are shuffled and the model refitted;
  the lines of R2Y and Q2 against the correlation of the shuffled with the true
  values are extended to zero correlation. An R2Y intercept below 0.3-0.4 and
  a Q2 intercept below 0.05 mean the model is not a chance result.
- **Plot**: observed against predicted, with measured units on both axes,
  tick values, a labelled **y = x** reference line and a colour legend for
  fitted, test-set and cross-validated predictions. Hover for the molecule's
  observed value, prediction and residual; click to highlight it and select
  its molecule row. The variance plot includes R²X, R²Y and Q² for each
  computed component and their cumulative values.
- **Save Results...** also writes `analysis_vip.csv` (the displayed VIP table),
  `pls_components.csv`, `pls_scores.csv`, `pls_vip.csv`,
  `pls_predictions.csv` and, when requested, permutation results.

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
Add `[width height]` after the PNG path to capture a chosen dialog size; the hook also checks table copying, hover and selection.

The command's plain text report uses the same summaries, legends, grouped
notes and component/VIP tables. `--json` and the original numeric CSV files
retain their existing schemas and numerical precision.
