---
type: pitfall
title: "Generic wx dialogs (choice, multi-choice, text/number entry) size to the message's longest line: break it with \\n"
area: wx-viewer
paths: ["src/apps/builder/DensityFragments.C", "src/apps/builder/NModePanel.C", "src/apps/organizer/CalcMgr.C", "src/wxgui/wxdialogs/GlobalPrefs.C"]
issues: [905]
---
`wxMultiChoiceDialog`, `wxSingleChoiceDialog`, `wxTextEntryDialog`,
`wxNumberEntryDialog` and the `wxGet*FromUser`/`wxGetSingleChoice*` helpers
lay the message out with `CreateTextSizer`, which never wraps: a 190-character
sentence gives a dialog about 1250 px wide. Write the message as lines of at
most ~65 characters joined with `\n`. `wxMessageBox`/`wxMessageDialog` are
native on GTK and MSW and wrap by themselves. A long `wxStaticText` in a
hand-made dialog needs `Wrap(px)` (or `\n`) for the same reason. To find
offenders, scan the dialog calls for message literals with a line over
70 characters.
