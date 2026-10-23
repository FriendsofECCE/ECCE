---
type: pitfall
title: "Builder mode: setMode() sets mode, viewer and toolbar together; Esc is a char hook"
area: wx-viewer
paths: [src/apps/builder/Builder.C, src/apps/builder/BuilderScript.C, src/tdat/chemistry/Fragment.C, tests/apps/builder_mode_test.py]
issues: [321]
---
`setMode` = `applyMode` (mode in effect, viewer pick/view, cursor) + `toggleModeButton`
(toolbar, Mode menu, pane layout). Calling the halves separately let the toolbar
and the mode in effect differ (Esc used to set only the mode). Never call
`toggleModeButton` after `setMode`; its Structure Library branch saves the default
layout only if the library pane is not already shown, or a second call would store
the library layout as the default.

A Structure Library insert keeps the mode (adding several ligands in a row); a click
on free space never lands on existing atoms, `Fragment::mergeFragment` moves the new
structure past the old bounding box if the clicked point is within 1.5 A of one.
A failed connect stays in the mode. `Fragment::bondingAtoms` used to
turn the picked hydrogen into a Nub before it knew the library side had a site, so a
failed connect left a Nub behind; the library side is checked first now.

Esc: key events go to the focused canvas, so the frame binds `wxEVT_CHAR_HOOK`
(`escapeKey`), which leaves text fields and spin controls alone and acts only in
Structure Library, Build and Bond modes.
Script hooks: `key escape`, `selectelem A`, and `info` (`mode= toolbar= checked=
viewer=`, `nubs=`); test `tests/apps/builder_mode_test.py`, run by `run_tests.py`.

Bond button (#462): `ID_MODE_BOND` is a check tool on the mode toolbar beside
Library; its click is `OnModeBondClick`, which publishes `BondChanged` with the
Build panel's kind, so the button never chooses one. `toggleModeButton` must
toggle it too, or Mode > Bond leaves it unpressed. Script hook `toolbar bond`;
test `tests/apps/builder_bond_button_test.py`. Icon: `bond.svg`, from
`tools/screenshots/build_icons_e.py`.
