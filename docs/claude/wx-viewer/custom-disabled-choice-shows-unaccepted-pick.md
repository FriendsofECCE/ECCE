---
type: pitfall
title: "A custom-disabled ewxChoice still changes the shown entry; the disabler puts the last accepted one back"
area: wx-viewer
paths: ["src/wxgui/ewxClasses/ewxChoice.C", "src/wxgui/ewxClasses/ewxDisabler.C", "src/apps/launcher/WxLauncher.C", "tests/machregister/launcher_test.py"]
issues: []
---
`setCustomDisabledStyle(false)` keeps a control looking live and pushes an
`ewxDisabler` that eats its events. On GTK the native popup of a `wxChoice`
still opens and changes the shown entry; only `wxEVT_CHOICE` is eaten. The
read-only Launcher (no calculation, or one not in the Ready state) showed a
newly picked machine over the previous machine's queue, run directory and
form. `ewxDisabler::filterChoice` now restores the last accepted entry
(`ewxChoice::SetSelection` and an accepted event record it).

Hook commands that call a handler directly (`machine NAME` in the Launcher
hook) bypass the disabler and cannot catch this; `pick NAME` sends the event
through the choice's handler chain, and `editable 0|1` sets the Ready state.
