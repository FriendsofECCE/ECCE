---
type: rule
title: "`execout()` returns false on that exit status"
area: codereg
section: "The input checker (`scripts/parsers/verifyinput`, #148)"
paths: ["scripts/parsers/verifyinput", "src/apps/calced/InputVerifier.C", "src/apps/calced/CalcEd.C", "tests/verify"]
issues: [148]
---
Findings are `LEVEL|LINE|CHECK|MESSAGE` on stdout; exit 1 means at
least one `BAD`. **`execout()` returns false on that exit status**,
so `InputVerifier` deliberately ignores its return value and decides
from the parsed output instead — a deck with faults in it is the
script working, not the script failing.
