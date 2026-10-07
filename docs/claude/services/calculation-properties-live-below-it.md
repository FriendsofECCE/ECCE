---
type: pitfall
title: "A calculation's molecule and basis properties are stored below it"
area: services
section: "Pitfalls"
paths: ["src/dsm/edsiimpl/FileEDSI.C", "src/dsm/dav/DavEDSI.C", "src/dsm/edsiimpl/TaskJob.C", "src/dsm/edsiimpl/ChemistryTask.C", "src/dsm/edsiimpl/DavCalculation.C", "src/apps/organizer/CalculationContextPanel.C", "tests/apps/summary_test.py", "tests/filedsi/resourceTest.C"]
issues: [216]
---
**A calculation's molecule and basis properties are stored below it.**
`empiricalFormula`, `numAtoms`, `numElectrons`, `symmetrygroup` go on
`Parameters/chemsys.mvm` (ChemistryTask), and the basis `name`,
`coordsys`, `numFunctions`, `numPrimitives` on
`Parameters/BasisSet.ecce_basisset` (DavCalculation), not on the
calculation. The Organizer's summary panel reads them from the
calculation: `TaskJob::updateProps` asks `getMetaData(..., true)`, and
for a virtual document (`ecce:resourcetype` = `virtual_document`) the
EDSI appends the ecce properties of everything below it. DavEDSI does it
with a depth-infinity PROPFIND (`getVirtualMetaData`); FileEDSI walks
the `.ecce-meta` sidecars below the calculation, skipping nested virtual
documents. Before FileEDSI did, every local-data calculation showed an
empty Formula/Atoms/Electrons/Symmetry and basis row while Theory and
Runtype, stored on the calculation, showed.

Tests: `tests/filedsi` (`resourceTest summary`),
`tests/apps/summary_test.py` (both modes, through the Organizer).
