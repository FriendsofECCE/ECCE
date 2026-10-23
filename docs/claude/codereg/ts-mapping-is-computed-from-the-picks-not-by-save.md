---
type: pitfall
title: "TS Search atom mapping (#464): computed from the picks when they are known, not only by a Save that writes the input"
area: codereg
paths: ["src/apps/calced/CalcEd.C", "src/apps/calced/QstStructures.C", "src/apps/calced/ESInputController.C", "tests/apps/calced_ts_reopen_test.py"]
issues: [464, 419]
---
The status line under the TS Search pickers ("Matched 3 atoms ...") and the
text of 'Show mapping' live only in CalcEd members (`p_qstStatus`,
`p_qstDetail`, `p_qstTable`); nothing stores them with the calculation.
They used to be filled only by `write_qst()`, which runs inside
`generateInput()`, which runs only when `isReady()` (a basis set that covers
the molecule) and, from Save, only when the editor is MODIFIED.
`loadTsKeys()` cleared them on every open. So:

- without a basis set, Save wrote nothing and the line kept saying "Save to
  match the atoms";
- a reopened or Organizer-duplicated calculation (an exact copy, state Ready)
  opened unmodified, so Save was a no-op and the mapping never appeared until
  some edit, typically re-picking the basis set, made Save regenerate the input.

The loaded basis set was never the problem: `p_basis` is read from
`gbsConfig()` on open and `isReady()` accepts it; Launch was on in both cases.

`tsMatchStructures()` runs the same AtomMap matching as the writers with
nothing written, from the editor's picks; CalcEd calls it in `loadTsKeys()` and
on every pick. Anything else shown from a generator side effect has the same
shape: it is empty on open until a modified, ready Save.
