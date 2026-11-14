---
type: map
title: "Tools as tabs (#983 stage 2): a tool stays a hidden wxFrame, its windows move into a notebook page"
area: services
paths: ["src/wxgui/jms/ToolTabs.C", "include/wxgui/ToolTabs.H", "src/apps/builder/Builder.H", "src/apps/builder/Builder.C", "src/apps/builder/BuilderWindow.C", "src/apps/organizer/CalcMgrApp.C", "src/wxgui/jms/InProcessTools.C", "tests/apps/one_process_test.py"]
issues: [983]
---
Switch: `ECCE_ONE_PROCESS=1` and `ECCE_LAYOUT=small` (`ToolTabs::enabled`).
Without both, nothing changes.

## Approach: reparent the client windows, keep the frame

Each tool is a wxFrame subclass whose code uses `this` as the parent of
every panel, toolbar and dialog, as the AUI managed window and as the
target of its event tables (the Builder: 7000+ lines). Turning the tools
into panels would touch all of that. Instead `ToolTabs::adopt` leaves the
frame alive and hidden and moves its client windows (not the menu bar,
status bar, floating panes or dialogs) and its sizer into a `ToolPage` in
the Organizer's `wxAuiNotebook`:

- `ToolPage::TryAfter` sends events nobody under it handled to the hidden
  frame instead of up to the notebook: event tables, `EVT_CHILD_FOCUS`
  (property panel focus), toolbar commands and their UI updates keep
  working. Events from the Organizer's own page still go up to its frame.
- The tool's `wxAuiManager` is `UnInit` and pointed at the page
  (`SetManagedWindow`); panes keep their info. `BuilderAuiManager::Update`
  moves panes made later (parent: the frame) into the page.
- The menu bar is detached from the tool and swapped onto the main window
  on tab change (`placeMenus`, A behaviour; `MenuPlacement::InTab` is the
  hook for drawing it inside the tab later). `MenuRouter`, pushed on the
  Organizer frame, sends menu commands whose id is on the active tool's
  menu bar to the tool's frame (else the Organizer would run its own
  handler for the same `wxID_SAVE`, `wxID_EXIT`, ...).
- A tab change sends the tool's frame a `wxActivateEvent`, so data notices
  go out under the tool's name (stage 1's acting tool).
- The tool overrides `Show`, `Raise`, `SetTitle` to select/retitle its tab,
  and `Destroy`/its destructor call `release(this, false)`, which gives the
  windows back to the frame before it is deleted.
- Open in Window = `release`: windows back, frame shown, Window > Move to
  Tabs added. The tab close cross is vetoed and becomes the tool's own
  `Close()` (its save question can still Cancel).

The Builder is adopted in its opener before its frame is first shown, so
the GL canvas is first realized inside the page. Open in Window reparents
a realized canvas.

## Known gaps

- The tool's status bar stays on the hidden frame (not shown in a tab).
- Dialogs and popups with the hidden frame as parent: placement on GTK.
- `wxEVT_CHAR_HOOK` bound on the tool frame does not fire in a tab (it goes
  to the Organizer's frame).
