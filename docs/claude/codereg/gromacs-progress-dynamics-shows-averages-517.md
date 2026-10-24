---
type: pitfall
title: "GROMACS dynamics Progress shows the run average +- s.d. from the monitor's own points; the log's averages block has no deviations (#517)"
area: codereg
paths: ["scripts/parsers/gromacs.desc", "scripts/eccejobmonitor", "src/util/genutil/JobProgress.C", "src/apps/organizer/CalculationContextPanel.C", "tests/progress/gromacs_test.py"]
issues: [517, 519]
---
`Average=<key>` in a `[PROGRESS]` entry makes the monitor keep a running mean
and standard deviation (Welford, once per step record) and send `avg= sd= n=`
with the stage; it is not thinned like the 60-point history. The log's
`A V E R A G E S` block has the means but no RMS fluctuations, so the
deviation can only come from the log-step points; the box says "(average)".
Minimisation entries have no `Average=`, so they keep the last value. The
`gmx-etot` stage is never drawn: the summary's "Final energy" for a
dynamics run reads its average. A rerun clears the panel's stored and live
snapshot as soon as the state drops below Submitted (#519), otherwise the old
plots show until the new run's first message.
