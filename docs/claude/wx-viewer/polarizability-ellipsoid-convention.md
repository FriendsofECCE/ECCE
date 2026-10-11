---
type: rule
title: "The polarizability is one translucent ellipsoid on the principal axes of the symmetrised tensor, centred on the centre of nuclear charge (#279)"
area: wx-viewer
paths: [src/viz/propsgcommands/AddPolarizabilityCmd.C, src/apps/builder/QuadPolePanel.C, src/apps/builder/MomentsPanel.C, src/apps/builder/BuilderScript.C, tests/apps/builder_overlay_test.py]
issues: [279, 266]
---
Properties > Polarizability is `QuadPolePanel` with property `POLARIZ`; only
it draws anything. Quadrupole, Hyperpolarizability and Stress show the table
(`drawsOverlay()` is `isPolarizability()`).

- **Shape.** `AddPolarizabilityCmd` adds one `SoSphere` under the property's
  switch, in an `SoTransform` (scale = semi-axes, rotation = the eigenvector
  matrix) and an `SoMaterial` with transparency 0.55. The semi-axis along
  eigenvector i is `eigenvalue_i x Amplitude`, in Angstrom, Amplitude being
  Angstrom per a.u. Same origin as the dipole arrow (`Center` = "charge").
- **Tensor.** `principalAxes()` symmetrises (a 9-value tensor is averaged
  with its transpose; the 6-value form xx xy xz yy yz zz is what the Gaussian
  parser stores), sorts the eigenvalues largest first and makes the axes
  right-handed (a left-handed set would make `SbRotation` a reflection).
- **Scale.** The default puts the longest semi-axis at 1.5 A. The panel's
  slider is in Angstrom per 10 a.u.: `SliderCombo` has two decimals, which
  would give water (about 7 a.u.) and benzene (about 75 a.u.) very different
  resolution if it were per a.u. The panel prints the eigenvalues, the
  isotropic mean and the semi-axes at the current scale.
- **Units.** The properties carry none; Gaussian and ORCA both report
  bohr^3 (a.u.), which is what the panel says. ORCA has no `POLARIZ` parser
  yet (`orca.desc`); the Gaussian one is `gaussian-16.db`.
- **Transparency.** The panel sets `SORTED_OBJECT_BLEND` on focus and
  `DELAYED_ADD` on losing it (as `MoPanel` does), so the atoms show through.
- **Measure, do not eyeball.** The Builder script command `ellipsoid PROP`
  prints the centre and each axis (semi-axis length, unit direction) from the
  matrix the renderer applies to the unit sphere; `builder_overlay_test.py`
  compares them with numpy's `eigh` of the stored tensor for water, CO2
  (long axis along the molecule) and benzene (in-plane axes equal, larger
  than the out-of-plane one), Gaussian 16 and ORCA 6.1.1 inputs.
