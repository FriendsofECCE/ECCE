---
type: map
title: "Rate box and Reaction overview [ts] (#872): the GUI runs `ecce-rate --json`, no formula in C++"
area: codereg
section: "How a property gets from disk into the Properties menu"
paths: ["src/util/genutil/RateRun.C", "src/apps/organizer/RateBox.C", "src/apps/organizer/RateDetailsDialog.C", "src/apps/organizer/ReactionOverviewDialog.C", "src/apps/organizer/EnergyProfile.C", "data/client/examples/workflows/sn2-cl-ch3br/workflow.txt", "tests/apps/rate_gui_test.py"]
issues: [872]
---
`runRate` (RateRun.C) runs `$ECCE_HOME/bin/ecce-rate ... --json 2>&1` and reads
the JSON with a small parser; absent values are NaN, a refusal is the
`error:` line. It takes folders, so only `file://` or absolute calculation
URLs work (server calculations are reported as unsupported).
- The Rate box is drawn by `WorkflowCanvas` for any workflow whose steps carry
  `recipe.key` reactant/ts (product optional); `keySteps` accepts a step with
  only `recipe.key`. Example workflows get the keys from `!key <folder> <role>
  [thermo]` lines of workflow.txt (`joinExampleWorkflow`). ecce-rate reruns only
  when a key step's url/completion date or the T field changes.
- The overview sends ecce-thermo two lines for `a -> [ts] -> c`: `a -> c` and
  `a -> ts`; the second gives dE‡ and dG‡, so they equal the thermo numbers.
- Hooks: `workflowrate WF [T=K] [png=F] [csv=F] [range=L:H:S]`, and
  `reactionoverview ... [png [csv [T]]]` prints a `Rates:` block.
- Only `[ts]` reactions get a chart: an energy profile (EnergyProfile.C). Its
  labels are placed by collision checks against the drawn segments and other
  labels, growing the empty band above/below the levels when nothing is
  free. The layout is computed apart from painting so `reactionoverview`
  (`layout P ...` lines) and `reactionprofile dEts dGts dEp dGp [png]` can
  print it; tests/apps/profile_layout.py checks it for overlaps
  (apps_reaction_profile_layout, apps_rate_gui). Change the layout and those
  tests together.
