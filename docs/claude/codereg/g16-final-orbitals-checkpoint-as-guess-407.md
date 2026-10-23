---
type: pitfall
title: "Gaussian 16 final orbitals as the next job's guess (#407): the checkpoint travels as guess.chkin"
area: codereg
section: "Pitfalls found more than once (check for siblings)"
paths: ["data/client/cap/Gaussian-16.edml", "scripts/parsers/ai.gauss16", "src/dsm/edsiimpl/OrbitalGuess.C", "src/dsm/edsiimpl/TaskJob.C", "src/comm/commxt/Launch.C", "src/comm/commxt/JobStore.C", "tests/launch/g16_guess_test.py"]
issues: [407, 75]
---
Same path as ORCA ([#75](orca-final-orbitals-as-the-next-jobs-guess-75.md)),
with these differences:

1. **The checkpoint was never kept.** `%Chk=<title>.chk` was written into the
   run directory and left there. The deck now says `%Chk=g16.chk` (a fixed
   name, so the declared output can match it whatever the calculation is
   called) and `<RetainOutputFile>g16.chk</RetainOutputFile>` in the `.edml`
   makes `Launch` add it to `filesToTransfer` and `JobStore` store it. Only
   with "Create Checkpoint File" on; without it the menu item is disabled.
   `RetainOutputFile` names one file because Gaussian declares two auxiliary
   outputs (fort.7 too), unlike ORCA's `RetainAuxiliaryOutput`.
2. **The guess input is `guess.chkin`, read with `%OldChk`.** The deck gets
   `%OldChk=guess.chkin`, `%Chk=g16.chk` and `Guess=Read` in place of the
   other guess keyword. `%OldChk` keeps the staged copy intact and its name
   does not depend on the run directory, local or remote. The guess file
   name is per code now: `TaskJob::orbitalGuessFile()` returns the code's
   auxiliary input called `guess.*`.
3. **A duplicate shares the source's run directory** in the test (and by
   default), where A's `g16.chk` still lies. Gaussian prints
   `Initial guess from the checkpoint file: "g16.chk"` either way, so the
   test removes A's file from the run directory first: the 1-cycle restart
   then proves the staged `guess.chkin` was read.
4. Gaussian cannot project onto another basis set (ORCA does), and the
   checkpoint is binary of the same Gaussian major version only. A chk of a
   large job is large; it is stored with every run.
5. `tests/verify` has no fixture for it: the codes-agreement check runs real
   Gaussian on every fixture and Gaussian refuses a deck whose `%OldChk`
   file is absent.

Measured (Gaussian 16, water HF/STO-3G): 7 SCF cycles by default, 1 from
the duplicate's checkpoint, same energy. Test: `tests/launch/g16_guess_test.py`
(`--central` for a data server).
