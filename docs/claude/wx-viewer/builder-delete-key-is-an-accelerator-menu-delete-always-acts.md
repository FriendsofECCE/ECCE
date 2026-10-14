---
type: pitfall
title: "Builder Delete: the bare key is a separate accelerator id; a read-only field (the Log) never takes it; a press in the 3-D view takes focus"
area: wx-viewer
paths: [src/apps/builder/Builder.C, src/inv/wxinv/SoWxRenderArea.C, src/wxgui/ewxClasses/ewxLogTextCtrl.C]
issues: [324]
---
wxGTK tries the frame's accelerator table before the focused control sees a
key, so Delete reaches `Builder::deleteKey` (id `ID_ACCEL_DELETE`) from any
focus. It edits an editable, enabled text field and otherwise deletes the
selection. Edit > Delete Selection and Ctrl+D are `wxID_DELETE`
(`OnDeleteClick`) and always delete; treating them like the key made both do
nothing with the read-only Log focused. A GL canvas does not take focus on a
click by itself, so `SoWxRenderArea::OnMouse` sets it on a button press.
`ECCE_DEBUG_PANELS` (set by `ecce --bug`) logs the atom/bond counts a delete
acts on. `ewxLogTextCtrl::DoLogTextAtLevel` receives text already prefixed
with time and level by wxLogFormatter, so it adds none.
Script hook: `delete <menu|key> <viewer|log|text>` (tests/apps/builder_edit_test.py).
