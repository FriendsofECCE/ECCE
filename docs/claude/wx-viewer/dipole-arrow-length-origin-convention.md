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
  polarizability, stress) overrides `drawsArrow()` to false: no vectors, no
  colour or scale row.
- **Direction.** Drawn along mu as parsed. NWChem, ORCA, Gaussian 16 and
  MOPAC all report mu = sum q r (negative to positive end); checked
  against each code's geometry for water. All parsers store Debye
  (NWChem and ORCA convert from a.u.).
- **Shape.** Constant shaft 0.06 A, head 0.15 x 0.30 A
  (`VRVector::fixedThickness(shaft, head, length)`).

Measure, do not eyeball: the Builder script command `vector DIPOLE`
prints the drawn arrow's start, direction, farthest vertex along the axis
(length) and from it (radius); `builder_overlay_test.py` checks them.
