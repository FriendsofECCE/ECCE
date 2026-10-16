---
type: pitfall
title: "Depth peeling copies from the multisampled window: NVIDIA under XWayland drew the scene shifted (#360)"
area: wx-viewer
paths: [src/inv/wxinv/SoWxRenderArea.C, src/inv/wxinv/SoWxViewer.C, src/apps/builder/MoPanel.C]
issues: [360, 210]
---
- Focusing the MOs panel sets SCREEN_DOOR transparency, which a Coin build
  turns into depth peeling (SORTED_LAYERS_BLEND) even when no orbital is
  drawn; the ESP field type uses SORTED_OBJECT_BLEND and never peels.
- Coin copies each layer with glCopyTexImage2D/glCopyTexSubImage2D from the
  window framebuffer, which is 4x multisampled since #210. GL does not define
  a copy from a multisampled read buffer; NVIDIA 550 under XWayland drew the
  whole frame (molecule and corner text) shifted down.
- SoWxRenderArea.C defines both functions (Linux, Coin builds): Coin's calls
  bind to them before libGL's. A copy from a multisampled default
  framebuffer is first resolved with glBlitFramebuffer into a single-sample
  FBO. `ECCE_GL_RESOLVE_COPIES=0` turns it off for comparison.
- It reproduces only in a Viewer the gateway opens and only with a real click
  (keyboard focus): a Viewer started with -context and a scene script never
  peeled until an orbital was computed, and a headless GNOME Shell never
  showed the shift. `ECCE_GL_SAMPLES=0` and `ECCE_QUICK_TRANSPARENCY=1` both
  hide it; neither is a fix.
