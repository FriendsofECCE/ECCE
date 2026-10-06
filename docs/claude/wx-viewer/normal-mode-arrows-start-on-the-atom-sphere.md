---
type: rule
title: "Normal-mode arrows start on the drawn sphere, have a fixed thickness, and the longest tip is 1 A out (#228)"
area: wx-viewer
paths: [src/viz/propsgcommands/NModeVectCmd.C, src/viz/nodes/VRVector.C, src/viz/sgcommands/SGContainer.C, src/viz/sgcommands/CSRadiiCmd.C, src/apps/builder/NModePanel.C, src/wxviz/viewer/SceneScript.C]
issues: [228]
---
An arrow that starts at its atom centre is hidden whenever its length is
below the drawn sphere radius (O 0.365 A, H 0.20 A, C about 0.38 A in Ball
And Stick), which is every heavy atom in an X-H stretch. So:

- **Scale.** The longest arrow's tip is 1.0 A from its atom (|d| times
  1/1.075/maxnorm; the tip of a `VRVector` is 1.075 |d|). All arrows keep
  their relative lengths. The panel slider overrides the scale (the
  `Amplitude` parameter) as an absolute value, not a multiplier of this one.
- **Start.** Each arrow starts on its atom's displayed sphere, along its
  own direction; the length is unchanged, so the tip moves out by the
  radius. `SGContainer::displayedSphereRadius` returns covalent radius x
  the style's sphere scale (half by default) in Ball And Wireframe and Ball
  And Stick, the CPK radius in CPK, 0 in Wireframe and for hidden
  atoms; in Stick the atom is the rod's rounded end, so the start radius is the
  bond cylinder radius (0.2 A by default).
- **Thickness.** `VRVector::fixedThickness(true)`: shaft radius 0.035 A,
  head radius 0.09 A, head length 0.18 A or half of a short arrow. Other
  users of `VRVector` (dipole, trajectory vectors) keep the old
  length-proportional shape. The POV-Ray export reads the old scale
  convention and does not follow the fixed thickness (a scale mismatch: it scales the shaft and head by |d|).

The start radius depends on the style, so `touchChemDisplay` (every style
change) and `CSRadiiCmd` call `SGContainer::updateNMVecStarts`, which
re-seats the existing arrows without rebuilding them. Arrows are matched to
atoms by child index in `getNMVecRoot()`.

Check by measurement, not arithmetic: the scene command `nmcheck <name>`
reports per atom the farthest arrow vertex from the atom centre
(`SoCallbackAction` triangles) and where a ray along the arrow meets that
atom's sphere (`SoRayPickAction`, atom detail); every tip must be outside
(`tools/coin/scenes/calc-water-nmcheck.scene`).
