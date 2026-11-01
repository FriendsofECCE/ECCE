---
type: map
title: "Recipes (#710): a .recipe text file, applied by WorkflowSteps::applyRecipe; the set-up panel and the key-data box"
area: codereg
paths: [src/apps/organizer/Recipe.C, src/apps/organizer/KeyData.C, src/apps/organizer/RecipePanel.C, src/apps/organizer/WorkflowLayout.C, src/apps/organizer/WorkflowSteps.C, data/client/recipes, tests/recipes, tests/apps/workflow_test.py]
issues: [710, 674]
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
  only when launched: `launchReadySteps` calls `refreshFromSources`, which
  acts on recipe steps only so other workflows behave as before.
- The method and basis chosen per role are recorded as properties; only G16's
  `ES.Theory.DFT.XCFunctionals` is set from the method. A basis set needs a
  structure (per-atom assignment), so it is applied when one exists.
- `WorkflowLayout.C` is the one layout of boxes and right-angled links; the
  canvas and the set-up panel's preview both use it.
- Key data (`KeyData.C`) shows numbers only when every Complete step has the
  same code, method, basis and solvent; otherwise a note and no numbers.
- Test hooks: `workflowrecipelist`, `workflowrecipe`, `workflowrecipepanel`,
  `workflowrecipecreate`, `workflowkeydata`, `workflowmark`, `workflowlink`,
  `workflowsaverecipe` (CalcMgr.C); test `tests/apps/workflow_test.py`.
