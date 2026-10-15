---
type: map
title: "While dragging, systems over 2000 atoms draw spheres and cylinders at low complexity (#343)"
area: wx-viewer
paths: [src/wxviz/viewer/SGViewer.C, include/wxviz/SGViewer.H, tools/viewer-bench/viewer-bench.C]
issues: [343]
---
moiv does not read `SoComplexityElement`, so `SoWxViewer`'s interactive draw
style never reaches the atoms. `SGViewer` registers viewer start/finish
callbacks; on start, if the scene holds 2000 atoms or more, every
`ChemDisplayParam`'s `atomSphereComplexity` is set to 0.2 and
`bondCylinderComplexity` to 0.1 (only lowered, never raised), and the saved
values go back on finish, which redraws at full quality. Changing them only
rebuilds the unit sphere/cylinder and the LOD spheres, not per-atom caches.
The complexity is a floor under the screen-size tessellation, so on a
10k-atom system the cap keeps the floor in force. A press without a drag
costs two redraws. `BENCH_DRAG=1 BENCH_SYSTEM=clay BENCH_STYLES="Ball And
Stick" tools/viewer-bench` times drag frames (clay = 10164-atom Si/O lattice).
