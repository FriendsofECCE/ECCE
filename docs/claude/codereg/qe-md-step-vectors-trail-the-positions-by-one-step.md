---
type: pitfall
title: "QE molecular dynamics: the energies and temperature describe the step before the positions printed with them; and an md run always ends on the step limit"
area: codereg
section: "How a property gets from disk into the Properties menu"
paths: ["scripts/parsers/qe.desc", "scripts/parsers/qe.mdstep", "scripts/parsers/ai.qe", "scripts/codereg/qeruntype.py", "tests/qe_features/run_tests.py", "tests/parsers/cases.py"]
issues: [329]
---
Checked against pw.x 6.7, `calculation='md'` (runtype `Dynamics` in
`QuantumESPRESSO.edml`).

- After each SCF, pw.x prints `Entering Dynamics: iteration = k`, the
  `ATOMIC_POSITIONS` of the *next* geometry, then `kinetic energy (Ekin)`,
  `temperature` and `Ekin + Etot (const)`. Those three belong to the state
  the forces were just computed at, one time step before the positions above
  them. `qe.mdstep` turns them into `KEVEC`, `TEMPVEC`, `TEVEC` (the
  conserved Ekin + Etot) and `PEVEC` (the difference, the DFT energy), so
  `len(vector) == len(GEOMTRACE)` and the first frame of the trace is the
  geometry after the first move, not the starting one. Same offset as relax
  (see `qe.desc`), and the reason no stateless entry pairs them exactly.
- `dt` is in Rydberg atomic units of time, 4.8377687e-17 s: the dialog's fs
  are divided by 0.04837768652 in `ai.qe` (1 fs = 20.6707).
- The `&IONS` keys pw.x accepts depend on `ion_temperature`; `ai.qe` writes
  only `tempw` (also the starting temperature, with every thermostat),
  `nraise` (berendsen, andersen, svr) and `tolp` (rescaling). The 6.7 binary
  accepted all of not_controlled, initial, rescaling, rescale-v, rescale-T,
  reduce-T, berendsen, andersen and svr; the dialog offers five of them.
- An md run always ends with "The maximum number of steps has been reached.",
  which the `[PROGRESS]` entry for a relax reads as `opt converged=no`, so the
  Organizer shows an "Optimisation not converged" line after an md run.
  **Not fixed.** Tried writing the stage as `{mdrun}opt`, invalid in md:
  `eccejobmonitor` refuses to load a `Progress=` that does not start with a
  literal stage name or a lone `{name}` (that killed the monitor for every QE
  job until the test caught it), and an empty lone `{name}` drops the entry,
  so a stage cannot be made conditional without a literal `opt` captured from
  a relax-only line (there is none in lower case). A fix is in
  `src/util/genutil/JobProgress.C` (ignore an `opt` stage with no `istep` or
  `step`) or in the monitor (a default for an unset `{name}`; remote hosts
  would need it too).
