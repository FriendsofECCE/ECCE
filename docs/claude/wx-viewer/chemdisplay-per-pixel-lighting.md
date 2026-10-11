---
type: map
title: "Coin build: atoms and bonds are lit per pixel by a GLSL program that reads the fixed-function state; small atoms keep Gouraud; ECCE_GL_PERPIXEL=0 turns it off (#210)"
area: wx-viewer
paths: [src/inv/moiv/ChemPerPixel.C, include/inv/ChemKit/ChemPerPixel.H, src/inv/moiv/ChemDisplaySpheres.C, src/inv/moiv/ChemDisplayCylinders.C, include/inv/ChemKit/ChemDisplay.H, src/wxviz/viewer/SceneScript.C, tests/look/perpixel.py]
issues: [210]
---
`ChemPerPixel::begin/end` bind a GLSL 1.20 program around the sphere and
cylinder loops of `renderAtomsAsSpheres` / `renderBondsAsCylinders` (Coin
build only; the vendored build compiles stubs and renders byte-identically).
The program evaluates the fixed-function lighting equation per fragment
from the GL state (`gl_LightSource`, `gl_FrontMaterial`, `gl_LightModel`,
fog), with the diffuse from `gl_Color` because ChemDisplay tracks diffuse
with `glColorMaterial`. So element/property colours, the emissive
selection highlight, SGContainer's specular, stipple and blending all work
unchanged; only the shading moves (mean colour within ~1/255, same covered
pixels). `ftransform()` keeps depth equal to labels/isosurfaces.

- One program per GL context (Coin cache context id, dropped by a
  `SoContextHandler` destruction callback) and per light/fog state: the
  enabled lights, directional or positional, fog mode, local viewer, two
  side are baked in. A generic loop over eight lights with uniform
  switches was a third slower on llvmpipe.
- Not used for billboard/LOD sphere and LOD cylinder styles (textured quads,
  unlit points). `clipAtoms`/`clipBonds` are TRUE by default and only cull;
  that is the normal draw path, not a special one.
- Gate: on only when `atomPixelRadius` >= 12 px / `bondPixelRadius` >= 2 px
  (set per frame in `setupSpheres`/`setupCylinders`; the atom value uses the
  largest radius in the radii table, so it overestimates). On llvmpipe the
  cost is per triangle per varying: one extra vec3 varying cost the
  5184-atom water box ~+20% frame time even with a trivial lighting body;
  at its default zoom (8.5 px) the gate keeps it byte-identical to before.
- Fallback: GL < 2.0 or a compile/link failure logs once
  ("ECCE viewer: per-pixel lighting unavailable (...)") and that context
  stays fixed-function. `ECCE_GL_PERPIXEL=0` disables it.
- Coin's SORTED_LAYERS_BLEND peels with an ARB fragment program (depth
  texture on unit 3 via eye-linear texgen, kill unless 0.004 deeper); our
  program replaces it while atoms draw. For opaque atoms that changes no
  pixel (each layer redraws the nearest atom; an opaque layer hides what is
  behind). Emulating the peel test made no difference and was removed;
  transparent atoms would need it.
- Scene commands: `perpixel <name>` (on/off/unavailable for the last
  context), `stylecs <scheme>/<style>` (colour schemes). Test:
  `ctest -R look_perpixel` (needs `ninja viewer-scenes`).
