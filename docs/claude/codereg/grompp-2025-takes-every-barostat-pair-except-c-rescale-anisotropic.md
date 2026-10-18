---
type: pitfall
title: "grompp 2025.2: C-rescale does not support anisotropic coupling, Berendsen draws a warning, and a topology-versus-gro atom-name mismatch is a warning too"
area: codereg
paths: [scripts/parsers/GromppWarn.pm, src/tdat/nwmdtask/InteractionModel.C, scripts/gensub, scripts/parsers/gromacs.launchpp, tests/gromacs_md/run_settings_test.py]
issues: []
---
Checked by running grompp 2025.2 on every barostat/coupling-type pair
(`tests/gromacs_md/run_settings_test.py`): C-rescale with anisotropic is an
ERROR ("does not support pressure coupling type Anisotropic yet"); every
Berendsen pair passes with one WARNING ("does not generate any strictly
correct ensemble"); Parrinello-Rahman and C-rescale otherwise pass.
`GromacsRun::pairProblem` repeats this, so it has to change with grompp.

Two traps in the same area. A restrained topology needs
`refcoord-scaling = com` once a barostat is on, so the editor writes it for
"Restrain positions" with constant pressure. And a `-maxwarn 0` run fails on
an unrelated warning when the structure's atom names differ from the
topology's ("N non-matching atom names"): the clay workflow this was
checked against has 4284 of them between `preem.gro` and `opc3.itp`, and its
own run scripts pass `-maxwarn 1` or `2`. ECCE now runs grompp once with
`-maxwarn 999`, sorts the warnings with the table in
`scripts/parsers/GromppWarn.pm` (atom names, Berendsen thermostat and
barostat, GROMOS twin-range, a net charge below 1e-4) and passes the number
of harmless ones as `-maxwarn`, so any other warning still stops the run.
The launch check lists each warning as harmless or STOPPING (grompp itself
only warns about net charge above 1e-4, so that entry rarely applies); the
job script (gensub) does the same with an awk program made from the same
table. The launch check's stdout on success is not shown by the GUI, only
a stop is. `tests/gromacs_md/grompp_warn_test.py`.

A define the topology never uses (`-DPOSRES` with no `[ position_restraints ]`
behind an `#ifdef POSRES`, or any misspelt flag) is a grompp WARNING, so it
stops a launch. pdb2gmx 2025 writes one `#ifdef POSRES` in every chain's
include, so "restrain chain B only" cannot be chosen by define alone: the
Run settings accept several names, which helps only for a topology whose
chains have their own.
