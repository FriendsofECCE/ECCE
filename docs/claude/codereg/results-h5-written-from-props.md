---
type: map
title: "results.h5 is written from the Props at the end of the job store, and the views switch in one place"
area: codereg
paths: ["src/tdat/chemistry/ResultsFromCalc.C", "include/tdat/ResultsFromCalc.H", "src/tdat/chemistry/MoAoBasis.C", "src/tdat/chemistry/DensityPlane.C", "src/comm/commxt/JobStore.C", "src/dsm/edsiimpl/PropertyTask.C", "src/tdat/resultsh5/ecce_results.C", "tests/resultsh5/results_calc_test.C"]
issues: [690]
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

**Checks.** tr(PS) = N per spin (1e-4 relative) and C'SC = 1 over occupied
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
call it. `fromModel` puts the basis centres on the geometry asked for (the
shown fragment), as `flatBasis` does, so both paths give the same density to
rounding. NOT switched: ComputeMoCmd (orbital isosurfaces), the MO diagram,
symmetry analysis and IRC panel; they need the code's own column order
(ShellRotation, SymmetryAnalysis), which `coefficients` must keep returning.

**Storage.** `IPropCalculation::fetchStoredFile/storeFile/removeStoredFile`
(PropertyTask) move the file as a binary output of the calculation; tdat
cannot reach the data store itself. The resultsh5 sources now compile into
`eccetdat` (HDF5 linked there) and `ecce-results` links the data-store
libraries; `ecceresultsh5` as a separate target is gone.

**Test.** `ctest -R resultsh5_calc`: stored ECCE-QM/ORCA/G16/NWChem calcs
(closed and open shell), ORCA CrO and HF/cc-pVQZ (g) blocks, xtb; density
equality between the two paths to 1e-8; `ecce-results write` back-fill.
