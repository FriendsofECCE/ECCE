---
type: map
title: "GROMACS Prepare, mineral source with ClayFF: the build flags, what the editor hides, and why the defines live in topol.top"
area: codereg
paths: [src/wxgui/mdtools/GromacsPreparePanel.C, include/dsm/GromacsPrepareModel.H, src/dsm/xml/GromacsPrepareModelXMLizer.C, scripts/minff_topology.py, scripts/parsers/md.gmxprep, tests/launch/gromacs_prepare_test.py]
issues: [930]
---
- **Choice.** The mineral box has "Force field: MINFF | ClayFF" (model
  `mineralForceField`, XML `MineralForceField`, default minff; an older task is
  MINFF). Changing it, the bend or the rigid-water tick marks the build stale.
  With ClayFF the variant, mineral, angle constant and ion-set rows are hidden;
  the M-O-H bend (`none`, `moh` Pouvreau 2017, `moh2004` Cygan generic; XML
  `ClayffBend`) and "Rigid water" (`ClayffRigidWater`) show instead.
- **Build.** `--forcefield clayff --clayff-bend X [--clayff-rigid-water]`, and no
  `--water`: the script forces SPC and argparse would refuse `spc`.
  `md.gmxprep` accepts WATER=spc for the mineral source (gensub's water box
  case already had it); the ions are the six of clayff.ff's ions.itp.
- **Defines are in `topol.top`.** `#define CLAYFF_MOH`, `CLAYFF_MOH_2004` and
  `RIGID_WATER` are written by the build, as the MINFF ion set is, so the
  staged topology carries them to the Prepare's genion/grompp and to every
  later task with no `.mdp` define; a later "Defines" entry in a task only adds.
- **Staging.** `clayff.ff/*.itp` are included files, found by
  `GromacsIncludes::collect` and stored as `clayff.ff__SLASH__<file>.itp`
  inputs like min.ff's; no `.gmxff` is needed (that is for `.rtp`/`.atp`
  files of imported force fields, which a topology does not include).
  `forcefield.doc` is not included, so it is not staged.
- **Warnings** (WARNING, never a block): the script's `warnings` (net charge not
  an integer), `types_missing_from_ff` and the keys of
  `charges_differing_from_atomipy`.
