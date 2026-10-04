---
type: rule
title: "A hand edit is the case the checker most needs to see and the one it missed"
area: codereg
section: "The input checker (`scripts/parsers/verifyinput`, #148)"
---
**A hand edit is the case the checker most needs to see and the one
it missed.** `processEditCompletion()` wrote Final Edit's result
back to DAV and nothing re-checked it. Also: the Verify button must
**not** `doSave()` first — that regenerates the input file and
silently destroys the user's hand edit, then reports the clean
regenerated deck as fine.
