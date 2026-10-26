---
type: rule
title: "The dipole arrow runs from the negative to the positive charge pole, value in the viewer corner (#578, was #266)"
area: wx-viewer
paths: [src/viz/propsgcommands/AddDipoleMomentCmd.C, src/viz/propsgcommands/AddVectorCmd.C, src/viz/nodes/VRVector.C, src/apps/builder/MomentsPanel.C, src/apps/builder/BuilderScript.C, tests/apps/builder_overlay_test.py, tests/apps/dipole_poles_test.py, help/src/dipole-moment.md]
issues: [266, 578]
---
Properties > Dipole Moment (and MP2 Dipole Moment) is `MomentsPanel`
(`data/client/config/PropertyPanelDescriptor.xml`); its `showVectors`
runs `AddDipoleMomentCmd`, which overrides `AddVectorCmd::execute` and adds
one `VRVector` under the property's switch.

- **Poles.** r+/r- = sum q r / sum q over the atoms of one sign. Charges:
  `ESPFIT` (CHELPG), else `MKFIT`, else `MULLIKEN`
  (`AddDipoleMomentCmd::chargeKey`); the panel names the set. No charges: a
  1 A arrow centred on the centre of nuclear charge.
- **Arrow.** Tail at r-, along mu as parsed (NWChem, ORCA, Gaussian 16 and
  MOPAC report mu = sum q r, negative to positive end; IUPAC direction), length
  = (r+ - r-) . mu-hat, so the head is level with r+ along mu where the pole
  line is skew to mu (Ac-Ala-NHMe, Mulliken: 0.14 A off). Length follows the
  geometry, not |mu|: no scale slider (it was Angstrom per Debye, 0.5, until
  #578). `VRVector` puts its tip at 1.075 |direction|; the command divides that
  out. Constant shape 0.04/0.10/0.20 A, drawn again through the molecule
  (`showThroughMolecule`, 60 % transparency, depth test off).
- **Value.** Not drawn in the scene: `MomentsPanel::showVectors` puts
  `Dipole moment μ = x.xx D (ESP|MK|Mulliken charges)` (`Dipole moment μ = 0 D` below
  0.005 D, CO2; the corner font is Latin-1 bitmaps, and `drawFixedPixelText`
  decodes UTF-8, drawing Greek μ as the micro sign) in `SGViewer::setURightText`; `loseFocus` clears it. A text
  node beside the arrow needed ASCII (no Greek mu in `SoText2`) and went stale
  on background changes.
- **Only the dipole panel draws it.** `QuadPolePanel` (quadrupole,
  hyperpolarizability, stress) draw no arrow (`drawsOverlay()` is false). The
  polarizability has its own drawing, see polarizability-ellipsoid-convention.md.

Measure, do not eyeball: the Builder script command `vector DIPOLE` prints
the drawn arrow's start, direction and length, the corner text (`label=`), the number of
`SliderCombo`s on the panel and the arrow count; `dipole_poles_test.py` (needs
a private install: `ECCE_TEST_HOME`, `ECCE_TEST_WRAPPERS`) recomputes the poles
from the stored charges and checks them, the label, CO2 and the missing slider.
The same script has `background r g b`, `viewall` and `rotate <deg>`.
