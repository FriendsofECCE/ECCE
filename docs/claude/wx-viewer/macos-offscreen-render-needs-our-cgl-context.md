---
type: pitfall
title: "macOS: Coin's own offscreen context fails, so every thumbnail failed until CoinCglOffscreen (#344)"
area: wx-viewer
paths: [src/inv/wxinv/CoinCglOffscreen.C, src/inv/wxinv/GlPlatform.C, src/wxviz/viewer/VizRender.C]
issues: [344]
---
Coin 4.0.10's CGL glue creates an `SoOffscreenRenderer` context first as a
CGL pbuffer and then with a `kCGLPFAOffScreen` pixel format. macOS 12 refuses
both (`Couldn't create CGL pBuffer: invalid drawable`, then `invalid pixel
format attribute`), so `render()` returned false and File > Save in the
Builder reported "VizRender error: Unable to render scene graph" from
`VizRender::thumbnail`. The structure itself was saved.

`CoinCglOffscreen` hands Coin a windowless CGL context (accelerated pixel
format, legacy profile) drawing into an FBO, through
`cc_glglue_context_set_offscreen_cb_functions`, as `CoinEglOffscreen` does on
Linux. It saves and restores the current CGL context around each render so
the wx canvas keeps its own.

Headless check on the Mac: run the Builder on a `file://` calculation with
`ECCE_VIEWER_SCENE` holding `vizfile t1 128` and `savethumb`;
`COIN_DEBUG_GLGLUE=1` prints which offscreen path Coin took.
