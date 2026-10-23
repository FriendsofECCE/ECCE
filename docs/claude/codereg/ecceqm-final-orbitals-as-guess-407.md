---
type: pitfall
title: "ECCE-QM final orbitals as the next job's guess (#407): ecceqm.qmorb travels as guess.qmorbin"
area: codereg
section: "Pitfalls found more than once (check for siblings)"
paths: ["data/client/cap/ECCE-QM.edml", "scripts/parsers/ai.ecceqm", "src/qm/output.cpp", "src/qm/main.cpp", "tests/launch/ecceqm_guess_test.py"]
issues: [407]
---
Same path as [NWChem](nwchem-final-orbitals-movecs-as-guess-407.md), but the
engine is ours, so the file format is too:

1. **The deck always says `write_orbitals ecceqm.qmorb`**; the edml's
   `RetainOutputFile` keeps it. The duplicate's deck adds
   `read_orbitals guess.qmorbin` (staged input, `guess.` prefix is what
   `TaskJob::orbitalGuessFile()` looks for).
2. **Plain text** (full C_a, and C_b for UHF), written only for a converged SCF.
3. **Same basis name, same atoms in the same order only.** Anything else is
   ignored with a note on stderr and the atomic-density guess is used;
   `run_scf` orthonormalises the stored orbitals against the new overlap and
   ignores a set with another electron count. Projection onto another basis
   would need the mixed overlap; not done, so the Calculation Editor says
   "needs the same basis set" for ECCE-QM, as for NWChem.
