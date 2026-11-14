---
type: map
title: "Tools as tabs (#983 stage 2): a tool stays a hidden wxFrame, its windows move into a notebook page"
area: services
paths: ["src/wxgui/mdtools/MDEdBase.C", "src/apps/mdprepare/MDPrep.C", "src/apps/basistool/WxBasisTool.C", "src/wxgui/jms/ToolTabs.C", "src/wxgui/ewxClasses/ewxModalGuard.C", "include/wxgui/ToolTabs.H", "src/apps/builder/Builder.H", "src/apps/builder/Builder.C", "src/apps/builder/BuilderWindow.C", "src/apps/organizer/CalcMgrApp.C", "src/wxgui/jms/InProcessTools.C", "tests/apps/one_process_test.py"]
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

## Basis Set Tool, MD editors, MD Prepare (adopted the same way)

- `BasisToolWindow.C` adopts right after `new WxBasisTool`; `MDEdBase::
  openInProcess` and `MDPrepWindow.C` adopt just before the first Show.
  `MDEdBase` is the one base of MD Energy/Optimize/Dynamics and Gmx
  Prepare, so all four are tabs.
- Each overrides `Show/Raise/SetTitle/Destroy` and releases in its
  destructor. `MDEdBase` and `MDPrep` also hide `FindWindow(long)` (their
  code looks controls up by id long after construction) and `GetMenuBar`;
  `Fit()` after a content change becomes `ToolTabs::layout`.
- `WxBasisTool::Iconize` does nothing while hosted: `Iconize(false)` on the
  hidden frame is a gtk deiconify and would map it.
- The Basis Set Tool's menu bar has no File menu (first menu "Molecular
  Basis"); the test checks that name.

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

## Blank first paint when the content fits (fixed)

On a page where the editor's content fits (no scrollbar) the tab was flat
grey until re-entered: the reparented children kept GTK's unallocated 1x1
because the body's pizza was never size-allocated again. `allocateChildren`
(on the scroller's idle, after `layout`/`select`) allocates them. See
../wx-viewer/gtk-reparented-children-never-allocated.md.

## Test hooks

`tabs`, `tab <n>`, `towindow <n>`, `totabs <title>`, `dialog`/`dialoginfo`, `tabinfo [labels]` (page size,
shown children, status bar parent, frame mapped or not, scroller view and
virtual size, `STALE` when the virtual height is below the sizer minimum,
`<label>=reachable|CLIPPED` for each button label in the comma list,
`clipped=N` other controls out of reach, `colours=N` distinct colours on
the screen under the page, so a blank page shows as a handful; `gtk` adds
frozen/mapped/GTK allocation of the body, its ancestors and first child), `charhook <key>`;
checked by `tests/apps/one_process_test.py --tabs-only` (Builder, editor, and MD Energy + MD Prepare + Basis Set Tool in `runTabsTools`) (`--shots DIR`).

## Dialogs of a tab

A tool's dialogs and message boxes have its hidden frame as parent, which
has no place on screen. `ewxModalGuard` (every modal dialog passes its
hook, `wxMessageBox` and file dialogs included) calls
`ToolTabs::dialogOpened`: for a hosted parent the dialog is made GTK-
transient for the Organizer's frame and centred over it, and the control
that had the focus gets it back in `dialogClosed`. No call site changes.
Closing must not touch the dialog's parent: a tool's own "save changes?"
box outlives the tool's frame (SIGSEGV at quit otherwise); the dialogs
moved are remembered in a set. Hook `dialog` (opens a message box over the
active tool tab, measures it 1.5 s later) + `dialoginfo` (transient, mapped,
inside the Organizer's rectangle).

## Known gaps

- Popup menus and other non-dialog windows with the hidden frame as parent:
  not checked (modal dialogs are handled, see below).
- `MenuPlacement::InTab` is not implemented.
