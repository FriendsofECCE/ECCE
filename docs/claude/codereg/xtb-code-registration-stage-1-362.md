---
type: map
title: "xtb (xTB) is registered like MOPAC/CP2K: a one-file deck split by gensub, geometries appended to the output, orbitals (stage 2), charges/vibrations still to come (#362)"
area: codereg
section: "Getting a calculation set up (the \"code registration\" system)"
paths: ["data/client/cap/xTB.edml", "scripts/parsers/ai.xtb", "scripts/parsers/xtb.desc", "scripts/gensub", "scripts/codereg/xtbtheory.py", "tests/launch/xtb_test.py"]
issues: [362]
---
Application type `xTB` (resource `xtb_es`, CONFIG key `xtb`, gensub sub `xtb`).
Theories GFN2-xTB, GFN1-xTB, GFN-FF (category SE, `needsBasis="false"`);
runtypes Energy and Geometry.

- **Deck = one file, two parts.** xtb has no input format that holds settings
  and coordinates, and it refuses a file whose extension it does not know, so
  `xtb.xtbin` is line 1 `xtb <options>` followed by an xyz file. gensub
  splits it (`xtbopts=`, `xtb.xyz`) and runs `xtb xtb.xyz $xtbopts`. The
  options are the user's to edit; `verifyinput` checks only the shape and the
  charge/`--uhf` parity. An xcontrol file (`--input`) cannot carry the
  geometry and ignores unknown blocks silently (`$alpb` was accepted and did
  nothing), so options stay on the command line.
- **xtb prints no coordinates.** After the run gensub appends the frames of
  `xtbopt.log` (or `xtb.xyz` for a single point) to the output between
  `ECCE-XTB-FRAME n natoms` / `ECCE-XTB-ENDFRAME`; `xtb.geomtrace` emits
  GEOMTRACE and TEVEC (the frame comment's energy) from the same frame so the
  two stay the same length. The status of the run is saved across the
  appending (`(exit $xtbStatus)`), since `determineStatus` reads `$?` next.
- **Exit status.** xtb exits 0 after "FAILED TO CONVERGE GEOMETRY
  OPTIMIZATION" (gensub `notConverged`, #614) and prints "normal termination
  of xtb" on stderr, which `2>&1` puts in the output (`normalTermination`).
- **Solvent names.** The ALPB list in `xtb --help` says `hexandecane`; the
  program accepts `hexadecane`. GBSA takes `h2o` and `n-hexane` (not `hexane`),
  and `dmf` and `n-hexane` only with GFN2 (checked by running every name);
  ai.xtb maps and refuses accordingly.
- **New property `HLGAP`** (HOMO-LUMO gap, eV, class NA) in
  `data/client/config/properties`, group Derived.
- **Stage 2/3 hooks.** Orbitals: `--molden` and a `.desc` entry in the
  style of `mopac.mo`/`ecceqm`; charges: `--pop` (Mulliken) and the
  CM5/Hirshfeld block; vibrations: `--ohess` and a Vibration runtype in
  `xTB.edml`. No `<Importer>`: a raw xtb output has no geometry, so an import
  needs the frames block or a companion xyz.

## Stage 2: orbitals (`--molden`)

- gensub adds `--molden` (never with `--gfnff`) and appends `molden.input` to
  the output between `ECCE-XTB-MOLDEN`/`ECCE-XTB-ENDMOLDEN`; `xtb.mo` emits
  MO/ORBENG/ORBOCC/MOAOORDER and the new `GTOBASIS` (one row per primitive:
  Z, l, exponent, coefficient, shell).
- The file's conventions, found by building an independent overlap matrix
  (`tests/parsers/xtb_trace_ps_test.py`, trace(P.S) = N, C'SC = 1):
  coefficients are for **unnormalised** primitives; d is **Cartesian**
  (no `[5D]`), order xx yy zz xy xz yz, each component unit-normalised, which
  is Gaussian's convention: `xTB.edml` sets `componentNormalized="true"` on
  its cartesian MOOrdering and `BasisFlatten::getoddNormalize` applies the
  sqrt(3); p is x y z. UHF gives ONE set with fractional
  occupations (no beta).
- A GFN-FF run has no molden file and nothing is stored.
- `ICalcUtils::slaterBasisConfig` falls back to `GTOBASIS`
  (`gtoBasisConfig`, primitive-norm factor so NORMP cancels) and reports
  `orthonormal=false` for it; only MOPAC's `SLATERBASIS` is orthonormal
  (`MoAoBasis` uses that flag). `tests/basisflatten/run_xtb_tests.py` gives
  trace(P.S) = N through the C++ (needs a build tree with this change).
- Never write `--` in an XML comment in an .edml: the parse aborts the code.

## Stage 3: charges and frequencies

- gensub appends `charges`, `vibspectrum` and `g98.out` between
  `ECCE-XTB-CHARGES`, `-VIBSPECTRUM` and `-G98` markers (after removing stale
  copies before the run). Run types: Vibration = `--hess` (the given
  geometry, so GEOMTRACE is the input xyz), GeoVib = `--ohess <level>`.
- `charges` goes to `MULLIKEN` (GFN-FF writes EEQ charges there too).
  `vibspectrum` lists all 3N modes with the rigid ones at 0.00, so
  `xtb.vibspec` gives VIBFREQ and VIBIR already aligned like ORCA's.
- **`g98.out` holds the unit mass-weighted eigenvectors, not Cartesian
  displacements** (cos = 1.0 against an eigendecomposition of xtb's own
  `hessian`; the Cartesian reading gave 0.98), to two decimals, and only the
  3N-6 vibrations. `xtb.vib` divides by sqrt(mass), renormalises (ORCA's
  convention) and pads leading zero modes. Its atoms are in xtbopt.log's
  frame, so there is no frame problem for the Builder (#312).
- Thermochemistry from the output: `:: zero point energy`, `| TOTAL
  ENTHALPY`, `| TOTAL FREE ENERGY`, and the `T/K ... T*S` row (ENTROPY is
  T*S, as for the other codes). ETHERM is not set: xtb prints no U.
- Parsed degenerate modes (methane) agree with the hessian only as a
  subspace, as expected; compare subspaces, not single vectors.
