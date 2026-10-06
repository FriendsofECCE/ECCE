---
type: rule
title: "Check what is INSIDE a section, not only that it exists"
area: codereg
section: "The input checker (`scripts/parsers/verifyinput`, #148)"
paths: ["scripts/parsers/verifyinput", "src/apps/calced/InputVerifier.C", "src/apps/calced/CalcEd.C", "tests/verify"]
issues: [148]
---
**Check what is INSIDE a section, not only that it exists.** Every
rule was about section presence and closure until a hand-typed `s`
in an NWChem deck passed all of them (reported live 2026-09-25). A
geometry known to be Cartesian must have three coordinates on every
line; that one rule catches the whole stray-character class in
Gaussian, ORCA and NWChem at once.
