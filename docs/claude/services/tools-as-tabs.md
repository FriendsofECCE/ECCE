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
  hook for drawing it inside the tab later). Binds on the
  Organizer frame (`routeMenus`) send menu commands whose id is on the active tool's
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

- The status bar moves into the page below a `body` panel (the AUI
  managed window, so panes are not laid over the bar) and back on release.
- GTK sends `wxEVT_CHAR_HOOK` to the top-level window: a handler on the
  Organizer's frame passes it to the active tool's frame first.
- The hidden frame must never be mapped: `ewxRaiseWindow` skips hidden
  windows (tabbed-tool-frame-is-mapped-by-raise.md).

## Calculation Editor (second tool adopted)

`CalcEdWindow.C` adopts the editor before its first Show (no AUI manager,
no status bar). CalcEd overrides `Show/Raise/SetTitle/Destroy/GetMenuBar`
as Builder does, plus three things a frame's own code takes for granted
and that stop working once its windows are in the page:

- `GetSizer()` is null (the sizer moved): `CalcEd::mainSizer()`
  (`ToolTabs::sizer`). Panel sizers (`p_chemSysInner->GetSizer()`) stay.
- `FindWindow(id)` on the frame no longer finds its controls: `CalcEd::
  FindWindow(long)` hides the base one and searches `ToolTabs::client`.
  Without it `populateSpinMultiplicities` dereferenced null (SIGSEGV on open).
- `Layout()` of the frame lays out nothing: `CalcEd::Layout` -> `ToolTabs::layout`.

## Editor tab: scrolling and children made later

- A tool without an AUI manager gets a `wxScrolledWindow` as the page body
  (`ToolPage`): a frame is fitted to its display by `fitToScreen`, a page
  is not, and the editor's sizer minimum (about 684 px) is taller than a
  1366x768 page (548 px). Builder pages stay plain panels. After a content
  change `CalcEd::refit` lays the page out (`ToolTabs::layout`), which sets
  the scroller's virtual size to the sizer minimum; `ToolTabs::select` lays
  it out too (a tab gets no first-Show layout).
- Controls the editor makes after adoption (code buttons, the summary
  labels under Settings) were created with `this`, the hidden frame, as
  parent: never shown, so the Code row was empty. Use
  `CalcEd::clientParent()`. See tab-children-made-later-need-the-page-as-parent.md.

## Known gap: blank first paint when the content fits

At 1366x768 (content taller than the page, so it scrolls) the editor tab
paints on first show. On a taller page (Xvfb 1280x1024, content fits, no
scrollbar) the tab is still blank grey until left and re-entered, though
`tabinfo` shows every control laid out and shown. `CalcEd`'s Freeze/Thaw now
act on the page (`UpdateHold`, `wxWindowUpdateLocker(clientParent())`), and
layout/`Refresh`/`SendSizeEvent` after the fill did not cure it; the cause is
not found. `one_process_test.py` (default screen) fails its
"editor tab is painted" check (distinct colours under the page) for it.

## Test hooks

`tabs`, `tab <n>`, `towindow <n>`, `totabs <title>`, `tabinfo [labels]` (page size,
shown children, status bar parent, frame mapped or not, scroller view and
virtual size, `STALE` when the virtual height is below the sizer minimum,
`<label>=reachable|CLIPPED` for each button label in the comma list,
`clipped=N` other controls out of reach, `colours=N` distinct colours on
the screen under the page, so a blank page shows as a handful), `charhook <key>`;
checked by `tests/apps/one_process_test.py --tabs-only` (`--shots DIR`).

## Known gaps

- Dialogs and popups with the hidden frame as parent: placement on GTK.
- `MenuPlacement::InTab` is not implemented.
