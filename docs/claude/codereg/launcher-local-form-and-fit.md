---
type: pitfall
title: "Launcher: the simplified local form is for unqueued machines only, and Fit() cannot shrink the frame"
area: codereg
paths: ["src/apps/launcher/WxLauncher.C", "src/apps/launcher/WxLauncherGUI.C", "tests/machregister/launcher_test.py"]
issues: []
---
`WxLauncher::refreshRemoteSection` decides the form. A machine that names
this host (`namesThisHost`) and has no queue (`Q` option) gets the local
form: no Remote Machine Access block, and Username, Scratch Directory and
Machine settings in the "Advanced" fold. A queued machine, even on this host
(a workstation running Slurm), keeps the full 9.2.2 form; only password,
shell and Open are hidden while the job runs here (`runsHere`). Which batch
fields show depends on the registration, not on locality: Nodes needs more
than one node, Wall Time needs `TL`, Memory `MM`, Alloc. Account `AA`. A
registration without them looks "stripped" but is not a regression.

The frame's minimum size is set at construction, when every panel is shown.
`Fit()` clamps to that minimum and never shrinks the frame, and whichever
item has proportion 1 takes the slack (the message log did). `refit()` calls
`GetSizer()->SetSizeHints(this)`, which recomputes the minimum. The log has
proportion 0 and a fixed five-line height.
