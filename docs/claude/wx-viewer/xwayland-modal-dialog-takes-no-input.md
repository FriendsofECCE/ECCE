---
type: pitfall
title: "On GNOME/Wayland (XWayland, Mutter 48) a modal dialog can show typed text or scrolling only after it is reopened; the busy cursor and window disabler are not the cause, every modal now goes through ewxModalGuard"
area: wx-viewer
paths: ["src/wxgui/ewxClasses/ewxModalGuard.C", "include/wxgui/ewxModalGuard.H", "src/wxgui/ewxClasses/ewxApp.C", "src/wxgui/ewxClasses/ewxWindowUtils.C", "src/apps/organizer/CalcMgr.C", "src/wxgui/wxdialogs/WxDavAuth.C", "tests/apps/modal_guard_test.py"]
issues: [120, 311, 332]
---
Seen only on GNOME/Wayland (ECCE runs under XWayland, `GDK_BACKEND=x11`),
never on X11: the Organizer's New Calculation / New GROMACS MD Study name
field shows nothing typed (Enter still applies the name, #311); the first
Verify dialog after a Builder save does not scroll, the second does (#332);
the login dialog took no keys (#120).

What the wx 3.2.8 GTK source rules out (src/gtk/dialog.cpp, cursor.cpp,
common/utilscmn.cpp):
- `wxDialog::ShowModal` on GTK uses a GTK grab, no `wxWindowDisabler`.
  `wxWindowDisabler(win)` disables the other shown top-level windows only;
  the dialog and a skipped parent stay sensitive.
- `GTKUpdateCursor` gives a window whose top-level is modal no busy cursor.
- Rename (`OnRenameClick`) opens the same `ewxTextEntryDialog` under the
  same `startDisabler()` and works; New GROMACS MD Study goes through the
  same `OnNewResourceClick` -> `createResource` as New Calculation, which
  already lifts `p_disabler`/`p_cursor` (9.1.0), and still fails.
So which dialog fails is not decided by the busy cursor or disabler.

What is known from #120 (gdb on beryllium): Mutter declines to activate a
window mapped with a stale timestamp; `gtk_window_present_with_time()` with
`gdk_x11_get_server_time()` fixes that layer (`ewxRaiseWindow`). A second
layer stayed open there: focus correct at every X11/GTK level, still no
visible effect of keys. Whether that is input not delivered or input not
repainted is what `ECCE_DEBUG_MODAL` now records.

`ewxModalGuard` (a `wxModalDialogHook`, installed by `ewxApp::OnInit`) runs
for every modal dialog of every ewxApp program, including wxTextEntryDialog
and the native GTK message and file dialogs (wxFileDialog enters twice):
- ends all busy cursors while the dialog is up and restores the count
  after (`wxBusyCursorSuspender` is a no-op on GTK when no global cursor
  was set, so it cannot be used);
- after the dialog is shown, `ewxRaiseWindow()` it (the #120 layer-1 fix
  for every dialog, not only the login);
- with `ECCE_DEBUG_MODAL=<file>` (also set by `ecce --bug`, `modal.log`):
  `enter` (busy, parent shown/enabled/frozen, disabled top-levels,
  `GTK_IM_MODULE` and GTK's `gtk-im-module` setting, which the desktop can
  set through XSETTINGS), `present`/`settled` (mapped, active,
  toplevel focus, focus widget, frame counter), one `input` line per key,
  scroll or click that reached the dialog with its repaint (`draw`) count,
  and `exit` totals. Keys counted but `draws` flat = GTK stopped painting
  (frame clock waiting on the compositor); draws rising with nothing on
  screen = the compositor; no `input` lines = input not routed to the window.

Not covered: the wxPython Details dialogs (`scripts/codereg/*.py`) and
`ecce-dialog` (own wxApp). Xvfb has no compositor or window manager, so
`tests/apps/modal_guard_test.py` checks only the bookkeeping (busy cursor
off while up, back after, parent enabled, dialog presented).
