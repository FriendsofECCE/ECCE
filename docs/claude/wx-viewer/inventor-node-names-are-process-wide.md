---
type: pitfall
title: "Inventor node names are process-wide: look a scene graph up among your own children (#1209)"
area: wx-viewer
paths: ["src/wxviz/viewer/SGContainerManager.C", "include/wxviz/SGContainerManager.H", "src/apps/builder/BuilderWindow.C", "tests/apps/one_process_test.py"]
issues: [1209, 983, 1139]
---
`SoNode::getByName()` searches every named node in the process. Each
Builder/Viewer names its per-calculation `SGContainer` after the URL, so
with tools as tabs (#983) a second tool on the same calculation, or a
Viewer reopened while the closed one's container is still alive, found
the other tool's container and never created its own: the fragment loaded
(atom count right, Properties fine) into a container that is not under
this viewer's root, and the 3-D view was empty. `SGContainerManager`
now searches only its own children (`findChild`). Any new lookup of a
scene node by name has the same trap.
Test: `tests/apps/one_process_test.py --1209` (`builderinfo` reports
`inscene=1` when the fragment's graph is under the viewer's root).
