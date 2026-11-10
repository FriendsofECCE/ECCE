---
type: map
title: "results.h5 is written from the Props at the end of the job store, and the views switch in one place"
area: codereg
paths: ["src/tdat/chemistry/ResultsFromCalc.C", "src/apps/builder/NModePanel.C", "src/apps/builder/ScanPanel.C", "src/apps/builder/IRCPanel.C", "src/apps/builder/GeomTracePropertyPanel.C", "src/viz/propsgcommands/GTStepCmd.C", "src/viz/propsgcommands/ComputeMoCmd.C", "src/qm/results_h5.cpp", "tests/resultsh5/trace_vib_test.py", "include/tdat/ResultsFromCalc.H", "src/tdat/chemistry/MoAoBasis.C", "src/tdat/chemistry/DensityPlane.C", "src/comm/commxt/JobStore.C", "src/dsm/edsiimpl/PropertyTask.C", "src/tdat/resultsh5/ecce_results.C", "tests/resultsh5/results_calc_test.C"]
issues: [690, 883]
---
**Write.** `jobstore_main` calls `resultsFileStore()` just before `cleanup(0)`
(so before the final run state), after every run and every Re-parse (a
re-parse is the same process with `gReparse`). It flushes the property cache,
runs `ResultsFromCalc::store`, and logs one line to the run log. Any
exception is caught: the job's own result stands. `ecce-results write CALC`
is the same call for calculations stored earlier.

**Conversion.** `gather` takes the last-step geometry, `MoAoBasis::coefficients`
(column order undone for MOAOORDER codes) and `MoAoBasis::flatBasis`, the same
flattened basis the viewer uses. `convert` rebuilds the shells from the flat
functions (consecutive, same atom, same l, same exponents; spherical against
Cartesian from the first l >= 2 shell: a Cartesian function is one monomial)
and maps each flat function onto a canonical one by overlap (a permutation,
a sign and a scale; cosine must exceed 1 - 1e-8 or no file is made). So no
code has its own table and a code ECCE reads gets a file, unless its basis
is a Slater one (MOPAC): those return "no Gaussian shells" and any older
file is removed. The basis centres in the file are the flat functions' own
(bohr per Angstrom 1/0.52917724924, BasisFlatten's constant, not the 092
one DensityPlane uses for query points).

**Checks.** tr(PS) = N per spin (1e-4 relative; absolute for an empty channel, #943) and C'SC = 1 over occupied
orbitals (1e-5), overlap screened for far pairs. A failed check still writes
the file with root attribute `checks = "failed: ..."`; `stored()` returns null
for a marked file. A restricted calculation has one spin-0 group with
occupations 0..2, as in the Props; a second group only when MOBETA exists.

**Read: one switch point.** `ResultsFromCalc::stored(calc)` decides (switch
off with `ECCE_RESULTS_H5=0`, only for the default orbital point, never a
marked file, cached per calculation for 60 s and dropped by
`PropertyTask::flushPropertyCache`). `MoAoBasis::orbitalSet` (Evaluator::build:
Density Map, Density Profile, ecce-density-field) and
`MoAoBasis::buildWithOrbitals` (MOs panel composition, atomic shell labels)
call it, and so does ComputeMoCmd (Density, Spin Density and MO
isosurfaces through `DensityPlane::Evaluator`; `ECCE_SURFACE_PROPS=1` forces
the old walk, #892). `fromModel` puts the basis centres on the geometry asked
for (the shown fragment), as `flatBasis` does, so both paths give the same
density to rounding. NOT switched: the MO diagram, symmetry analysis and the
per-point orbitals (PTORB/PTMO); they need the code's own column order
(ShellRotation, SymmetryAnalysis), which `coefficients` must keep returning.

**Vibrations and traces in the views.** `ResultsFromCalc::property(calc, key)`
is the views' read of VIBFREQ, VIBIR, VIBRAM, VIB, SCANGEOM/ENERGY/COORD,
IRCGEOM/ENERGY/COORD, GEOMTRACE and TEVEC (NModePanel, NModeVectCmd,
NModeTraceCmd, ScanPanel, IRCPanel, GeomTracePropertyPanel, GTStepCmd). It
copies the parsed Prop and puts the file's values in, converted back to the
Prop's units, only when `storedAny(calc)` (any unmarked file, also one
without orbitals, any orbital point) has the same shape; anything else, a
missing Prop, or `ECCE_RESULTS_H5=0` gives `calc->getProperty(key)`. The
shape test is what keeps a running job's growing GEOMTRACE and ECCE-QM's
3N-6 vibrations against 3N parsed ones on the Props. Views keep the pointers
they get, so a copy is never freed (a changed one is retired, not deleted).
Writers (BuilderTraceStress, the live update, the GEOMTRACE export) and
existence checks still use `getProperty`. `ORCA IRCGEOM` carries units "NA";
`toBohr` reads that as Angstrom.

**Storage.** `IPropCalculation::fetchStoredFile/storeFile/removeStoredFile`
(PropertyTask) move the file as a binary output of the calculation; tdat
cannot reach the data store itself. The resultsh5 sources now compile into
`eccetdat` (HDF5 linked there) and `ecce-results` links the data-store
libraries; `ecceresultsh5` as a separate target is gone.

**Vibrations and traces (#883).** `addProperties` also stores VIB (modes,
as the code printed them), VIBRAM, and `/trace/<name>`: `scan` (SCANGEOM,
SCANENERGY, SCANCOORD), `irc` (IRCGEOM, IRCENERGY, IRCCOORD) and from
GEOMTRACE (>= 2 frames) with TEVEC one of `md` (TIMEVEC present, cell from
LATTICEVEC), `ensemble` (CONFREL/POSESCORE) or `optimization`. A companion
whose length differs from the structures is dropped; energies in an unknown
unit are dropped (toHartree returns 0); frames with differing atom counts
(CREST protomers) give no trace. No Prop holds a Hessian or per-step
gradient vectors, so the job store writes neither; ECCE-QM does. A
calculation without Gaussian orbitals (MD, MOPAC) now gets a file with
atoms, vibrations and traces only, `checks = "ok: no orbitals (...)"`;
`stored()` still ignores it (no MOs).

**ECCE-QM writes its own file.** gensub runs `ecce-qm --results results.h5`
(src/qm/results_h5.cpp, compiled only in the ECCE tree with HDF5, from the
same ResultsH5Io/ResultsBasis/EspField sources). The job store fetches
`results.h5` with the wildcard files and `store()` KEEPS a stored file with
provenance `writer = ecce-qm` when it passes `check()`; otherwise it is
rebuilt from the Props. Not identical by design: ecce-qm lists an atom's s
shells first (the Props keep the library's s,sp order), stores only the
3N-6 vibrations, full-precision orbitals and the Hessian.

**Test.** `ctest -R resultsh5_trace` (raw-output oracles: ORCA benzene
modes/IR/opt path, ORCA H2 scan table, ECCE-QM butane scan; ECCE-QM water own
file against the Props file, matched by AO keys). `ctest -R resultsh5_calc`: stored ECCE-QM/ORCA/G16/NWChem calcs
(closed and open shell), ORCA CrO and HF/cc-pVQZ (g) blocks, xtb; density
equality between the two paths to 1e-8; `ecce-results write` back-fill;
`results_calc_test views` on stored examples (ORCA benzene optfreq, ECCE-QM
butane scan and water freq, ORCA CCSD/cc-pVTZ H2 scan, ORCA SN2 IRC): every
view property from the file against the Props to 1e-6 of its scale, and
`ECCE_RESULTS_H5=0` returns the parsed Prop itself.
