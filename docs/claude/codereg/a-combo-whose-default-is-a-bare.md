---
type: pitfall
title: "A combo whose default is a bare integer opens BLANK when the list is built conditionally"
area: codereg
section: "Pitfalls found more than once (check for siblings)"
paths: ["tests/dialogs"]
issues: []
---
**A combo whose default is a bare integer opens BLANK when the list is
built conditionally.** `wx.Choice.SetSelection()` ignores an
out-of-range index without complaint, so the control shows nothing
selected and `GetValue()` returns `""` — which every generator
translates to no keyword at all. Same silent-emission class as a
dialog string that doesn't match the generator's, same cause: one
list, several lengths, a default written against the longest. Found
in every Gaussian runtype dialog at once
(`ES.Runtype.GeomOpt.InitialHessian`, `default = 1` against a
one-entry list for all but a handful of theories) — and the same file's
`CheckDependency()` already carried the `len()==2` guard the
constructor was missing. Always write these as
`choices.index("Name")`, never an integer; `xcFuncDefault` is the
same hazard (a stale `36` was selecting Mod. Perdew-Wang 1K where
B3LYP was intended). `tests/dialogs` now fails on any combo left with
no selection, in every category/theory/runtype context.
