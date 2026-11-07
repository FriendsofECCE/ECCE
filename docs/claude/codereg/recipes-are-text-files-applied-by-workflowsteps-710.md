---
type: map
title: "Recipes (#710): a .recipe text file, applied by WorkflowSteps::applyRecipe; the set-up panel and the key-data box"
area: codereg
paths: [src/apps/organizer/Recipe.C, src/apps/organizer/KeyData.C, src/apps/organizer/RecipePanel.C, src/apps/organizer/WorkflowLayout.C, src/apps/organizer/WorkflowSteps.C, data/client/recipes, tests/recipes, tests/apps/workflow_test.py]
issues: [710, 674, 808, 847, 901, 902]
---
- Format and rules: `Recipe.H` (wx-free; tested by `recipe_test`). Built-ins
  are `data/client/recipes/*.recipe`; a site adds `Recipes/` in any site
  configuration layer (central server included) or `ECCE_RECIPE_DIRS`.
  Save Workflow as Recipe writes to the user layer's `Recipes/`.
- Steps are made by `WorkflowSteps::applyRecipe`: ordinary `*_es` children
  with the recipe's run type and ES.* keys, links named as in #674
  (`reactant`, `tsguess.max`, ...), and the QST/NEB picks stored at once
  (the picks are just calculation URLs, so no structure is needed). Each step
  gets `ecce:recipe.*` properties (step, role, key, thermo, method, basis);
  the workflow gets `ecce:recipe`, which is what makes the view draw the
  key-data box.
- A recipe step with no incoming link and no atoms is a placeholder
  ("needs structure"). Its successors get geometry, charge and multiplicity
  when launched, or before opening an empty step's Calculation Editor once
  all sources are Complete: `CalcMgr::startApp` calls `refreshFromSources`
  before dispatching the editor request (#847). It checks the step's own
  fragment without input-provider fallback and preserves existing structures
  and launched steps. `launchReadySteps` still refreshes at launch. Both
  paths act on recipe steps only so other workflows behave as before.
- Ready without the editor (#901): `refreshFromSources` ends in `prepareStep`,
  which writes a Created recipe step's deck (`DeckWriter::generate`) once all
  sources are Complete and it has atoms, sets Ready, and copies launch settings
  from a source (else another step) when it has none. Called after Apply
  Recipe, when a step finishes, and on every launch path. Why not is stored as
  `ecce:workflow.notready` (play tooltip, `workflowplay`, `workflowstate`).
  A `workflowcopy` seed has no launch settings (StoredCalcCopy strips them),
  so the first step after it shows "no launch settings" until one is set.
- Launches from the workflow view must not use `findNode(url, true, ...)`:
  openWhenFound calls SelectItem, which replaces the view with the step's
  panel (#902, auto-run tick). Hook: `workflowautorunclick`.
- The method and basis chosen per role are recorded as properties.
  `applyRoleMethod` sets theory RDFT plus `ES.Theory.DFT.XCFunctionals` (or
  SCF/RHF for HF) for G16, ORCA and ECCE-QM only (ECCE-QM: theory DFT/DFT or SCF/HF, functionals SVWN, PBE, B3LYP, PBE0), through `methodValue`, which holds
  copies of the functional lists in `ged16theory.py`/`orcatheory.py`/`ecceqmtheory.py`: add a
  functional there and here. Other codes keep their default.
- `structureSet` runs when a recipe step's structure is stored: the
  Organizer's `ecce_url_subject` subscription (the Builder publishes it on
  save) calls `CalcMgr::structureStored`, and `refreshFromSources` calls it
  after carrying a source's structure. It re-runs `recommend` with the
  structure for every not-submitted recipe step whose level is still the
  recommendation (blank-facts default or the last ruled value in
  `ecce:recipe.ruled`), then applies method and basis. A basis set needs a
  structure (per-element assignment), so `applyRoleBasis` stores the role's
  basis when the step gets one (at creation from the box it is appended to,
  or from its source at launch); `6-31+G(d)` is looked up as `6-31+G*` too.
- `WorkflowLayout.C` is the one layout of boxes and right-angled links; the
  canvas and the set-up panel's preview both use it.
- Key data (`KeyData.C`) shows numbers only when every Complete step has the
  same code, method, basis and solvent; otherwise a note and no numbers.
- Test hooks: `workflowrecipelist`, `workflowrecipe`, `workflowrecipepanel`,
  `workflowrecipecreate`, `workflowrecipehere` (the box menu's Apply Recipe from here), `workflowkeydata`, `workflowmark`, `workflowlink`,
  `workflowsaverecipe`, `workflowstructure` (CalcMgr.C); test `tests/apps/workflow_test.py`.
- Step flag `charge=+N` (`RecipeStep::chargeShift`, #808): `carryInto` adds N to
  the fragment charge it copies from the source (at creation after a box and in
  `refreshFromSources`, which re-copies each time, so it never adds twice). Only
  the first step after CREST Protonation Sites needs it; later links copy the
  shifted charge. CREST ensemble links use `geometry.c1` (the lowest conformer):
  the ensemble frames are in CREST's file order, so plain `geometry` is the
  last frame, not the lowest. The Solvation Shell result is one frame (the
  grown cluster), so plain `geometry` carries solute plus solvent; its CREST
  step's QCG solvent follows the dialog's Solvent row for water, methanol and
  acetonitrile (`applyRecipe`).
- Recipe step flag `field` (with `fragment`, #201): a one-atom, non-free fragment
  gets a preceding step `<id>-N-field` (`RecipeStep::field`, prop
  `ecce:recipe.field`), linked `geometry` from the complex and `orbitals` to the
  fragment step. Its atom runs in point charges of -0.1 e at the complex's
  non-H atoms within 1.25 covalent radii of it (`storeFieldCharges`, stored as the
  input `pointcharges.pc`, `ES.Theory.SCF.PointChargeFile`, `%pointcharges` in
  `ai.orca`); `refreshFromSources` attaches its `.gbw` as the fragment step's
  MORead guess once it is Complete. Measured, ORCA 6.1.1, def2-SVP, Cr(0) in 6
  point charges: -0.1 and -0.15 e give t2g6, -0.2 e and stronger give d4 4s2
  (the point charges raise 3d more than 4s). Co(III) is t2g6 at any of these.
  The bare ion's RHF d set is an arbitrary subset, which is why the step exists.
  `fragmentSteps` skips field steps.
