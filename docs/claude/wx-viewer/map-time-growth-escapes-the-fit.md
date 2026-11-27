---
type: pitfall
title: "A window grows when GTK maps it, after `fitToDisplay()` has clamped it; the `wxEVT_SIZE` re-fit that catches this is a race"
area: wx-viewer
paths: ["src/wxgui/ewxClasses/ewxWindowUtils.C", "src/apps/calced/CalcEd.C", "tests/apps/smallscreen_test.py"]
issues: [189]
---
`ewxFrame::Show()` fits and `keepOnDisplay()`s the window *before*
`wxFrame::Show()` maps it. GTK then gives the toplevel its own natural
height from the size hints the sizer set, so a window centred at its
pre-map size can end up taller, and below the display, after the clamp has
already run. What normally puts it back is the `wxEVT_SIZE` watcher in
`ewxWindowUtils.C`, and that is a race: it only fires while the window
reports itself shown, and gives up after three tries or when the size
matches the one a window manager refused.

Seen in the Calculation Editor at 1366x768: 570x613 at +400+120, one pixel
of the title-bar reserve below the screen, failing `apps_smallscreen_*`.
The same binary passed on Xvfb, where the watcher won the race; the
failure reproduces by disabling the watcher. An app that is centred and
then grows should clamp its own position after `Show()`
(`CalcEd::Show()` does, with `CallAfter`), rather than rely on the watcher.
