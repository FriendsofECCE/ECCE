---
type: map
title: "The shared docking layer (#682, #455): receptor/ligand split in the .param, a pose ensemble as GEOMTRACE + POSESCORE, best-N through ScanPoint"
area: codereg
section: "Getting a calculation set up (the \"code registration\" system)"
paths: ["scripts/parsers/docking.pl", "scripts/parsers/dock.poses", "scripts/parsers/ai.xtb", "scripts/gensub", "scripts/parsers/xtb.desc", "data/client/config/properties", "src/dsm/edsiimpl/ScanPoint.C", "src/apps/organizer/WorkflowSteps.C", "tests/launch/dock_test.py"]
issues: [682, 455]
---
What a second docking code (AutoDock Vina, #455) reuses; xtb is the first user.

- **Receptor/ligand split** lives in the `.param` of the calculation, so any
  `ai.<code>` reads it: `ES.Runtype.Docking.LigandAtoms` (1-based atoms in the
  Builder's order, `1-5,9`; empty = the smallest connected molecule) and
  `ES.Runtype.Docking.LigandCharge`. `scripts/parsers/docking.pl` `dockSplit()`
  turns that into two index lists; `require "docking.pl"` in the ai script.
  Alkali and alkaline-earth ions use ionic radii in the bond test, else a K+
  inside a crown ether counts as bonded to it. There is no atom-pick UI yet:
  the Runtype Details dialog takes the numbers as text.
- **Deck**: option line, then `$order` (atoms listed receptor first, then
  ligand), `$receptor` and `$ligand` blocks, each a complete xyz file, ended
  by `$end`. A code that wants the ligand last in its own file format needs
  the same order list, because a pose comes back in that order.
- **Pose ensemble** = the output carries, per pose, a frame marker (`ECCE-DOCK-FRAME`
  for `dock.poses`; xtb reuses `ECCE-XTB-FRAME`, because a property key can have only
  one section in a `.desc` and GEOMTRACE already has xtb's, so `xtb.geomtrace` also
  emits POSESCORE) with `n natoms`,
  `score: <value>`, the atoms of the whole system **in the calculation's own
  atom order**, `ECCE-DOCK-ENDFRAME`. `scripts/parsers/dock.poses` emits
  GEOMTRACE and `POSESCORE` (class NA, kcal/mol, `PropTSVector<Geometry
  Step>`, label "Docking Pose Score" in `data/client/config/properties`).
  The Geometry Trace x axis reads "Pose" (`StepPlotAxes.H`). A code whose score
  is another quantity (Vina's empirical score) registers its own property key
  with its own label and unit and adds it to the key list in
  `ScanPoint.C` `ensembleKeys`; nothing else changes.
- **Best N**: `ScanPoint` treats CONFREL or POSESCORE the same way, a rank
  `cK` = K-th lowest value (a more negative score is better, ranked first).
  `WorkflowSteps::createBest` writes "Pose K of N ... score" for poses
  (`ScanPoint::ensembleIsPoses`). Test: `workflow_test.py` `run_best_poses`.
- **xtb**: run type `Docking (aISS)`, `xtb dock receptor.xyz ligand.xyz`.
  gensub takes the poses from `optimized_structures.xyz`, else
  `best_after_gen.xyz` (the genetic algorithm's best pose); the comment line
  of a pose ends in its interaction energy.
- **xtb 6.7.1 (Debian 13) aborts** in `xtb dock`, after the genetic
  optimisation ("stack smashing", GFN2; "allocate already allocated 'neigh'",
  GFN-FF), exit 134 or 2, leaving only `best_after_gen.xyz`. The format of
  `optimized_structures.xyz` is therefore unchecked against a successful run.
  `tests/launch/dock_test.py` probes for that and runs a stand-in xtb script
  when the real one aborts.
