---
type: pitfall
title: "Builder: Ball and Stick draws every bond as one cylinder; aligned bonds were forced single; a delete with bonds selected kept the nubs; Lasso Select applied only at the next mode change"
area: wx-viewer
paths: [src/viz/sgcommands/BondEditCmd.C, src/viz/sgcommands/SGFragment.C, src/apps/builder/Builder.C, src/wxviz/viztools/ViewerEvtHandler.C, src/inv/moiv/ChemDisplayCylinders.C, src/inv/wxinv/SoWxRenderArea.C, tests/apps/builder_edit_bugs_test.py]
issues: []
---
Bond orders are stored and reach `SGFragment::getBondType`, but only the
wireframe path (`ChemDisplayWireframe.C`) draws double/triple bonds;
`ChemDisplayCylinders.C` (Ball and Stick, Stick) draws one cylinder per
bond, as in the PNNL base, so a changed order is invisible there.

Bond mode on two molecules with "Align molecules?" = Yes went through
`FragUtil::bondParts` with a literal 1.0; it now gets the chosen order.

`SGFragment::deleteSelectedAtomsAndBonds` (atoms and bonds selected, as a
drag box or Select All selects them) rebuilt the delete list from the
highlight alone and dropped the nubs `syncNubSelection` added: each nub
stayed and gained a nub of its own.

Select mode (Builder): `SGSelection::chooseGesture` decides at the press.
Empty space starts a box (`lassoType` DRAGGER for that press only), a
selected atom may move the selection after the 333 ms drag delay, any other
pick only click-selects (a drag from an unselected atom moves nothing).
`Builder::syncLassoType` turns this on in Select mode only; the View >
Select Mode Action menu is gone and `/SelectModeDrag` is no longer read.
The box is drawn by `SoWxRenderArea::drawRubberBand` in the GL frame after
the scene (pixel corners from `SGSelection`), not by the overlay scene
graph, which is still a stub.

Script hooks: `bondorder single|double|triple|aromatic|metal` (sends
BondChanged), `bondorders`, `selectbonds`, `selection`, `coords`.
Test: `tests/apps/builder_edit_bugs_test.py`
(`apps_builder_edit_bugs`).

Experiment `ECCE_MULTIBOND_CYLINDERS=1` (off by default): Ball and Stick
and Stick draw double/triple/aromatic bonds as parallel cylinders
(`ChemDisplay::renderMultiBondCylinders`, own pass; the single-bond pass
gets a list without them). The live single-bond path is `clipNormalCylinder*`
in `ChemDisplayCylindersLOD.C`, not `normalCylinder*` (`clipBonds` is true).
Headless view: `viewer-scenes` systems `ethene`, `acetylene`, `co2`,
`benzene-kekule`, `benzene-arom`.
