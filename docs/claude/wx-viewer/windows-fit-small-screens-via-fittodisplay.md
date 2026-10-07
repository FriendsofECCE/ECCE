---
type: rule
title: "Every ewxFrame/ewxDialog is capped to its display by `fitToScreen()`; codereg dialogs scroll in `EccePanel` (#189)"
area: wx-viewer
paths: ["src/wxgui/ewxClasses/ewxWindowUtils.C", "scripts/codereg/templates.py", "tests/apps/smallscreen_test.py"]
issues: [187, 189]
---
**`ewxFrame::Show()` and `ewxDialog::Show()/ShowModal()` call
`ewxWindowUtils::fitToDisplay()`; do not add per-window size caps.** It
caps the client size to `wxDisplay::GetClientArea()` minus the non-client
height and a 36 px title-bar reserve (a bare X server shows no title bar and
the menu bar cannot be told from a decoration in `GetSize() - GetClientSize()`),
keeps the window on screen, and when the content's natural size (sizer min
size, or an explicit minimum the app set) does not fit, moves it into a
scrolled window above a fixed row. The row is the trailing button row (plus
up to three items under it), or the sizer passed to `fitToScreen(row)`.

- The scroller's natural size comes from the content sizer, clamped to
  what is left above the fixed row (#187 lesson 2); the scrolled item is
  proportion 1 (#187 lesson 1).
- A window can leave the display after it is shown (size restored, content
  loaded), so a `wxEVT_SIZE` handler re-runs the fit, at most three times
  per episode so a window that cannot obey is not chased.
- An app that grows the window itself (`CalcEd::update*Fields()`) calls
  `fitToScreen()` after `SetSizeHints(this)`; the hint alone sets a
  minimum above the display.
- codereg dialogs: `EccePanel` is a `ScrolledPanel`; the message box and
  buttons are in a footer owned by the frame, so they never scroll. Call
  `panel.Fit()` (overridden) when the content changes.
- `tests/apps/smallscreen_test.py` (ctest `apps_smallscreen_*`) opens
  every app and every codereg theory/runtype dialog on Xvfb at 1024x768 and
  1024x600 and fails for a window that, with the title-bar reserve, ends
  past the screen.
