---
type: pitfall
title: "Geometry constraints are edited in the Builder and stored with the calculation; the input uses them only for Geometry/GeoVib"
area: codereg
section: "Input writers"
paths: ["src/apps/builder/GeomConstraintsDialog.C", "src/apps/builder/Builder.C", "src/apps/calced/CalcEd.C", "src/apps/calced/ESInputController.C", "src/tdat/chemistry/GeomConstraintModel.C", "src/tdat/chemistry/GeomConstraintPick.C", "tests/apps/builder_constraints_test.py", "tests/apps/calced_constraints_line_test.py", "scripts/parsers/ai.ecceqm"]
issues: [306, 456, 461]
---
The constraint model (`GeomConstraintModel`) holds `TAtm*`, so it follows the atoms
of one fragment and must be rebuilt when that fragment object is replaced. The Builder
keeps one per open context (`Builder::constraintModel`), loads it from the calculation
when the context opens, drops atoms that no longer exist (`cleanUnmatchedAtoms`) and
writes it back only in `doSave` (`saveConstraints`). The Calculation Editor has no
model: it counts the stored constraints for its note and opens the Builder.

Two traps found on the way:
- `CalcEd::subjectMCB` (run when the Builder saves) used to call `clearGeomModel()`,
  which *deleted the stored file* and rebuilt from nothing. Nothing in the editor may
  write the geometry constraint model any more.
- `GeomConstraint::operator==` compares the atom *sets*, so `H2-N1-H3` and `H2-H3-N1`
  are "duplicates". The order is kept as picked; `orderConstraintAtoms` puts an angle's
  bonded centre in the middle before the constraint is made.

Which run types use the constraints is `GeomConstraintModel::usedByRunType` (Geometry,
GeoVib); `write_constraints` leaves the file out for any other, so the writers never
see them. The editor's note and Verify (`constraints` finding) say so.

The Settings line in the editor and the note in the Builder dialog both come
from `GeomConstraintModel::useBy(code, runtype)` and `notUsedText`: USED
(Geometry, GeoVib) or IGNORED (every other run type: no file is written).
ECCE-QM also ignores them for GeoVib, whose frequencies would be at a
constrained geometry. Nothing refuses: a warning never blocks.

The dummy atom X is shaped "Lone" in the Build panel, and `FragUtil::addToNub` returns
without doing anything for a Lone shape, so X could not be put on an open site;
`SGFragment::addAtom` now adds it to a nub as Terminal.
