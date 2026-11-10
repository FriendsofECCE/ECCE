---
type: pitfall
title: "Organizer: a tree reload from a timer inside the example import (or any wxYield) frees Resources the caller still holds"
area: services
paths: ["src/apps/organizer/CalcMgr.C", "src/wxgui/wxtools/WxResourceTreeCtrl.C", "tests/windows/organizer-probe.ps1"]
issues: [932, 889]
---
`WxResourceTreeCtrl::refresh(node)` deletes and rebuilds every child item of
`node` and calls `clearChildren()` on each child's Resource, so any
`Resource*` or `WxResourceTreeItemData*` below `node` held by code further up
the stack is freed. The example import (`openExampleSet`) holds them across
`ewxProgressDialog::Update()`, which pumps timers (on MSW too, despite
`YieldFor`'s UI filter), and so does every `wxYield()` loop.

Two timer-driven reloads ran in there:
- the #889 auto-refresh tick: the import selects the project it fills, the
  local watcher sees its own copies, and 1 s later `autoRefreshTick` rebuilt
  that project mid-import. `autoRefreshIdle()` now says no while
  `p_batchImport` is set or the active event loop `IsYielding()`.
- `ECCE_TEST_ORGANIZER` commands (500 ms timer): the Windows probe's
  `newproject` refreshes the home while the first-start import was still
  running. The command timer now waits while `p_batchImport` is set.

Symptom (#932): the Organizer died with 0xC0000005 one command later
(`newcalc`), so every following probe step (CalcEd, Builder, Basistool,
Launcher, MachineRegister) read "exited rc=-1073741819" as if each app had
crashed. Timing-dependent: only when the import took ~0.7 s instead of
~0.45 s on the runner; about two runs in three after #889, none before.
Read `windows-run/probe/organizer.err.txt` from the artifact, not the job
log, which shows only its last 15 lines.
