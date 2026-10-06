---
type: rule
title: "Do not move the checker's automatic run into `enableLaunch()`/`enableAllFields()`"
area: codereg
section: "The input checker (`scripts/parsers/verifyinput`, #148)"
paths: ["scripts/parsers/verifyinput", "src/apps/calced/InputVerifier.C", "src/apps/calced/CalcEd.C", "tests/verify"]
issues: [148]
---
Do not move the automatic run into `enableLaunch()`/`enableAllFields()`
— those fire on every edit, and a check costs a WebDAV fetch of the
input file plus a process.
