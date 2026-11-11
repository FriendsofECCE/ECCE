---
type: map
title: "Batch Add from SMILES (#961): SmilesBatch builds each structure without the Builder and hands an xyz to Series::make"
area: services
paths: [src/dsm/edsiimpl/SmilesBatch.C, include/dsm/SmilesBatch.H, src/apps/launch/ecce-batch-smiles.C, src/apps/organizer/BatchSmilesDialog.C, src/apps/organizer/CalcMgr.C, src/tdat/chemistry/Smiles.C, tests/launch/batch_smiles_test.py]
issues: [961, 621, 867]
---
- `SmilesBatch::structure` repeats what `SmilesCmd` (viz, needs an
  SGContainer) does for Add from SMILES: `FragGraft::smilesTo3D`, bond
  orders from `BondOrderRule::perceive` (as `CleanCmd` assigns them, not the
  SMILES orders), then `ForceField`+`UFFDesc` with CleanCmd's defaults
  (100 steps, 1e-4, cutoff 5).  A change to the Builder's Clean should be
  mirrored here.
- Each row goes through `Series::make` as a temporary `<name>.xyz` with
  `charge=N` in the comment: the file stem is the calculation name, so names
  are made safe (no `/`, no leading dot) before.
- The default template "QSAR template" is created in the folder with
  theory/run type set directly (`TTheory("SE","GFN2-xTB")`, `Geometry`) and
  launch data for `localhost` (rundir from the machine's profile, else
  `~/ecce-runs`, as WxLauncher fills in). Without a rundir LaunchBatch::plan
  refuses ("no run directory stored").
- Measured values: `ecce:qsar.measured` (JSON, keyed by the column header
  with its unit, numbers as JSON numbers) and `ecce:qsar.smiles`; the
  annotation shows them too.  Job 2 (descriptor table) reads these.
- Column guessing: MOE exports carry ~100 numeric descriptor columns, so
  only activity-like headers (IC50, Ki, pIC50, ..., or a concentration unit
  in parentheses) are ticked; "logP(o/w)" is not a unit.
- MOE writes the charge before the H count (`[N+H]`); `Smiles::parse`
  accepts that order since #961 (OpenSMILES has H first).
- Organizer hooks: `batchsmilesdialog`, `batchsmiles ... [launch] [replace]`.
  A folder made on disk after the Organizer started is not in its tree:
  `launchSelected` then answers "no calculations".
