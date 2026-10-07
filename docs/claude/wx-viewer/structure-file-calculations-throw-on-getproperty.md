---
type: pitfall
title: "A structure opened from a file is not a calculation: AbstractPropCalculation::getProperty() throws"
area: wx-viewer
paths: [src/apps/builder/AbstractPropCalculation.C, src/apps/builder/BuilderPanels.C, src/apps/builder/Builder.C, tests/apps/run_tests.py]
issues: []
---
`p_calculation` in the Builder is any `IPropCalculation`. A new structure
(`DefaultCalculation`) and every structure opened from a file (`PdbCalculation`,
`XyzCalculation`, `CarCalculation`, `MvmCalculation`, `CubeCalculation`,
`TrajectoryCalculation`) derive from `AbstractPropCalculation`, whose
`getProperty()`, `putProperty()` and `deleteProperties()` throw
`NotImplementedException`. Nothing catches it: the Builder aborts with
"Unhandled standard exception of type NotImplementedException".

Guard property reads with `dynamic_cast<AbstractPropCalculation*>`, not with
`DefaultCalculation` alone: a check for "new structure" misses every file.
The list + detail header's total-energy line did exactly that and killed
`ecce-builder glycine.pdb` in alpha.6 (only in list + detail, the default
for a fresh install, so a developer with a saved Classic layout never saw it).

`tests/apps` (`checkStructureFiles`) opens a PDB in every layout and a CAR and
an XYZ in list + detail through the `ECCE_VIEWER_SCENE` hook, and fails on a
crash, a throw log, or no snapshot of the loaded atoms. An XYZ raises a modal
units prompt before the context is set; `ECCE_TEST_XYZ_UNITS=angstrom` makes
`WxUnitsPrompt::ShowModal` answer it without showing it.
