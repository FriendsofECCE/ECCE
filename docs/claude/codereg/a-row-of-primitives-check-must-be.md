---
type: rule
title: "A \"row of primitives\" check must be anchored"
area: codereg
section: "The input checker (`scripts/parsers/verifyinput`, #148)"
paths: ["scripts/parsers/verifyinput", "src/apps/calced/InputVerifier.C", "src/apps/calced/CalcEd.C", "tests/verify"]
issues: [148]
---
**A "row of primitives" check must be anchored.** Counting numbers
found anywhere in the line accepts the *next shell's header*
(`S   1  1.00` holds two numbers), so a shell declaring five
primitives and listing three read as complete. Found by the suite,
not by inspection.
