---
type: pitfall
title: "The Builder draws normal modes (and MOs, dipole) on the last GEOMTRACE step, so a code that reorients must deliver one (#312)"
area: codereg
paths: [tests/apps/mopac_import_modes_test.py, scripts/parsers/mopac.vib, scripts/parsers/mopac.forceframe, scripts/parsers/orca.desc, scripts/parsers/nwchem.desc, scripts/parsers/gaussian-16.desc, src/viz/propsgcommands/ShowFragCmd.C, tests/parsers/vib_frame_test.py, tests/apps/vib_frame_builder_test.py]
issues: [312, 369]
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

**MOPAC** reorients in a FORCE run ("ORIENTATION OF MOLECULE IN FORCE
CALCULATION") and prints its modes in that frame, while `[GEOMTRACE]` keeps
the `CARTESIAN COORDINATES` blocks in the input orientation (#369). The
parser rotates the modes back instead of replacing the geometry:
`mopac.geomtrace` and `mopac.forceframe` (entry `[MOPFORCEFRAME]`, not a
property) leave both frames as `$key/mopacInputGeom` and `mopacForceGeom`,
and `mopac.vib` fits one onto the other (Horn, proper rotation, refused above
0.01 A rms) and rotates every mode. Two traps: the force orientation cannot
be a wider `[VIB]` Begin, since that block swallows the heat of formation and
zero-point energy after it; and any harness that runs the scripts must give
them one shared work directory. Without the side files `mopac.vib` exits 1 with
its reason on stderr (a parse error in the job's activity log) and stores no
modes, rather than storing them unrotated. The real Organizer import is
checked by `tests/apps/mopac_import_modes_test.py` (stored modes and the
Builder's arrows on the imported calculation). `tests/parsers/vib_frame_test.py` checks the
mass-weighted share of every MOPAC vibration inside the six rigid motions of
the stored structure (was 0.2 to 0.7, now below 1e-6), and
`tests/vibanalysis` labels the stored geometry directly.
