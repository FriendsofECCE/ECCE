# tests/teaching -- introductory chemistry calculations, end to end

Runs a set of small NWChem calculations a first-year lab would run, through
the same path a student uses, and checks the chemistry against known answers.

```
tests/teaching/run_tests.py --build build            all 21 calculations
tests/teaching/run_tests.py --jobs 4                 four at a time
tests/teaching/run_tests.py --group B                one set
tests/teaching/run_tests.py --case co --case hf -v   named cases, every check shown
tests/teaching/run_tests.py --list
tests/teaching/run_tests.py --show-decks --case h2se print the generated decks
tests/teaching/run_tests.py --transport direct       set ECCE_TRANSPORT for the ECCE processes
```

CTest: `ctest -R teaching` (SKIP, exit 77, without NWChem, csh, apache2, java).
It needs the `launchjob`, `eccejobmaster`, `eccejobstore` and `ecmd` targets.

## What is exercised

1. `deckgen.py` makes the `.frag`/`.basis`/`.param` CalcEd would write and
   runs the real `scripts/parsers/ai.nwchem` over a copy of `nwch.tpl`.  The
   settings a student does not touch come from `nedtheory.py` and
   `nedruntype.py` run headlessly once each (tests/dialogs/harness.py), as
   CalcEd does, so a changed dialog default changes the deck.
2. `tests/launch/harness.py` starts an isolated data server and broker
   (own state `~/.cache/ecce-teach-state`, ports 8596/8588) and
   `launchjob` creates and launches each calculation on `localhost`/Shell
   through the real `Launch`; eccejobmaster/eccejobstore/eccejobmonitor run
   as in a session.
3. Checks read `Props/` from the data server and the raw `ecce.out` trace
   from the run directory, which NWChem wrote and ECCE did not touch.

Every job must: reach `completed`; store TE, GEOMTRACE, MO, ORBENG, ORBOCC,
ORBSYM; have a GEOMTRACE last frame equal to NWChem's final geometry, a TE
equal to its last energy, and an ORBENG equal to the last orbital set it
printed (the reprint after optimisation, #198); and converge its optimiser.

## Rules used for the MO order

Orbitals come from ORBENG/ORBOCC/ORBSYM.  NWChem's autosym labels a linear
molecule in a finite subgroup (C4v here), so the pi pair is labelled `e`;
degeneracy (energies within 1e-4 Eh) is what is tested, the labels are
reported.  After the 1s cores, the occupied valence orbitals are listed as
(degeneracy, occupation) in energy order, followed by the lowest empty group:

| molecule | pattern | meaning |
| --- | --- | --- |
| C2 | 1, 1, 2(pi) then empty 1 | pi(2p) below sigma(2p) |
| O2 (ROHF) | 1, 1, 1, 2(pi), 2(occ 1) then empty 1 | sigma(2p) below pi(2p), two half-filled pi* |
| CO | 1, 1, 2(pi), 1 then empty 2 | 5sigma HOMO above 1pi |
| HF | 1, 1, 2(pi) then empty 1 | |

pi orbitals of planar molecules are counted as occupied orbitals whose
irrep is odd under the molecular plane (`analysis.py`, per point group,
with the plane found from the final coordinates); linear molecules use the
degenerate pairs.

## Notes

* Starting structures are in `geometry.py`; the SF4 and ClF3 isomer starts
  are nudged by a few hundredths of an angstrom so the optimiser is free to
  leave a symmetric stationary point.
* Basis sets are given as the Basis Set Tool's library names, per element
  (`deckgen.basis()` writes what `TGBSConfig::dump("NWChem")` does).  Pople
  sets are Cartesian in ECCE.
* The summary table at the end lists settings, status, total energy and the
  key numbers; `-v` prints every check.
