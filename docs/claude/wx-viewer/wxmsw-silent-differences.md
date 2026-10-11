---
type: pitfall
title: "wxMSW: 16-bit menu ids, no right-click selection, no manifest, text-mode 0x1A, a grey frame, no alpha by default, console windows from system()/popen()"
area: wx-viewer
paths: ["packaging/windows/ecce.rc", "src/apps/calced/CalcEd.C", "src/apps/organizer/CalcMgr.C", "src/dsm/edsiimpl/ChemistryTask.C", "src/dsm/edsiimpl/Resource.C", "src/wxgui/ewxClasses/ewxStyledWindow.C", "src/inv/wxinv/SoWxViewer.C", "src/apps/builder/Builder.C", "src/apps/builder/VizPropertyPanel.C", "include/util/Ecce.H", "src/tdat/chemistry/MoFragments.C", "src/comm/rcommand/RCommand.C", "src/comm/commxt/Launch.C", "src/util/command/CommandWrapper.C", "tests/windows/probe_win.py", "src/wxgui/wxdialogs/WindowShot.C", "src/apps/builder/NModePanel.C", "src/apps/builder/StructLib.C"]
issues: [133]
---
Each of these worked on GTK and failed silently on Windows:

- **Menu ids are 16 bits** (WM_COMMAND). A popup item with id 100000
  arrives as another id and its handler never runs; the quick basis menu
  did nothing. Keep menu ids below 32768.
- **The native tree does not select on right click.** A context menu built
  from the selection shows the previously selected item's menu. Select in
  the item-menu handler.
- **No manifest means classic common controls**: embossed disabled text,
  Windows 95 widgets, wx's generic progress dialog (painted by the busy
  main thread: black areas). `ecce.rc` includes `wx/msw/wx.rc` with
  `wxUSE_RC_MANIFEST`.
- **Text-mode streams end at 0x1A** and turn \n into \r\n. Binary files
  (JPEG thumbnails: 0x1A at byte 52) need `ios::binary` / "rb"/"wb".
- **A wxMSW frame is APPWORKSPACE grey**; editors with controls directly
  on the frame need the button face (`ewxStyledWindow::setStyles`).
- **The default GL pixel format has no alpha**, so Coin's depth peeling
  fell back to the screen door; ask for `WX_GL_MIN_ALPHA 8` on MSW too.
- **Keyboard focus does not follow a shown pane** (on GTK either), so viz
  focus taken only in `OnChildFocus` never came; see the
  property-overlay entry.
- **system() is cmd.exe** (no sh syntax, no extensionless scripts, exit
  code not wait status, a console window flashes): `Ecce::runCommand`,
  `Ecce::scriptCommand`.
- **popen() is cmd.exe in a new console**: a window per call. The MO
  diagram's five `symops` calls flashed five at every open of a calculation
  (and slowed the build past the progress dialog's 300 ms, so "Building MO
  Diagram" flashed too). `Ecce::readCommand` reads a command's output with
  no window; `shell=false` with `Ecce::winArg` skips cmd.exe for the
  dialog programs (passdialog, hostkeydialog, msgdialog).
- **`SetSize(-1, -1, -1, -1)` sizes a frame to its minimum.** A fresh
  profile has no saved size, and `restoreWindowSettings` passed -1s: GTK
  kept the constructor's 800x650, MSW opened the Organizer at 500x400.
  The Organizer now picks a first size from the display.
- **A client-DC blit into a default `wxBitmap` keeps alpha 0** where GDI
  drew (text, backgrounds), so the test hooks' PNGs (`snap`, `shot`) had
  icons only. `ecceWindowShot`/`ecceClientShot` blit into a 24-bit bitmap.
- **A `wxWrapSizer` reserves too many rows in its first layout** (the
  Vibrational Frequencies controls took ~100 px more than they used, and
  the plot said "Not enough room"); a second `Layout()` at the same size
  fixes it. A GL canvas shown after being hidden stays black until its
  parent is laid out and it is refreshed (Structure Library preview).

`tests/windows/probe_win.py` checks the first, second and fourth through
the apps' hooks; `tests/windows/winshot.ps1` photographs windows, which
needs the interactive desktop (an ssh session has none).
