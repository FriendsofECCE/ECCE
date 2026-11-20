---
type: pitfall
title: "Depth cueing hazes into the background colour; perspective keeps its own angle; SyncToggle names its Builder (#1214)"
area: wx-viewer
paths: [src/wxviz/viztools/ViewerEvtHandler.C, src/viz/sgcommands/SGContainer.C, src/inv/wxinv/SoWxViewer.C, src/apps/builder/Builder.C, src/wxviz/viewer/SceneScript.C, tests/apps/saveimage_test.py]
issues: [1214]
---
- **Fog colour.** `ChemDisplayParam::fogColor` defaults to black and
  `DepthCueCmd` sends 0,0,0, so depth cueing darkened a molecule on a white
  background (it looked like a lighting change). `ViewerEvtHandler::depthCue`
  passes the viewer's background colour, and every background change
  (`ViewerEvtHandler::setBackground`, the scene and Builder-script
  `background` hooks) calls `SGContainer::setDepthCueColor`. A new place that
  sets the background must do the same.
- **Perspective round trip.** `SoWxViewer::toggleCameraType` used to derive
  the perspective `heightAngle` from the ortho height, so ortho -> perspective
  never gave back the first picture. The angle before the switch is kept
  (`p_perspHeightAngle`) and the eye is moved along the view direction so the
  focal plane shows the ortho height (the zoom carries over).
- **SyncToggle is process-wide.** `EventDispatcher` reaches every Builder (tab
  mode: several in one process). `syncToggle` sets the wx event's object to
  its own Builder, and `Builder::eventMCB` ignores a SyncToggle from another.
- Test: `saveimage_toggles` in `tests/apps/saveimage_test.py` (on/off/on of
  both toggles, normal and hosted mode via `hosted_suite.py`).
