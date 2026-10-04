---
type: map
title: "Builder Reset View (toolbar, Render menu, Home key) is camera only; the old Go-to-home button restored a pre-molecule camera"
area: wx-viewer
paths: [src/apps/builder/Builder.C, src/inv/wxinv/SoWxViewer.C, src/wxviz/viewer/SceneScript.C]
issues: [166]
---
`SoWxViewer::saveHomePosition` runs when the camera is attached, before any
molecule is loaded, so the 16px "Go to home view" button in the rotator row
restored a camera that does not frame the system (until "Set home view" was
used). `Builder::OnToolResetViewClick` (View toolbar, Render menu "Reset
View", accelerator Home) calls `resetToHomePosition()` then `viewAll()` and
zeroes the X/Y/Z rotation spinners: orientation back to the home one and the
whole system in view; atoms and edits are not touched. Scene command
`resetview` (labels.scene) shows it: after `rotate`, the snap equals the
first one on Coin (0 differing pixels).
