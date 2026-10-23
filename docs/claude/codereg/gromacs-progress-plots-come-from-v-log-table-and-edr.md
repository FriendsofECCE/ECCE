---
type: pitfall
title: "GROMACS Progress plots: minimiser Epot/Fmax only with mdrun -v (\\r records), dynamics from the log's fixed-width energies table, density only from the edr (#477)"
area: codereg
paths: [scripts/gensub, scripts/parsers/gromacs.desc, scripts/eccejobmonitor, src/util/genutil/JobProgress.C, tests/progress/gromacs_test.py]
issues: [477, 284]
---
Each plot is its own `[PROGRESS]` stage (`gmx-epot`, `gmx-fmax`, `gmx-temp`,
`gmx-pres`, `gmx-dens`) because the snapshot keeps one history per stage.

- **Minimisation.** The log has no per-step force; `Step=, Dmax=, Epot=,
  Fmax=` goes to stderr only with `mdrun -v`, which gensub adds for steep/cg/
  l-bfgs. The records are separated by `\r`, not newlines, so
  `ProgressTail` ends a record at either. gensub echoes `ECCE-EMTOL <value>`
  into the job output first (same stream, so the tolerance is known before
  the first step). A rejected step prints `Epot= inf`; the regex wants a digit.
- **Dynamics.** `Table=15` in a `[PROGRESS]` entry: the energies block is
  header/value lines of 15-character columns up to a blank line; values are
  kept as `{t_<header>}` and the line `ECCE-TABLE-END <integrator>` triggers
  the entries. `TableStop=A V E R A G E S` keeps the run averages, which
  repeat the marker, out of the plots.
- **Density** is not in the 2025 log's step blocks (the box appears only in the
  averages), so gensub's helper runs `gmx energy` on a copy of `ener.edr` every
  20 s while pressure coupling is on and echoes new points as `ECCE-DENSITY
  <ps> <kg/m3>`. It ends when the script's shell ends. The first points show
  up after the first `nstenergy` output.
- Post-mode monitors read the second file before the log, so a test cannot
  rely on a `Remember` from the log for `{emtol}`; that is why gensub echoes it.
