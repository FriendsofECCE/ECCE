---
type: map
title: "xtb (xTB) is registered like MOPAC/CP2K: a one-file deck split by gensub, geometries appended to the output, orbitals/charges/vibrations still to come (#362)"
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
