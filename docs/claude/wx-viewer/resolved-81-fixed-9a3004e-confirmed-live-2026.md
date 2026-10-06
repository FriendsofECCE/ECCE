---
type: pitfall
title: "A static `EVT_RADIOBOX` entry never reaches an `ewxRadioBox`'s panel; Bind on the widget (#81)"
area: wx-viewer
section: "wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls"
issues: [81]
paths: ["src/wxgui/ewxClasses/ewxRadioBox.C", "src/apps/builder/NModePanel.C"]
---
**A static `EVT_RADIOBOX` table entry never arrives from an
`ewxRadioBox`.** `ewxRadioBox::Create()` pushes an `ewxHelpHandler`, so a
command event's route from the control to its panel is not wx's plain
propagation; the same holds for any ewx control with a pushed handler
chain. Bind on the widget itself (`radbox->Bind(wxEVT_RADIOBOX, ...)`), as
`NModePanel` does for its Animation/Vector and Graph/Table boxes. Without it
the Vibrational Frequencies panel's mode switch, row swap and Play button
never reacted.

`NModePanel` also sets `wxWS_EX_PROCESS_UI_UPDATES` on the box: wxGTK sends
`wxUpdateUIEvent` to ordinary child controls during idle only with that
style, so an `EVT_UPDATE_UI` fallback without it is inert.
