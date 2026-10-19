---
type: pitfall
title: "smallscreen_test: dialogs open on the real Wayland screen; wxDisplay on Wayland has no work area; the splitter hides the periodic table's width (#337, #351)"
area: wx-viewer
paths: ["tests/apps/smallscreen_test.py", "scripts/codereg/templates.py", "src/apps/basistool/WxBasisTool.C", "src/apps/builder/PlotAnyPropertyPanel.C"]
issues: [337, 351, 335]
---
- **Every dialog reported "no window to measure"** because the test started
  `scripts/codereg/*.py` with `DISPLAY` set but not `GDK_BACKEND=x11`. On a
  Wayland desktop GTK3 ignores `DISPLAY` and opens the dialog on the real
  screen (the app wrappers force x11; the test did not). The test now sets
  `GDK_BACKEND=x11` and drops `WAYLAND_DISPLAY`. Any new test that launches a
  wx program directly needs the same.
- Windows are told apart by X window id, not by (title, size, position):
  successive dialogs are identical in all three, so one that had not yet
  disappeared hid the next, and a dialog that opens nothing is now a failure.
- `wxDisplay::GetClientArea()` equals the full geometry on native Wayland
  (no panel is subtracted); under x11/XWayland it subtracts the 32 px GNOME
  bar. `fitToDisplay` therefore only caps correctly on x11, which is what the
  wrappers force. `EccePanel.TitleReserve` is 56 so a dialog ending a few pixels
  short of the screen still has room for decorations.
- A codereg frame has no resize border; `EccePanel.FitToScreen` adds it when
  the content has to scroll, so the user can make room (#351).
- `wxSplitterWindow` does not pass its panes' minimum widths up to the window:
  the Basis Set Tool's window was narrower than list + 18 periodic-table
  columns and cut the right-hand elements off. `WxBasisTool` adds the width
  of the top pane to the window minimum, as it already did for the height.
- A plot with a large `SetMinSize` keeps its height and clips the rows below
  it in a short pane; `PlotAnyPropertyPanel` uses a small minimum so the plot
  shrinks first. `wxPlotCtrl` shows no title unless `SetShowPlotTitle(true)`.
