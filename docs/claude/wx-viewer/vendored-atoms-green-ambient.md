---
type: pitfall
title: "Vendored-Inventor build: atoms tinted by a frozen ambient (green from the corner text); ChemDisplay must set glColorMaterial(DIFFUSE) before enabling it (#83, #210)"
area: wx-viewer
paths: [src/inv/moiv/ChemDisplay.C, src/inv/moiv/ChemDisplaySpheres.C, src/inv/moiv/ChemDisplayCylinders.C, src/wxviz/viewer/SGViewer.C, src/wxviz/viewer/SceneScript.C]
issues: [83, 210]
---
Symptom: with `-DECCE_USE_COIN=OFF` (and every 8.x build) carbon renders
sage green and hydrogen pale green, on any background; about +51 on the
green channel only (= 0.2 light-model ambient x 1.0). The Coin build is
unaffected.

Cause: the vendored core enters `ChemDisplay::GLRender` with
`GL_COLOR_MATERIAL` in `GL_AMBIENT_AND_DIFFUSE` mode, so ChemDisplay's
`glMaterialfv(GL_AMBIENT, 0.2)` is ignored. The sphere/cylinder code then
calls `glColorMaterial(GL_FRONT, GL_DIFFUSE)`, which freezes the ambient at
whatever colour Mesa last flushed into the material. Mesa applies a
`glColor` outside Begin/End lazily, so that is often not ChemDisplay's
`glColor3f(1,1,1)` but the corner text's `glColor3f(fg)` from
`SGViewer::myCallbackRoutine` (green, the foreground for a black
background). Coin enters with `GL_DIFFUSE`, so its ambient stays 0.2 grey.

It depends on flush timing: `ECCE_DEBUG_MATERIAL` (its `glGet*` calls flush)
turns the tint into a uniform +51 grey (white ambient), which is the "+41 on
every channel" vendored offset `tools/coin/compare.py` corrects for. So the
debug log does not show the green; compare renders made without it.

Fix: `glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE)` before
`glEnable(GL_COLOR_MATERIAL)` in `ChemDisplay::GLRender` (inside its
`glPushAttrib(GL_LIGHTING_BIT)`, so nothing outside changes).

Separate, screenshot-only: the scene script's `background` used to leave the
foreground (corner text) green on white; it now picks the contrasting colour
as `ViewerEvtHandler::setBackground` does.
