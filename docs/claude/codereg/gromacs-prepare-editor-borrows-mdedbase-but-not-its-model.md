---
type: pitfall
title: "GROMACS Prepare editor (gmxprepare): MDEdBase assumes an MDCompositeModel, and a tool listed in the wrong resource type opens beside NWChem's"
area: codereg
paths: [src/apps/gmxprepare, src/wxgui/mdtools/GromacsPreparePanel.C, src/wxgui/mdtools/MDEdBase.C, data/client/config/ResourceDescriptor.xml, data/client/config/ResourceDescriptorRxn.xml]
issues: [930]
---
`GmxPrepare` derives from `MDEdBase` for the window, messaging, Save/Launch
buttons and the `ECCE_TEST_MDED` hook, but its model is a
`GromacsPrepareModel`, not an `MDCompositeModel`. What that costs:

- `MDEdBase::getMDModel()` casts `p_model` unchecked. The Builder, Final Edit,
  Launch and edit-completion handlers used it only for the URL and now read
  `p_model->getUrl()`; anything new in the base that calls `getMDModel()` will
  read a `GromacsPrepareModel` as an `MDCompositeModel`. `GmxPrepare` overrides
  `setContext`, `saveModelAndInputFile`, `generateInputFile` and Reset All
  rather than reaching those paths.
- The base builds the Inputs page (topology, structure, includes) for every
  editor. A Prepare makes those files, so `GmxPrepare::constructor` destroys it;
  with it left in, `enableLaunch` would keep Launch off for want of a topology.
- `generateInputFile` runs `md.gmxprep` by name. The code's
  `TaskInputGenerator` is `md.gmxtask` (the `.mdp`), and `getCodeFile(PRIMARY_INPUT)`
  returns the first primary input, `gromacs.mdp`. Use neither for a Prepare.
- The structure choice is not in the model: a stored `*.gmxpdb` means "PDB
  file", none means "from the Builder" (Launch writes `<sys>.pdb`). The panel
  stores or removes the `.gmxpdb` as the choice changes, not on Save.
- `runTestCommand` was non-virtual; editors add commands by overriding it
  (`set`, `fields`, `recipe`, `model` for the Prepare).

The `GMXPrepare` tool had been listed under `nwchem_md_prepare`'s enabled
tools as well as `gromacs_md_prepare`'s, so an NWChem Prepare offered two
editors; and its `<Tool>` definition (label, `./gmxprepare`) was missing, so
the Organizer had nothing to start. Both resource descriptors carry the
definition now and only the GROMACS type enables it. The Organizer hook
`summary`/`toolsmenu` shows the buttons; `tests/launch/gromacs_prepare_test.py`
checks both codes.

The force-field and water-model lists in the panel are a static table from
GROMACS 2025.2's `*.ff/forcefield.doc` and `watermodels.dat` (the editor must
not need a local `gmx`); a new GROMACS release that adds a force field needs
the table extended, and "Other (GMXLIB)" covers it meanwhile.

Merge chains (`-merge all`) is on by default now (model, `md.gmxprep` and
`gensub` agree); a recipe written before that has `MERGE=0` stored.
