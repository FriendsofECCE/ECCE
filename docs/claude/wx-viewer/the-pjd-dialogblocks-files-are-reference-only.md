---
type: map
title: "The `.pjd` (DialogBlocks) files are reference only; never regenerate code from them"
area: wx-viewer
section: ""
issues: [109, 187, 210]
---
**The `.pjd` (DialogBlocks) files are reference only; never regenerate
code from them.** They name plain wx classes, and PNNL's step that
turned the output into ECCE's `ewx` subclasses is lost, so
`dialogblocks --generate` replaces every `ewx` control with a plain
wx one (63 references became 3 in machine registration). Three are
also missing controls added by hand since: `NModesGUI` (#109),
`CalcEdGUI` (Verify, Use Symmetry, Regenerate Input) and
`WxMachineRegisterGUI` (#187). Edit the generated `.C`/`.H` directly;
when #210 reworks a dialog, delete its `.pjd` in the same commit.
