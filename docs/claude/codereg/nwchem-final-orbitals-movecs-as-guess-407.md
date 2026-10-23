---
type: pitfall
title: "NWChem final orbitals as the next job's guess (#407): nwchem.movecs travels as guess.movecsin"
area: codereg
section: "Pitfalls found more than once (check for siblings)"
paths: ["data/client/cap/NWChem.edml", "scripts/parsers/ai.nwchem", "scripts/parsers/nwch.tpl", "tests/launch/nwchem_guess_test.py"]
issues: [407, 75]
---
Same path as [G16](g16-final-orbitals-checkpoint-as-guess-407.md):

1. **The deck fixes the vectors file name.** `InitialGuess` in `ai.nwchem`
   appends `output nwchem.movecs` for SCF and DFT (not NWPW, whose
   `vectors` line has other keywords), so `<RetainOutputFile>nwchem.movecs`
   matches whatever the calculation is called. NWChem's default would be
   `<title>.movecs`.
2. **The guess input is `guess.movecsin`:** the deck gets
   `vectors input guess.movecsin output nwchem.movecs`.
3. **Same basis set only.** A different basis needs
   `vectors input project <old basis tag> <file>`; that is not written, so a
   duplicate with other orbitals/basis is not supported for NWChem.
4. The file is binary (NWChem movecs, same build/word size).
