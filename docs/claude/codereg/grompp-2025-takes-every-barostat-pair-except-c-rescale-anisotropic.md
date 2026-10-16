---
type: pitfall
title: "grompp 2025.2: C-rescale does not support anisotropic coupling, Berendsen draws a warning, and a topology-versus-gro atom-name mismatch is a warning too"
area: codereg
paths: [src/tdat/nwmdtask/InteractionModel.C, scripts/gensub, scripts/parsers/gromacs.launchpp, tests/gromacs_md/run_settings_test.py]
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
own run scripts pass `-maxwarn 1` or `2`. ECCE passes 0 (plus one for
Berendsen, one for GROMOS), so such a system is stopped at launch until the
names agree.

A define the topology never uses (`-DPOSRES` with no `[ position_restraints ]`
behind an `#ifdef POSRES`, or any misspelt flag) is a grompp WARNING, so it
stops a launch. pdb2gmx 2025 writes one `#ifdef POSRES` in every chain's
include, so "restrain chain B only" cannot be chosen by define alone: the
Run settings accept several names, which helps only for a topology whose
chains have their own.
