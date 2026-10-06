---
type: rule
title: "Verify keywords, basis names and output formats by running the code, not by reading its manual"
area: codereg
section: "Pitfalls found more than once (check for siblings)"
paths: ["scripts/codereg", "scripts/parsers", "tests/basisload/nwchem_library_check.py", "tests/verify"]
issues: []
---
**Verify keywords, basis names and output formats by running the code,
not by reading its manual, and run a real job, not only an input check.**
Manuals and the code disagree often enough that a table written from
documentation ships decks that fail, or silently compute something else,
after reaching a queue. Examples, each in its own entry:

- NWChem rejects functionals and dispersion spellings it documents:
  [keyword sweep](verify-a-codes-keyword-list-by-running.md).
- ORCA's input check accepts double hybrids that then die without an
  auxiliary basis: [accepted is not working](the-code-accepted-the-keyword-is-not.md).
- Basis names: ORCA takes `6-31++G**` but not `6-31++G`; Gaussian's
  `def2SVPP` is a smaller basis: [basis-set writers](the-basis-set-writers-decide-how-a.md).
- The input checker therefore checks deck shape, never keyword validity:
  [shape, not keywords](it-checks-the-shape-of-a-deck.md).
- Where the code ships its own parser, use it as the test oracle:
  [Gaussian `testrt`](gaussian-ships-testrt-its-own-route-card.md).
