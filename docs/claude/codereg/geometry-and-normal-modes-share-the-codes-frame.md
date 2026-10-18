---
type: pitfall
title: "The Builder draws normal modes (and MOs, dipole) on the last GEOMTRACE step, so a code that reorients must deliver one (#312)"
area: codereg
paths: [scripts/parsers/orca.desc, scripts/parsers/nwchem.desc, scripts/parsers/gaussian-16.desc, src/viz/propsgcommands/ShowFragCmd.C, tests/parsers/vib_frame_test.py, tests/apps/vib_frame_builder_test.py]
issues: [312]
---
ORCA with UseSym, Gaussian (standard orientation) and NWChem (`geometry
autosym`) print modes, orbitals and tensors in their own frame, not the one
chemsys.mvm holds. The Builder opens a calculation on the last GEOMTRACE
step (`ShowFragCmd`, index -1), so the frames agree only if the code's own
geometry reaches GEOMTRACE. ORCA and Gaussian print one in every run. NWChem
prints the coordinates of a frequency-only run under `task_hessian
task_energy`, which `[GEOMHESS]` now routes to `nwchem.geomtrace` (it was
discarded, and the arrows floated off the atoms); in GeoVib the script
drops it because the gradient blocks already hold the trace.

Check with `tests/parsers/vib_frame_test.py`: the hydrogen that moves most
in a mode above 2500 cm-1 moves along its own X-H bond only if geometry and
modes share a frame. ORCA's NumFreq turns symmetry off and keeps the input
frame, so it needs no care.

**MOPAC is not covered by that check and fails it.** A MOPAC force run
reorients the molecule ("ORIENTATION OF MOLECULE IN FORCE CALCULATION") and
prints its normal modes in that frame, while `[GEOMTRACE]` keeps the
`CARTESIAN COORDINATES` blocks, which are in the input orientation. For
methane (`tests/parsers/fixtures/mopac/ch4_force.out`, `ch4_geovib.out`,
`ch4_force_nothermo.out`) the modes are not orthogonal to the stored
structure's rotations, so the arrows in the Builder point in the wrong
directions. `VibAnalysis` measures exactly that (the largest share of any
vibration inside the six rigid motions, mass-weighted) and labels nothing
when it exceeds 10 %; `tests/vibanalysis/run_tests.py` asserts the refusal
for the stored MOPAC geometry and analyses the same modes in the force
orientation read from the output.
