---
type: pitfall
title: "Charge-colouring panels must change the colour table and its binding in one paint; a paint per entry shows a half-restored table"
area: wx-viewer
paths: [src/apps/builder/MullikenPanel.C, src/apps/builder/PartialCharges.C, src/apps/builder/VecAtomSpectrum.C, src/viz/sgcommands/CSLoadColorsCmd.C, src/wxviz/viewer/SceneScript.C]
issues: [258]
---
`ChemColor::atomColor` means two things: with `ATOM_PER_ATOM_INDEXED`
(the normal view) it is indexed by atomic number, with `ATOM_PER_ATOM`
(the charge view) by atom. MullikenPanel, PartialCharges and
VecAtomSpectrum switch between the two by writing the table one
`CSLoadColorsCmd` at a time and setting the binding afterwards.

`fw.execute(cmd)` paints synchronously (see the `Builder::execute` entry),
so leaving the charge view painted ~120 frames, one per element entry,
all still per-atom: atom 0 in entry 0's pale salmon (1, 0.502, 0.447),
the others in charge colours until their entry came round. The binding
change itself only scheduled a redraw, which ran after the next panel
was built, so the last wrong frame stayed up. 120 vsync-limited swaps
are ~2 s on a real display; ~0.23 s on Xvfb.

Fix: `execute(cmd, true)` for every entry, set the binding, then one
`refreshRenderArea()`. Check headlessly with the Builder scene command
`propswitch <name> <ms> <property>` (detail layout): it clicks the
property in the Properties list and writes every frame painted, with
the colour each atom is drawn in, to `<name>-frames.txt`.
