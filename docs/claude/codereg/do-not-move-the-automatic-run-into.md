---
type: rule
title: "- Do not move the automatic run into `enableLaunch()`/`enableAllFields()` — th"
area: codereg
section: "The input checker (`scripts/parsers/verifyinput`, #148)"
---
Do not move the automatic run into `enableLaunch()`/`enableAllFields()`
— those fire on every edit, and a check costs a WebDAV fetch of the
input file plus a process.
