---
type: pitfall
title: "A details dialog also runs without a window (NO_GUIValues): no event loop, no timers"
area: codereg
section: "Pitfalls found more than once (check for siblings)"
paths: ["scripts/codereg/templates.py", "scripts/codereg/vinaruntype.py", "src/apps/calced/CalcEd.C"]
issues: [1244]
---
**A details dialog also runs without a window.** Choosing a theory or run
type in the Calculation Editor (`startTheoryApp(true)`/`startRuntypeApp(true)`)
starts the dialog with `NO_GUIValues`: it builds its widgets, and
`FinalizeSetting` sends every widget's value in one batch with
`#INITIALIZED` and closes. `EcceFrame.Finalize` never enters `MainLoop`,
so `EcceGlobals.EventLoopStarted` stays False and no `wx.Timer` fires.
These batched values are what the calculation stores until the user opens
Details. A default computed in the dialog (the Vina ligand from the
`.molecules` side file) must therefore be put into the widget before
`AddButtons()` in that mode — a plain `SetValue`, no `OnChanges`, which
would run `CheckDependency` on widgets not yet built. Only the shown dialog
(`GUIValues`) may defer it to a timer and send it as an edit, after the
stored values are restored.
