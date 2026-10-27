---
type: pitfall
title: "An imported GROMACS run's structure comes from GromacsStructure::groToPdb, and trajectory steps are wrapped on load (#560)"
area: wx-viewer
paths: [src/apps/organizer/GromacsImport.C, src/dsm/chemistry/GromacsStructure.C, src/dsm/edsiimpl/PropertyTask.C, include/dsm/MdTask.H]
issues: [560]
---
- `GromacsImport.C` once had its own `.gro` -> PDB writer that took the first
  letter of the atom name as the element, so argon (residue AR, atom AR) was
  stored as "A", drew as nothing, and the 9.4.1 element fix in
  `GromacsStructure` never reached an imported run. It now calls the shared
  writer; keep it that way.
- The Viewer shows the last trajectory frame (`PropertyTask::loadStep`) on
  the stored structure's bonds. The frames are not whole across cell faces,
  so `loadStep` calls `wrapIntoCompactCell(false)` for tasks whose
  `wrapsStepsIntoCell()` is true (MD); the Geometry Trace stepper already did.
- `tests/apps/md_compare_examples_test.py` must wait for the big
  trajectories' `Props/GEOMTRACE` before stopping the Organizer; an import
  cut short leaves no structure, which looks like a slow Viewer.
