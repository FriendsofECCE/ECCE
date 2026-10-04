---
type: rule
title: "blank, not green"
area: codereg
section: "The input checker (`scripts/parsers/verifyinput`, #148)"
---
An empty finding list means *nothing was checked*, and the lamp goes
**blank, not green**. A light that cannot distinguish "looked and
found nothing" from "did not look" is the first one a user believes.
The dialog smoke test asserts this; it caught the bug once already.
