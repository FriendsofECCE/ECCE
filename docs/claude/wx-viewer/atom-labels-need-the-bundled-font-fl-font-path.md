---
type: pitfall
title: "Atom labels drew nothing in 9.x: the sh wrappers no longer set FL_FONT_PATH, so flclient found no font (both builds)"
area: wx-viewer
paths: [src/inv/flclient/fl.c, src/inv/moiv/ChemBitmapFontCache.C, tools/coin/scenes/labels.scene, tools/coin/compare.py]
issues: [166]
---
Labels (atom, bond, residue, measures) are `glBitmap` text from FreeType
bitmaps made by `src/inv/flclient` (`ChemBitmapFontCache`); this is the same
on the vendored and the Coin build. `flCreateFont` looks for `Utopia-Regular`
in `$FL_FONT_PATH`, else in `/usr/share/fonts` (where it is not), and
`ChemBitmapFontCache` then silently keeps zero characters: no error, no text.

The old csh `ecce_env` exported `FL_FONT_PATH`; the 9.x sh wrappers do not,
so labels were blank in every packaged build, vendored and Coin alike
(measured under Xvfb: 0 label pixels on both without the variable, 1343
for "Element with Number" on benzene on both with it). The font does ship
(`/opt/ecce/data/client/fonts/Utopia-Regular`). Fixed in `fl.c`: with
`FL_FONT_PATH` unset the path is `$ECCE_HOME/data/client/fonts`.

Why it hid: `tools/coin/compare.py` exported `FL_FONT_PATH` itself, and two
builds that both draw nothing are "pixel-identical". The harness no longer
sets it, and `labelCheck` in `compare.py` (scene `labels.scene`) reports
`NO TEXT` for any label option that puts no label-coloured pixel on screen.
"Name", "Type" and residue labels are empty for a plain XYZ fixture (no names
or residues in the file), so they are only meaningful on a loaded
calculation (`calc-water.scene`).
