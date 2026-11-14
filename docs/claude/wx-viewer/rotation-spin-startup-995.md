---
type: pitfall
title: "Raising the hidden Builder realizes its rotation spinners before pane layout"
area: wx-viewer
paths: [src/apps/builder/Builder.C, tests/apps/builder_upgrade_test.py]
issues: [995]
---
`Builder::Create()` calls `OnNewClick()` to establish the initial blank
structure before `restoreSettings()` lays out the panes. That reaches
`setContext()`, whose unconditional `ewxRaiseWindow(this)` can present the
hidden GTK frame. Its first native layout squeezes the viewer's X rotation
spinner (`Builder::createRotators`, `p_xrot`), despite its explicit minimum
width. GTK logs `for_size smaller than min-size` for the entry and both
buttons once per process.

With `G_DEBUG=fatal-warnings`, the warning's backtrace goes through
`wxWindow::DoMoveWindow`, the rotation row's `wxWrapSizer`,
`ewxRaiseWindow`, `Builder::setContext`, `OnNewClick`, and `Builder::Create`.
Inspecting the resized wxWindow identifies it as `p_xrot`. Changing the
wrap sizer's last-item extension flag alone does not prevent the warning.

`setContext()` raises only an already-shown Builder; the normal app show
then happens after construction and pane layout. Keep the content-based
minimum widths on all three rotation spinners. The CTest-registered
`apps_builder_upgrade` also rejects this warning in the combined startup
stdout/stderr log.
