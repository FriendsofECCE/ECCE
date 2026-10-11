---
type: map
title: "Atom sphere and bond cylinder tessellation: the quality sliders are a floor, the rest follows screen size within a vertex budget (#210)"
area: wx-viewer
paths: [src/inv/moiv/ChemDisplaySpheres.C, src/inv/moiv/ChemDisplay.C, src/inv/wxinv/SoWxViewer.C, src/inv/moiv/ChemDisplayCylinders.C, src/inv/moiv/ChemUnitSphere.C, src/inv/moiv/ChemUnitCylinder.C, src/tdat/chemistry/DisplayDescriptor.C, src/viz/sgcommands/SGContainer.C, src/wxviz/viztools/VizStyleChooser.C]
issues: [210, 224]
---
The default Ball and Stick draws atoms as camera-facing hemispheres
(`ATOMSPHERE_HEMISPHERES`, immediate mode, one `ChemUnitSphere` for all atoms,
no display list: `DISPLAYLIST` is never defined) and bonds as
`BONDCYLINDER_SOCYLINDER_ROUNDCAP`.

- Sphere: `DisplayDescriptor::SPHERERQ` (Sphere Quality, 50) ->
  `atomSphereComplexity` 0.5 -> octahedron subdivision depth 5
  (`ChemUnitSphere.C computeDepth`: depth = 1 + 9c below 0.5, -20 + 50c
  above), i.e. a 20-gon silhouette (4 x depth edges). Visibly faceted once
  an atom is about 40 px in radius.
- Bond: until #210 the SOCYLINDER styles ignored `bondCylinderComplexity`
  (Cylinder Quality) entirely; both builds drew 16 sides from the traversal
  state's `SoComplexity` default (0.5). Now `renderSoCylinder` draws one
  section with `ChemUnitCylinder::soSides`; `CYLRQ` became 50 (= 16 sides) so
  the default did not get slower.
- Per frame, `ChemDisplay::maxPixelsPerUnit` projects every atom centre in
  view and the depth/sides are raised until the polygon stays within 0.25 px
  of a circle of the largest atom/bond radius on screen
  (`polygonSidesFor`), capped so atoms x vertices <= 300k (bonds likewise).
  The slider value is never undercut. Effect: small molecules are smooth at
  any zoom; the 5184-atom water box keeps depth 5 and is unchanged (byte-
  identical renders, same frame time), including when zoomed in, where the
  nearest atoms still show the 20-gon. A per-atom LOD would fix that; the
  `ATOMSPHERE_LOD` path has one but `computeAtomBBoxes` sets
  `lastSphereComplexity` early, so `ChemLOD::changeSphereComplexity` is never
  called for it.

The Display Style dialog's sliders only change their number label; nothing
is applied until Update (or Add). After Update the view redraws by itself
in both builds (no rotation needed): `stylerq` + `idleframes` in a scene
script show one idle frame identical to a forced paint.

Specular: in the Coin build ChemDisplay's own `glMaterial` is overwritten
by the lazy element's send of SGContainer's `p_cdMat` SoMaterial; in the
vendored build it is not. Both carry the same values (0.4 grey, shininess
0.3 = 38.4 in GL); change both together.

Edges: `SoWxViewer::CreateControls` asks for 4x multisampling
(`ECCE_GL_SAMPLES`, 0 = off; off by default under `LIBGL_ALWAYS_SOFTWARE`).
Xvfb + llvmpipe grants it, but `snap`'s pre-swap `glReadPixels` does not
show the resolved edges on Mesa's software winsys; an `xwd` grab of the
screen does. So headless snaps cannot judge antialiasing.
