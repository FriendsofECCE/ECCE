---
type: pitfall
title: "A tool frame kept hidden for a tab was mapped by ewxRaiseWindow (gtk_window_present) and covered the main window as an empty grey frame (#983)"
area: services
paths: ["src/wxgui/ewxClasses/ewxWindowUtils.C", "src/apps/builder/Builder.C", "src/wxgui/jms/ToolTabs.C", "tests/apps/one_process_test.py"]
issues: [983]
---
- `Builder::Create` -> `OnNewClick` -> `setContext` calls
  `ewxRaiseWindow(this)` ("raised for popup dialogs") before the frame was
  ever shown. On X11 that is `gtk_window_present_with_time`, which
  `gtk_widget_show`s the window; wxGTK's map callback then sets
  `m_isShown`, so wx believes the frame is shown too. Same for wxGTK's own
  `wxTopLevelWindow::Raise()` (also `gtk_window_present`): calling the
  virtual `Raise()` instead does not help before the tool is adopted.
- In a tab the frame's windows are in the page, so the mapped frame is an
  empty grey window on top of the Organizer (no menu bar, no content, no
  tab strip in a screenshot). Every later `ewxRaiseWindow` (invoke,
  `raiseAllWindows`) presented it again.
- Fix: `ewxRaiseWindow` returns at once for a top-level window that is not
  shown. All other callers show the window first.
- Check: `tabinfo` (one_process_test) reports `frame=mapped|hidden` from
  `gtk_widget_get_mapped`, not `IsShown()`, which the map callback also
  sets.
