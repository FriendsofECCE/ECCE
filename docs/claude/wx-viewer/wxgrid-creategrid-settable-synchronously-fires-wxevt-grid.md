---
type: map
title: "`wxGrid::CreateGrid()`/`SetTable()` synchronously fires `wxEVT_GRID_SELECT_CELL`"
area: wx-viewer
section: "wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls"
issues: [78]
paths: []
---
**`wxGrid::CreateGrid()`/`SetTable()` synchronously fires
`wxEVT_GRID_SELECT_CELL`** on wx3.2/GTK3 — immediately, during
construction, not deferred to the event loop the way it effectively
was on wx2.8/GTK2. A grid built early in a panel's `CreateControls()`
(e.g. `NModesGUI`'s vibration-mode grid) can reach an
`EVT_GRID_SELECT_CELL`-bound handler in a subclass (`NModePanel::
OnModeSelection` → `showMode()`) that dereferences sibling
controls/members not constructed yet — including ones that don't
exist until a *separate, later-running* function does (`p_slider`,
only built in `NModePanel::Create()` itself, after the
`NModesGUI::Create()`/`CreateControls()` call that triggers the
event) — so reordering statements within the one function that
triggered it isn't guaranteed to be enough; every dependency the
handler touches needs to actually exist first. Reliably segfaulted
`builder` on opening *any* job with vibrational (`VIB`) data (#78).
Fixed by wiring up the panel's already-declared-but-dead `p_isValid`
flag as a real "construction is fully finished" guard: the handler's
target function returns immediately if not yet valid, and the flag
is set `true` only once every control exists, right before the
panel's own legitimate first call into that function. Fixed twice,
independently, on two machines during the same investigation window
(#78) — one pass guards in `OnModeSelection()` itself, the other
guards inside `showMode()`; both landed and were kept as layered
defense-in-depth rather than picking one, along with deferring
`NModesGUI::CreateControls()`'s `CreateGrid()` call to the end of the
function (belt-and-suspenders against the narrower null-button
dereference that a first, incomplete pass at this fix hit). Check for
the same shape (an early-constructed grid/combo/list whose "populate
my data" call synchronously fires a selection/change event into a
handler with unmet dependencies) in any other panel that builds a
grid before finishing `CreateControls()`.
