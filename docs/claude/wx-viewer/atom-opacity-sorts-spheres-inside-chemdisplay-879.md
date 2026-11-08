---
type: map
title: "Atom opacity: ChemDisplay sorts the spheres itself, back to front, with a constant-alpha blend (#879)"
area: wx-viewer
paths: [src/inv/moiv/ChemDisplaySpheres.C, include/inv/ChemKit/ChemDisplayParam.H, include/tdat/DisplayDescriptor.H, src/viz/sgcommands/SGContainer.C, src/wxviz/viztools/VizStyleChooser.C, src/wxviz/viewer/SceneScript.C, tests/apps/atom_opacity_test.py]
issues: [879]
---
Opacity is a property of the display style (`DisplayDescriptor::getAtomOpacity`,
percent, style string key `AO`, preference `AtomOpacity`), copied to
`ChemDisplayParam::atomOpacity` in `SGContainer::touchChemDisplay`. "The
selection only" is therefore a style group of its own, as for every other
style setting. Coin sorts whole shapes, so `renderAtomsAsSpheres` does it:
below 1 it sorts the atoms of the normal and highlight lists by eye depth
(modelview z), then calls the same 18 draw loops once per atom, far to near.

- Blending is `glBlendColor` alpha with `GL_CONSTANT_ALPHA`, because the loops
  set colour with `glColor3f`; it works with the per-pixel shader unchanged.
  The vendored core has no such entry point and stays opaque.
- Depth writes stay on. ChemDisplay draws the bonds first, so a bond behind a
  sphere shows through it and one in front hides it. Bonded and non-bonded
  atoms are two calls and are sorted separately.
- The result depends on the view: auto caching is switched off while an atom
  is transparent. The opaque path (opacity 100) is the old code.
- Scene hooks: `opacity <0-100> [selection]`; `dsstyle info` reports
  `atom_opacity`. Test: `tests/apps/atom_opacity_test.py`.
