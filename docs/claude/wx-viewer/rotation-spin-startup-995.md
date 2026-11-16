---
type: pitfall
title: "The hidden Builder's first layout gives the X rotation spinner 1 px"
area: wx-viewer
paths: [src/apps/builder/Builder.C, src/wxgui/ewxClasses/ewxWindowUtils.C, tests/apps/builder_upgrade_test.py]
issues: [995]
---
`Builder::Create()` calls `OnNewClick()` before `restoreSettings()` lays
out the panes; `setContext()` then raises the frame (`ewxRaiseWindow`),
and the viewer panel's first layout runs while it is about 1 px wide.
The rotation row's `wxWrapSizer` sizes `p_xrot` to 1 px, and GTK3 logs
`for_size smaller than min-size` for the entry and both buttons once per
process (backtrace: `DoMoveWindow` <- wrap sizer <- `ewxRaiseWindow` <-
`setContext` <- `OnNewClick` <- `Builder::Create`).

Do not defer or skip that raise: in one-process mode a tabbed tool's
frame is mapped by it (`services/tabbed-tool-frame-is-mapped-by-raise.md`),
and `apps_builder_upgrade` never saw the calculation open without it.
The rotation spinners are `RotationSpinCtrl`, whose `DoSetSize` never
goes below GTK's own minimum width (`ewxNativeMinWidth`); the narrow
panel clips it instead. `apps_builder_upgrade` fails on the warning.
