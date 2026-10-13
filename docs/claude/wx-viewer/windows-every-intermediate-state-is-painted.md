---
type: pitfall
title: "wxMSW paints every intermediate state: panes before AUI places them, menu bars item by item, a disabled active window hands the activation away"
area: wx-viewer
paths: ["src/apps/builder/Builder.C", "src/apps/builder/BuilderPanels.C", "src/apps/builder/BuilderApp.C", "src/apps/builder/MoPanel.C", "src/apps/organizer/CalcMgr.C", "src/wxgui/jms/WxJMSMessageDispatch.C", "tests/windows/framelog.ps1", "tests/windows/redraw_win.py"]
issues: [254, 255, 256, 296]
---
GTK draws once per idle; wxMSW shows each step of a change. Found with
60 fps recordings, checked with `tests/windows/redraw_win.py`.

- **`wxAuiManager::Update()` shows a pane's window before it lays the
  frame out**, so a pane appears for a moment wherever its window was: a
  never-placed window sits at the frame's (0,0), over the viewer.
  `updatePanes()` moves such windows off the frame first; property panels
  are created hidden (`Hide()` before `Create()`).
- **`Freeze()` of a top-level window is a no-op on MSW** (wx freezes its
  children instead), and a window shown while frozen is shown before it is
  frozen. `BuilderRedrawHold` (MSW only) holds the Builder's children
  while a calculation opens or a property panel changes.
- **The menu bar is not one of the frame's children**: wxMSW redraws it
  for every item appended or deleted (half a menu bar on screen). Freeze the
  `wxMenuBar` itself while menus are rebuilt; `wxMenuBar::Refresh()` skips
  while frozen. The Organizer redraws the bar after the hold only when a
  top menu changed state, and sets items and tools only when their state
  changes (#296; the title flash this addresses was never checked on
  Windows here).
- **Disabling the active window (`wxWindowDisabler` with no window to
  skip) hands the activation to the window behind it**, another program;
  a prompt then opens behind that, and the tree loses its focus and
  repaints its selected rows. The Organizer's `startDisabler()` skips the
  Organizer; a modal dialog disables it itself.
- **`internalInvokeMCB()` shows every registered top shell before the
  app's own `invokeMCB()`**. A Builder started for a tool request is
  registered (`registerTopShell`) only when its invoke has set the
  calculation, so it is first shown complete.

- **New calculation (W2)**: `createResource()` holds the Organizer's redraws
  (`FrameHold`: freeze, thaw, `RedrawWindow(RDW_ALLCHILDREN)`), or the tree
  collapsing and re-expanding and an empty summary pane were painted; the
  Organizer's menus are rebuilt once per settled selection (`CallAfter`).
- **Builder (W3, W10)**: `BuilderRedrawHold` repaints every child when the
  outermost hold is released (`RDW_ERASE | RDW_ALLCHILDREN`), and
  `loadPaneLayout()` refills the Atom and Residue tables of the layout it
  restores (the Atom Table came back empty after the Structure Library).
- **Popups (W4)**: `TearableContent::Popup()` repaints at once on MSW.
- **Progress box (W8)**: a stage with only a title (a `freq` line with no
  fields) was zero rows high and the next title was drawn over it; every
  stage takes at least one row.

Measuring: `framelog.ps1` samples in C# (PowerShell took up to 0.5 s per
sample) and must not call `GetWindowText` (it waits for the busy window, the
very moment worth seeing). The idle test PC's blank screen saver makes
every `CopyFromScreen` fail ("The handle is invalid"); `redraw_win.py`
ends it first.
