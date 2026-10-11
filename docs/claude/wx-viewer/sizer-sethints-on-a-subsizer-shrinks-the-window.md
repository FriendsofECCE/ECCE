---
type: pitfall
title: "`SetSizeHints(window)` on a small sizer sizes the whole window to it: the Calculation Editor shrank to a title bar on every theory or run-type change (W5)"
area: wx-viewer
paths: ["src/apps/calced/CalcEd.C", "src/apps/calced/CalcEd.H", "tests/windows/framelog.ps1"]
issues: [254]
---
`wxSizer::SetSizeHints(win)` sets `win`'s minimum size (and size) from that
sizer. `CalcEd::fillSummary` called it on the theory/runtype details sizer
right after `Clear(true)`, so the frame was sized to an empty sizer (136x59
in `framelog.ps1`), and refilled by the next `refit()`. Every theory or
run-type change did this two or three times. Only the frame's own sizer
(`refit()`) may size the frame; the sub-sizer call is gone.

`CalcEd::UpdateHold` (Freeze, one `refit()` at the end, repaint of all
children) covers the several `update*Fields()` of one change. Test commands
`theory <label>` and `runtype <label>` in `ECCE_TEST_CALCED` drive it.
