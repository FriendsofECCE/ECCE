---
type: pitfall
title: "An offscreen render has no background of its own; a GLX Coin warns about the EGL offscreen context (#276)"
area: wx-viewer
paths: [src/wxviz/viewer/VizRender.C, include/wxviz/VizRender.H, src/apps/builder/Builder.C, src/inv/wxinv/CoinEglOffscreen.C, src/wxviz/viewer/SceneScript.C]
issues: [276]
---
`SoOffscreenRenderer` clears to its own background colour, black unless it is
set; the viewer's background lives in the canvas's scene manager, outside
`getTopNode()`. `VizRender::thumbnail(SoNode*, ...)` therefore takes the
colour as a required argument (the Builder passes
`p_viewer->getBackgroundColor()`), and the url overload used by the
vizthumbnail app takes it from the dialog viewer, which `loadDisplayStyle`
gave the Builder's Background preference. The thumbnail was black for
every background while r, g, b defaulted to 0.

Debian's Coin is a GLX build: setting up the glue of each new GL context,
`glxglue_init` calls `glXGetCurrentContext()` and, with our EGL offscreen
context current, posts `Coin warning in glxglue_isdirect(): Couldn't get
current GLX context.` (once per offscreen context; there is no environment
switch). `CoinEglOffscreen` drops exactly that warning, only between its
`makeCurrent` and `reinstate`, and forwards everything else to Coin's
handler.

Headless check: scene commands `background r g b`, `savethumb`,
`storedthumb <name>` in the Builder with `ECCE_VIEWER_SCENE_HOLD` set (so
the script draws in the Builder's own viewer), then read the corner pixels
of `<name>.jpg`.
