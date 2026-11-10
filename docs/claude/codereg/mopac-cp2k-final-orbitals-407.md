---
type: pitfall
title: "MOPAC and CP2K final orbitals: density staging names and restart retention (#407)"
area: codereg
paths: ["data/client/cap/MOPAC.edml", "data/client/cap/CP2K.edml", "scripts/parsers/ai.mopac", "scripts/parsers/ai.cp2k", "src/comm/commxt/Launch.C", "src/dsm/edsiimpl/OrbitalGuess.C", "src/apps/calced/CalcEd.C", "tests/launch/mopac_guess_test.py", "tests/launch/cp2k_guess_test.py"]
issues: [407]
---
Both follow the stored auxiliary-output / distinct `guess.*in` input path
used by NWChem and ECCE-QM. Each extension needs both MIME tables.

- MOPAC requires `DENOUT` to write `mopac.den`. The duplicate stores it as
  `guess.denin`, but `OLDENS` reads the basename of the actual input file:
  `mopac.mop` needs `mopac.den`. Launch renames the binary download and uses
  the same name in its transfer list, before the local or remote run.
  The editor checks `OLDENS`, rather than the stored guess filename, to
  decide whether the deck reads the guess. Same element sequence only;
  the Hamiltonian may change.
- MOPAC's `SCF CALCULATIONS` summary counts whole SCF calls, not iterations.
  The launch test adds `PL` to expose iterations. Its energy comparison is
  the heat of formation in kcal/mol, printed to five decimal places, with
  a tolerance of 0.0001 kcal/mol.
- CP2K's default wavefunction filename follows `PROJECT`, which comes from
  the calculation title. `SCF/PRINT/RESTART` explicitly names
  `=cp2k-RESTART.wfn` so `RetainOutputFile` can find it for every title.
  The duplicate reads `guess.wfnin` through `DFT/WFN_RESTART_FILE_NAME` and
  `SCF_GUESS RESTART`.
- CP2K chooses its basis in Theory Details, outside `TGBSConfig`.
  `OrbitalGuess::attach` records those basis settings (including the six
  per-element override rows) and the atom sequence for the editor's live
  compatibility note. CP2K needs the same basis and element sequence.

Tests use the real programs, remove the source restart from the run tree
before launching the duplicate, and check binary contents, iteration counts,
energies, and compatibility notes. `--central` exercises WebDAV transfers.
