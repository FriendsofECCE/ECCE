---
type: pitfall
title: "Gaussian 16 final orbitals without the checkpoint (#690): results.h5 -> guess.cardsin, Guess=Cards"
area: codereg
paths: ["src/dsm/edsiimpl/OrbitalGuess.C", "src/tdat/chemistry/ResultsFromCalc.C", "include/tdat/ResultsFromCalc.H", "scripts/parsers/ai.gauss16", "data/client/cap/Gaussian-16.edml", "src/dsm/edsiimpl/TaskJob.C", "src/comm/commxt/JobStore.C", "src/wxgui/wxdialogs/GlobalPrefs.C", "tests/launch/g16_guess_test.py"]
issues: [690, 407]
---
When the source has no `g16.chk` but `ResultsFromCalc::stored()` gives
orbitals, `OrbitalGuess::attach` writes `guess.cardsin` (second guess input
in the .edml; `TaskJob::orbitalGuessFiles()`/`attachedOrbitalGuessFile()`)
and ai.gauss16 writes `Guess=Cards` plus `@guess.cardsin/N` after the basis.
The checkpoint wins when both exist. Traps, each found against Gaussian:

1. **AO order: no table.** `ResultsFromCalc::codeColumns` inverts
   `convert()` with the same overlap match (`matchShells`) against the
   calculation's own flat basis, so the columns come back in the Props'
   order and normalisation, which for G16 is what Punch=MO wrote (6D/10F,
   SP shells and all).
2. **A blank line before the include is read as the format line**
   ("read in format ." then a crash in l401). A route-card basis leaves an
   empty `##basis##` line; `cleanup` collapses the run of blank lines.
3. **A `0` header line ends the alpha set.** The header number names the
   orbital to replace, so without it the beta orbitals overwrite the alpha
   ones: the run still says "Replacement beta orbitals read" but takes the
   full number of cycles. Punch=MO writes the `0`; so must we.
4. Cards are never projected: the editor line asks for the same basis set
   and the `orbitalguesssource` atoms prop for the same atoms.

Opt-in Preferences > Data switch `DiscardBinaryOutputs` (off by default):
the job store removes `g16.chk` after `store()` when `stored()` accepts the
file. Measured (G16, 6-31G*): see the test; water RHF and O2 UHF triplet
reach the source energy in 1 cycle.
