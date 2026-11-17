---
type: map
title: "Tools as tabs (#983 stage 2): a tool stays a hidden wxFrame, its windows move into a notebook page"
area: services
paths: ["src/wxgui/mdtools/MDEdBase.C", "src/apps/mdprepare/MDPrep.C", "src/apps/basistool/WxBasisTool.C", "src/wxgui/jms/ToolTabs.C", "src/wxgui/ewxClasses/ewxModalGuard.C", "include/wxgui/ToolTabs.H", "src/apps/launcher/LauncherWindow.C", "src/apps/launcher/LauncherMessages.C", "src/apps/builder/Builder.H", "src/apps/builder/Builder.C", "src/apps/builder/BuilderWindow.C", "src/apps/organizer/CalcMgrApp.C", "src/wxgui/jms/InProcessTools.C", "tests/apps/one_process_test.py", "src/util/genutil/WindowLayout.C", "include/util/WindowLayout.H", "src/wxgui/wxdialogs/GlobalPrefsWindows.C"]
issues: [983]
---
Switch: `ECCE_ONE_PROCESS=1` and Preferences > Windows and tabs
(`util/WindowLayout`, stored in EcceGlobal; default Normal).
`ToolTabs::enabled` (tab strip made at the Organizer's start) needs some
row set to Tab then; each opener calls `ToolTabs::open(tool, mgr, title,
key)`, which adopts only if that tool's row is Tab now. A tool that opens
in a window by preference gets no `Tab` entry, so no Move to Tabs. Normal = Small plus
the Set-up tab (below). The tab menu's Always open in a window = `setRow(key,false)`
+ `release`; GlobalPrefs re-reads the setting in `Show`.

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

## Rows overlapping after a summary line appears (fixed)

A `wxScrolledWindow` lays its sizer out at its *virtual* size, which nothing
updated when content grew (the "DFT Functional" line under Settings), so
rows were stacked into the old height. `ToolTabs::fitScrollBody` (in
`layout`, `select` and the scroller's size event) sets virtual size =
max(sizer minimum, view). Also wx and GTK could disagree about a widget's
size (the Settings static box kept its old height and framed too little):
`allocateChildren` re-allocates every child whose GTK size differs from wx's,
not only the 1x1 ones. The `setup ...` hooks report `overlaps=N` (shown leaf
controls of the editor's column whose rects intersect) and `more="+N hidden"`;
the hook `setup width N` resizes the Organizer's frame, not the editor's
hidden one.

## Blank first paint when the content fits (fixed)

On a page where the editor's content fits (no scrollbar) the tab was flat
grey until re-entered: the reparented children kept GTK's unallocated 1x1
because the body's pizza was never size-allocated again. `allocateChildren`
(on the scroller's idle, after `layout`/`select`) allocates them. See
../wx-viewer/gtk-reparented-children-never-allocated.md.

## Test hooks

`layout show|<preset>|KEY=tab|window`, `tabmenu <n>`, `alwayswindow <n>`,
`prefs windows show|preset|row|snap` (runLayouts), `tabs`, `tab <n>`, `towindow <n>`, `totabs <title>`, `dialog`/`dialoginfo`, `popup`/`popupinfo`, `float`/`floatinfo`, `tabinfo [labels]` (page size,
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
inside the Organizer's rectangle; `focus=` only once it has closed). On
GTK `wxMessageDialog` is native and runs `gtk_dialog_run`: `EndModal` only
asserts and leaves it open, and the nested loop then hangs Quit (killed,
rc -15). Close it with `gtk_dialog_response`.

A tool's own sizing code (`FindWindow`, `GetSizer`, `Fit`) finds nothing
on the frame once it is a tab: guard it with `ToolTabs::layout(this)`, as
MDPrep and `WxBasisTool::updateSelectionsSize` do (SIGSEGV on a message
that reloads the calculation otherwise).

## Set-up tab (Normal, `WindowLayout::combinedSetupTab`)

`ToolTabs::open(..., SideFn)` makes the page a `wxSplitterWindow`: the
editor's scroller left, a side panel right that the SideFn fills on every
(re)adoption (`CalcEd::makeSetupArea` -> `SetupArea`). Open in Window
destroys the side panel; `p_setupArea` is cleared by its wxEVT_DESTROY, and
the details go back to being dialogs. Theory/Runtype Details are embedded
only for codes with a `.dlg` (native engine `src/wxgui/details`):
`SetupArea::showDetails` parses the same command line the editor would run,
builds a `details::Model` + `DetailsPanel` hosted in a panel, and a
transport that delivers each line to `processTheoryInput`/
`processRuntypeInput` by `CallAfter` (as UDP would: after the event, in
order), after a `#STARTED 0` so no pid is ever killed. The NO_GUIValues
(headless initialise) run still uses the process. The preview is the stored
deck (`PRIMARY_INPUT`) while nothing is unsaved; after a change it is
`DeckWriter::generate` with `Options::dryRun` (500 ms timer restarted by
each `setEditStatus`): theory, runtype, spin and the editor's GUIValues
replace the stored ones in the .param file, the deck goes to
`Result::deck`, nothing is put to or removed from the calculation.
Structure and basis are still the stored ones (they are read from the
calc in several helpers). The detail area is a wxScrolledWindow whose
virtual size is the embedded dialog's sizer minimum; the sash is set to
the editor's minimum width and the window widened to 1100 px (work area).
wxGTK: the embedded panel stays blank until a size event after it is made
(`SetupArea` sends one by CallAfter).

## Known gaps

- `MenuPlacement::InTab` is not implemented (the design above is one
  sentence: each tab draws its own menu bar; no layout is specified).
- A floated AUI pane stays on screen when another tab is selected.
- Process-wide singletons (the event dispatcher, #1088) see every tool:
  anything holding pointers to a tool's objects must drop them when the
  tool closes (`one-process-event-dispatcher-listeners.md`).
- Never select a tab by focusing a window in it; the notebook ignores
  focus on a hidden page (`tab-switches-when-a-hidden-page-takes-focus.md`).
- Launch from the editor tab is not covered by `runTabs1088` (Save and the
  close button are); the screen greying while a tool starts or a job is
  submitted (#1088 items 5, 6) was `startDisabler`'s `wxWindowDisabler`; now
  `InputBlock` (an event filter, nothing disabled) in one-process mode.
- `InProcessTools::open` reuses a window by the context it was opened for
  as well as its poll context (set only after its first invoke).
- Open: #1088 item 9 (Viewer in an empty box after Terminate) not
  reproduced or fixed; item 4's Quit-ECCE cause was not found, only guarded
  (`closingTool` in `confirmAndQuit`).

## Popups and secondary windows of a tab (#1068)

`PopupMenu` on the hidden frame has no screen place to attach to: call
`ToolTabs::popupParent(this)->PopupMenu(menu, pt)` (pt in that window's
coordinates; the page in a tab, else `this`). Done in `Builder::mouseEvent`
(constraints menu) and `CalcEd::OnButtonCalcedBasisQuickClick`. Popups on
child controls (ElementButton, PerTabPanel, ewxGrid, WxFeedback,
ThingMenubtn, `ewxPopupMenu(this...)`) already use a window in the page.
AUI floating frames, `TearFrame` and `TearableContent` (a
wxPopupTransientWindow whose parent is the Builder frame) call
`ToolTabs::secondaryOpened(win, parent)`: GTK-transient for the Organizer's
window. The AUI float frame had its transient link cut on purpose
(`detachFromParent`); it is set to the main window after that. Frames made
with `wxGetTopLevelParent(this)` of a page child (OPDOS, normal modes plots,
`ewxPlotCtrl`) already have the Organizer as parent. Hooks `popup` /
`popupinfo`, `float` / `floatinfo`; test `runTabsPopups`
(`ECCE_POPUPS_ONLY=1`).

## Launcher (adopted the same way, #983 step 1)

`WxLauncherApp` split like the Basis Set Tool's: the handlers are
`LauncherMessages` (owned and deleted by `WxLauncher`), the app only makes
the frame. `LauncherWindow.C` opens one in process (`launcher` row, Tab in
Normal/Small). The launch steps' `DeferMessages` guard defers the window's
own `LauncherMessages` (`p_msgs`), not `wxTheApp`, which in one process is
the Organizer's dispatcher. The frame overrides `Show/Raise/SetTitle/
Destroy/Iconize/Layout/GetMenuBar/FindWindow(long)`; `refit` becomes
`ToolTabs::layout`. Standalone `launcher` is unchanged.

## LaunchPanel and the Set-up area's Launch (#983)

The Launcher's controls and launch logic are `LaunchPanel` (a wxPanel);
`WxLauncher` is only the frame, menus, geometry and ToolTabs plumbing around
it, and the panel talks to its host through `LaunchHost` (close, defer broker
messages, relayout, menu enabling, `inlineWarnings`). `LauncherMessages` and
the test script drive the panel; Show/Iconize/Close go to the frame.

In the Set-up tab Launch... shows a panel in `SetupArea` (kind `LAUNCH`),
made through `LaunchEmbed::maker()` because the editor is linked without the
Launcher; with none registered the tool opens as before. That host has
`inlineWarnings()`: the three pre-launch confirmations (MPI missing, ECCE-QM
size, the launch check) become a warning line and the button reads "Launch
anyway"; the next click accepts exactly the warnings shown, then launches.
The embedded panel has no broker subscription: it publishes its launch
notices as usual but does not follow later state or machine-list messages.

A launch whose machine asks for an ssh password runs the separate
`passdialog` program on the worker thread; until it is answered the GUI
thread waits in `runLaunchStep` and Quit does not return. The old test
fixture (an NWChem calculation on `localhost` over ssh) hit that, which was
the -15 at Quit.

## Remaining tools: what each is and where it should run (#1066)

"In" = opens in the Organizer's process with `ECCE_ONE_PROCESS=1`
(`InProcessTools::add`). Most of the tools the issue lists are not GUI
programs at all: they are command-line helpers (the gateway never starts
them as windows; scripts, parsers and the Builder's own code call them),
so there is nothing to bring in.

| Tool (binary) | GUI | Started by | Size | Globals / exit() | Recommendation |
|---|---|---|---|---|---|
| Periodic Table (`pertable`) | wx frame | gateway `ecce_invoke`/`PeriodicTable` | 155 + frame in `wxgui/pertable` | none; app only does `-pipe` login and auth cache | **In, as a window** (done) |
| Machine Browser (`machbrowser`) | wx frame | gateway, from Organizer/Launcher | 3100 | `_exit(0)` only in the `ECCE_MACHBROWSER_SHOT` test hook (not linked to a message path) | **In, as a window** (done); frame is its own `JMSPublisher`, kept |
| Register Machines (`machregister`) | wx frame | gateway, Organizer, Launcher | 10000 | `exit()` in `JobPreview.C` and `WxMachineRegisterScript.C` (script mode); modal dialogs, test hooks, own `-pipe` login | **In, as a window** (done): the `_exit`s are a forked child after `execvp` (`JobPreview.C`) and the `ECCE_MACHREG_SCRIPT` hook, neither on the window path; not a tab |
| Solvate (`solvate`) | wx frame | gateway (AT_SOLVATE calculation) | 2000 | app main does EDSI setup per window | Tab candidate after the Builder-style adoption; own step (large, editor-like) |
| MD Analysis (`mdanalysis`) | no | `ecce-parse`/job scripts | 80 | exit via main | Stays a process |
| Thermochemistry (`ecce-thermo`) | no, CLI + JSON | scripts, Builder panel calls the library | 870 | `g_*` option globals in the CLI | Stays a process (the GUI is the Builder's panel) |
| MO composition (`ecce-mocomp`) | no, CLI | scripts, tests | 300 | main only | Stays a process |
| Force comparison (`ecce-force-compare`) | no, CLI | scripts | 30 | main only | Stays a process |
| Line profile (`lineprofile`) | no, CLI | scripts, tests | 200 | main only | Stays a process |
| Density field / plane (`ecce-density-field`, `densityplane`) | no, CLI | scripts, tests | 200 each | main only | Stay processes |
| Symmetry (`src/apps/symmetry`) | no: autosym/gensym/... filters, some Fortran | the symmetry code | n/a | each is a `main()` | Stay processes |
| Job store (`eccejobstore`, `eccejobmaster`) | no | the job machinery | 450 | `exit()` | Stays a process |
| Dialog helpers (`passdialog`, `hostkeydialog`, `msgdialog`, `ecce-dialog`) | small wx | a process that must ask without a window of its own (ssh, worker threads) | small | exit codes are the answer | Stay processes: the caller waits for the exit status, and the Quit hang in tool-tabs.md comes from running these in the GUI thread |

## Periodic Table, Machine Browser and Register Machines as in-process windows

`PerTabWindow.C` and `MachineBrowserWindow.C` follow the Basis Set Tool's
split: the frame is unchanged, a small `WxJMSMessageDispatch` subclass (the
App's handlers: invoke, quit, identify, preferences / machine changes)
replaces the `WxAppJMSMessageDispatch`, and the app's per-process part stays
the Organizer's. The apps' objects are built once (`pertable-objs`,
`machbrowser-objs`) and linked into the Organizer and the standalone
program, whose `OnInit` is unchanged.

- Plain windows, no `ToolTabs::open`: both are small utility windows with
  their own menu bar and no Move to Tabs.
- Both frames `Destroy()` themselves on close. The opener binds
  `wxEVT_DESTROY` on the frame: it untracks the dispatcher at once (so
  `closeAll` and the reuse lookup never see a dead frame), clears the
  dispatcher's frame pointer, and deletes it by `CallAfter`.
- Register Machines (`MachineRegisterWindow.C`, `machregister-objs`) is the same: invoke selects `initmachine`/`initpage`, quit closes it; the app's `-admin` mode stays standalone only. Closing asks about unsaved changes as before.
- One window each: a second request finds the tracked dispatcher
  (`InProcessTools::open`, poll context empty) and raises it.
- Checked by `ECCE_UTILS_ONLY=1 one_process_test.py` (`runUtilityWindows`):
  open, reuse, `closetools`, open again.

## The Set-up area's Launcher panel follows the broker (#1064)

`EmbeddedLaunch` (`LauncherWindow.C`) owns a `LauncherMessages` with host
NULL and `subscribeFollow()` (state, property, machine, preference notices;
not invoke/quit/identify). It is deleted before the panel, also from the
panel's destroy event, since its handlers hold the panel raw.
