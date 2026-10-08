---
type: pitfall
title: "macOS (wx 3.3): opening a calculation with MOs crashed the Builder in wxAuiManager::DoFrameLayout"
area: wx-viewer
paths: [src/apps/builder/BuilderPanels.C, src/apps/builder/MoPanel.C]
issues: [133]
---
Symptom: `builder: ended by SIGSEGV` at start-up on a MOPAC or NWChem
calculation that has an `MO` property (not on ECCE-QM, not on Linux, not on
a calculation without MOs; flaky on some others). The crash report (kept
under ~/Library/Logs/DiagnosticReports, written a minute late) was
`Builder::setContext -> updatePropertyMenus -> updatePanes ->
wxAuiManager::Update -> DoFrameLayout`, a virtual call on a freed
`sizer_item` (the register held a malloc free-list pointer). DoFrameLayout
calls `m_frame->Layout()` and then walks `m_uiParts`; a second `Update()`
started from inside that layout frees them. `Builder::updatePanes` now
runs a nested request after the outer one returns (`ECCE_DEBUG_PANELS=1`
prints "nested, deferred"). The nesting path itself was not identified.
