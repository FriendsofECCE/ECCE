---
type: checklist
title: "Don't trust `ai.<code>`'s own comments for the `.frag`/`.param`/ `.basis` format"
area: codereg
section: "New-code checklist (gotchas found integrating ORCA, issue #38)"
paths: ["ESInputController.C"]
---
**Don't trust `ai.<code>`'s own comments for the `.frag`/`.param`/
`.basis` format** — verify against the actual C++ writers instead:
`ESInputController.C`'s `write_cs()`/`write_setup()`/
`write_gbsconfig()` (`CalcEd::write_setup` in particular — the real
key list, e.g. `Category`/`Theory`/`RunType`/`Charge`/
`ChemSys.Multiplicity`, plus whatever `GUIValues::dumpKeyVals()`
exports from the theory/runtype dialogs). A sibling code's own
scripts can be stale or written against slightly different
assumptions than what the GUI actually emits today.
