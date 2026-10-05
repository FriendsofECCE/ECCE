---
type: pitfall
title: "ChemDisplay's glPopAttrib undoes what Coin's lazy element sent inside it; the next shape in a fresh context drew unlit"
area: wx-viewer
paths: [src/inv/moiv/ChemDisplay.C, src/inv/moiv/ChemDisplayCylinders.C, src/wxviz/viewer/VizRender.C, tests/look/offscreen_shading.py]
issues: [229, 231]
---
`ChemDisplay::GLRender` wraps the atoms and bonds in
`glPushAttrib(GL_CURRENT_BIT|GL_LIGHTING_BIT|GL_ENABLE_BIT)`. Bond cylinders
call `SoGLLazyElement::send(ALL_MASK & ~diffuse)` inside it, so the lazy
element records the light model (GL_LIGHTING on) as sent. The pop restores
whatever was enabled before the push; the lazy element does not know and
skips the next `glEnable(GL_LIGHTING)`.

It shows only in a context where nothing had enabled lighting before the
molecule, i.e. the first render of a new `SoOffscreenRenderer`. VizRender
makes a new one on every size change, so nearly every thumbnail and Save As
image had the ESP surface flat and unshaded (ball-and-stick only; spacefill
and wireframe do not send). Later renders replay a cache built while
lighting happened to be on and look right. Measured on the synthetic ESP
(`esptest`): surface luminance mean 195.5 / sd 50.1 against the canvas's
150.8 / 58.3; after the fix 150.8 / 58.2.

Fix: `reset(state, ALL_MASK)` on the lazy element right after the pop, so
it re-sends. Any node that sends lazy state inside its own push/pop needs
the same. Test: `ctest -R look_offscreen_shading` (needs `ninja
viewer-scenes`). A headless scene that rotates the camera must paint the
canvas (`snap`) before an offscreen render, or the headlight still points
the old way.
