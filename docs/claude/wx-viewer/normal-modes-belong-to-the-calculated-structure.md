---
type: pitfall
title: "Normal modes belong to the calculated structure; reloading a fragment needs touchNumbers (#244)"
area: wx-viewer
paths: [src/apps/builder/NModePanel.C, src/viz/propsgcommands/NModeTraceCmd.C, src/viz/propsgcommands/NModeStepCmd.C, src/viz/propsgcommands/NModeVectCmd.C, src/viz/sgcommands/SGContainer.C, src/viz/sgcommands/SGFragment.C, tests/apps/nmode_edit_test.py]
issues: [244]
---
VIB holds one vector per atom of the calculated structure, and nothing
ties it to the structure in the viewer, which the user can edit (Periodic
Builder Generate/Supercell, adding atoms). `NModePanel::modesApply()`
compares the viewer's atom count with VIB's rows and the element at each index with
the calculation's fragment (cached at `Create`). When they differ, the
panel disables itself with a note and never runs a mode command. The
commands also refuse a count mismatch on their own.

`NModeTraceCmd` replaces the viewer's fragment (`clear()` +
`calc->getFragment()`). After any such reload, call `touchNumbers()`.
`SGFragment`'s bond index (`p_bondIndices`), which every `ChemDisplay`
reads, is rebuilt only there. Without it the display draws bonds between
atoms that are gone (SIGSEGV in `ChemDisplay::clipNormalCylinderBHAI`).
`ShowFragCmd` does this. `NModeTraceCmd` did not.

Arrows are matched to atoms by child index, so `updateNMVecStarts` (run
on every structure change) drops them when the counts differ.
Test: `tests/apps/nmode_edit_test.py` (scene commands `vib...`).
