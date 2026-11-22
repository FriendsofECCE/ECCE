---
type: pitfall
title: "A wxStaticText rewrapped from its own size events loops forever and hangs the tool (#1246)"
area: wx-viewer
paths: [src/wxgui/wxdialogs/DiffractionDialog.C, include/wxgui/DiffractionDialog.H]
issues: [1245, 1246]
---
The Diffraction pane's status line was a wxStaticText without
`wxST_NO_AUTORESIZE`, rewrapped (`Wrap(width)`) from its own `wxEVT_SIZE`.
Each `SetLabel`/`Wrap` resized the label, the size event queued the next
rewrap, and so on: the main loop never returned to idle, so the Builder
stopped answering its script hooks. It looked like a crash in the tests
(no output); gdb showed the main thread in `wxTextWrapper::Wrap` under
`DiffractionView::status`.

Rule: keep `wxST_NO_AUTORESIZE` on a wrapped label, take the wrap width from
the parent's size event, and return early when the text shown is unchanged.
