---
type: rule
title: "The dipole arrow is 0.5 A per Debye from the centre of nuclear charge, along mu (negative to positive end) (#266)"
area: wx-viewer
paths: [src/viz/propsgcommands/AddDipoleMomentCmd.C, src/viz/propsgcommands/AddVectorCmd.C, src/viz/nodes/VRVector.C, src/apps/builder/MomentsPanel.C, src/apps/builder/BuilderScript.C, tests/apps/builder_overlay_test.py]
issues: [266]
---
Properties > Dipole Moment (and MP2 Dipole Moment) is `MomentsPanel`
(`data/client/config/PropertyPanelDescriptor.xml`); its `showVectors`
runs `AddDipoleMomentCmd`, and `AddVectorCmd::execute` adds one `VRVector`
under the property's switch. Nothing else draws it.

- **Length.** The panel's value is Angstrom per Debye (default
  `AddDipoleMomentCmd::ANGSTROM_PER_DEBYE`, 0.5): the tip is exactly that
  times |mu| from the origin. `VRVector` puts its tip at 1.075 |direction|,
  so the command divides that out; with the old scale 1.0 the tip was
  1.075 A/D, about 2.4 O-H bonds for water.
- **Origin.** `Center` = "charge": the centre of nuclear charge
  (Z-weighted). The "mass" branch of `getSingleCenter` is the
  mass-weighted centre (`TAtm::getMass`); nothing selects it yet.
- **Only the dipole panel draws it.** `QuadPolePanel` (quadrupole,
  hyperpolarizability, stress) draw no arrow (`drawsOverlay()` is false):
  no vectors, no colour or scale row. The polarizability has its own
  drawing, see polarizability-ellipsoid-convention.md.
- **Direction.** Drawn along mu as parsed. NWChem, ORCA, Gaussian 16 and
  MOPAC all report mu = sum q r (negative to positive end); checked
  against each code's geometry for water. All parsers store Debye
  (NWChem and ORCA convert from a.u.).
- **Shape.** Constant shaft 0.06 A, head 0.15 x 0.30 A
  (`VRVector::fixedThickness(shaft, head, length)`).

Measure, do not eyeball: the Builder script command `vector DIPOLE`
prints the drawn arrow's start, direction, farthest vertex along the axis
(length) and from it (radius); `builder_overlay_test.py` checks them.
- **Through the molecule.** A diatomic's arrow lies inside the bond, so the
  atoms hid it. `AddDipoleMomentCmd::shapeVector` calls
  `VRVector::showThroughMolecule()`: a second pass over the same shaft and
  head, after the opaque one, with the depth test off (`SoCallback` with raw
  `glDisable/glEnable(GL_DEPTH_TEST)`) and 60 % transparency. The first pass
  is unchanged; the display style is never touched. In the viewer's usual
  screen-door transparency the dimmed pass is a 40 % stipple.
  `builder_overlay_test.py --only hf` counts the arrow's yellow pixels (85
  without the pass, 429 with it).

