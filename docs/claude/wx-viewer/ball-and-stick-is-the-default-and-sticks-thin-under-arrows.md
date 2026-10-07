---
type: rule
title: "Ball And Stick is the default style; sticks are drawn at 0.25 radius while normal-mode arrows are shown (#227)"
area: wx-viewer
paths: [src/tdat/chemistry/DisplayDescriptor.C, src/tdat/chemistry/TAtm.C, src/tdat/chemistry/Fragment.C, src/tdat/chemistry/Residue.C, include/tdat/DisplayStyle.H, src/viz/sgcommands/SGContainer.C, src/wxviz/viztools/ViewerEvtHandler.C, src/wxviz/viewer/VizRender.C, tests/look/nm_sticks.py]
issues: [227]
---
The default comes from several places that must agree: `DisplayDescriptor::DISPLAY`,
the `TAtm`/`Fragment`/`Residue` constructors, `DisplayStyle`'s default argument,
`SGContainer`'s initial style and fragment style, and the two `DefaultStyle`
fallbacks in `ViewerEvtHandler` and `VizRender`. A style a user saved
(`DefaultStyle` in the viewer's ini, written on exit) still wins; there is no
way to tell a saved Ball And Wireframe from the old default, so existing users
keep it.

An arrow along a bond lies inside the stick. While the normal-mode arrow switch
(`getNMVecRoot()`) is not NONE and holds arrows, `SGContainer::applyNMStickScale`
multiplies every `bondCylinderRadius` by `NM_STICK_FACTOR` (0.25) and divides it
back when they are hidden; a field sensor on the switch's `whichChild` triggers
it, so it runs from the event loop's delay queue (a scene script must call
`processDelayQueue`, as `nmhide` does). At 0.15 the bond disappears, at 0.4 the
shaft (0.035 A) is still half inside the stick. The normal-mode animation
(`getNMRoot()`) shows no arrows and keeps normal sticks.
