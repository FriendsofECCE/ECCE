---
type: pitfall
title: "GROMACS periodic-molecules: a tri-state in the Interaction model, resolved from the structure's CRYST1 only when the editor opens the task"
area: codereg
paths: [include/tdat/InteractionModel.H, src/dsm/xml/NWChemMDModelXMLizer.C, src/wxgui/mdtools/MDEdBase.C, src/wxgui/mdtools/InteractionPanel.C, src/tdat/chemistry/FragmentSummary.C, scripts/parsers/md.gmxtask, scripts/parsers/ai.gromacs, src/dsm/chemistry/GromacsStructure.C]
issues: [262]
---
`<PeriodicMolecules>` in the model's `<Interaction>` is -1 (unchosen, not
written), 0 or 1. `md.gmxtask` has no structure, so it cannot decide a
default: `MDEdBase` sets the unchosen value from
`FragmentSummary::isPeriodic()` (a CRYST1 record in the task's PDB, seen
by `scanPDB`) right after `getTaskModel`, before the input file is
generated. A task whose model is written without the GROMACS editor having
opened it keeps `periodic-molecules` off.

The structure check is the cell only (`GromacsStructure::groToPdb` writes
CRYST1 from the 3- or 9-value box). Bonds longer than half a box vector are
not looked for.

`ai.gromacs` writes `periodic-molecules = yes` or nothing (the GROMACS
default is no). The checkbox is hand-added to `InteractionPanelGUI.C`, not
in the `.pjd`, and shown only by `applyGromacsProfile`.
