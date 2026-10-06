---
type: map
title: "Normal-mode arrows start at the atom centre and scale uniformly; small ones end inside the sphere (#228)"
area: wx-viewer
paths: [src/viz/propsgcommands/NModeVectCmd.C, src/viz/nodes/VRVector.C, src/apps/builder/NModePanel.C, src/wxviz/viewer/SceneScript.C]
issues: [228]
---
`NModeVectCmd` puts one `VRVector` per atom at the atom centre, direction
`d * 0.5/maxnorm` (Scale slider multiplies it). `VRVector` scales its whole
geometry (shaft, head, thickness) by |d|; the tip is at 1.075 |d|, so the
longest arrow reaches 0.5375 A from its centre.

Displayed sphere radii (Ball And Stick and the default style) are covalent
x 0.5: O 0.365, H 0.20, C about 0.38. An atom whose displacement is below
about 70 % of the largest one (any heavy atom in an X-H stretch) has its
whole arrow inside its sphere. Separately, in Ball And Stick an arrow
pointing along a bond lies inside the bond cylinder and is hidden too (the
water symmetric stretch shows no arrows at all with Sign unticked).

Measured, not computed: the scene command `nmcheck <name>` reports per atom
the farthest arrow vertex from the atom centre (`SoCallbackAction`
triangles) and where a ray along the arrow meets that atom's sphere
(`SoRayPickAction`, atom detail); `tools/coin/scenes/calc-water-nmcheck.scene`.
