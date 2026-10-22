---
type: pitfall
title: "Vibration Animation plays on choice (#443); VizPropertyPanel::hasFocus() is still false inside receiveFocus()"
area: wx-viewer
paths: ["src/apps/builder/NModePanel.C", "src/apps/builder/VizPropertyPanel.C", "src/apps/builder/Builder.C", "tests/apps/vib_autoplay_builder_test.py", "tests/apps/nmode_edit_test.py"]
issues: [443]
---
The Vibrational Frequencies panel has no Start/Stop buttons. `showAnimationMode()`
and `showMode()` start the timer when Animation is chosen and the panel holds the
viewer; `showVectorMode()` and `loseFocus()` stop it and restore the atoms with
`selectFragStep(-1)`. Space (Builder's `wxEVT_CHAR_HOOK`, only with the 3-D canvas
focused) calls `NModePanel::togglePause()`.

**`doFocus()` sets `p_hasFocus` after `receiveFocus()` returns**, so a test of
`hasFocus()` inside `receiveFocus()` or anything it calls reads the old value. The
panel keeps its own `p_active`, set at the top of `receiveFocus()` and cleared in
`loseFocus()`.

Scene-script hooks: `vibanim`, `vibvector`, `vibmode`, `vibspace`, `vibstate`
(prints animating, paused and a coordinate sum that must match after a stop).
