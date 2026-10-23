---
type: pitfall
title: "TD-DFT options (#383): ORCA has no triplets-only switch, the two codes normalise coefficients differently, and the checker vocabulary must be rebuilt"
area: codereg
paths: ["scripts/codereg/orcatheory.py", "scripts/codereg/orcatheory.dlg", "scripts/codereg/ged16theory.py", "scripts/parsers/ai.orca", "scripts/parsers/ai.gauss16", "scripts/parsers/verifyinput", "include/tdat/ExcitedStates.H", "tests/tddft/run_tests.py"]
issues: [383]
---
The Theory Details dialogs of ORCA and Gaussian 16 have an "Excited States
(TD-DFT)" box (SCF and DFT categories only) with the shared keys
`ES.Theory.TDDFT.{Use,NStates,Spin,TDA}`. ai.orca writes `%tddft nroots N
[triplets true] tda true|false end`; ai.gauss16 writes `TD(...)` or `TDA(...)`
on the route card.

- **ORCA cannot be asked for triplets only.** `triplets true` adds them to the
  singlets (checked on 6.1.1: 5 + 5 states), and `singlets false` is a syntax
  error. The ORCA dialog therefore offers "Singlets" and "Singlets and
  triplets" only; Gaussian offers all three (`Singlets`, `Triplets`, `50-50`).
  The parser then returns 10 roots, labelled Singlet and Triplet.
- **Unrestricted references get no spin keyword** in either generator (no
  singlet/triplet split there).
- **Coefficient normalisation differs.** ORCA's printed c is normalised
  (weight = c^2, and its transitions carry a spin letter, "8a>9a"); Gaussian's
  restricted states are normalised to 2 sum c^2 = 1 (weight = 2 c^2, no
  letters). `ExcitedStates::weightPercent` applies that rule; the HOMO comes
  from the ORBOCC occupations, not from the transitions.
- **`tools/verify/build_vocabulary.py` must be re-run** after adding any
  keyword to a generator, or `tests/verify` fails with "the keyword
  vocabulary is out of date".
- `TDA(...)` as a route keyword is accepted by Gaussian's route parser tables
  but was not run here (g16 aborts at start-up in the agent sandbox); verify
  on a real g16 before relying on it.
