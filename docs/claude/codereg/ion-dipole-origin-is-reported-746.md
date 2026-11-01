---
type: rule
title: "An ion's dipole depends on the origin: ECCE-QM reports it about the centre of mass and stores DIPOLEORIGIN (#746)"
area: codereg
paths: [src/qm/scf.cpp, src/qm/output.cpp, scripts/parsers/ecceqm.parse, scripts/parsers/orca.dipoleorigin, src/apps/builder/MomentsPanel.C, tests/qm/unit_test.cpp]
issues: [746]
---
`ScfResult::dipole` stays about the coordinate origin (finite-difference dipole
derivatives and polarizabilities use it); `dipole_rep` is what `dipole_au`/
`dipole_debye` print: shifted by mu' = mu - qR to the centre of mass when the
charge is not 0, unchanged for neutrals (`dipole_origin independent`). The
`dipole_origin <kind> x y z` line becomes `DIPOLEORIGIN` (PropString:
`centre_of_mass`, `centre_of_charge`, `coordinate_origin`); `MomentsPanel` adds
"(about the ...)" to the |mu| text. ORCA: `orca.dipoleorigin` reads "Choice of
electric origin" from the property settings. Gaussian 16 and the other codes state
no origin in the output, so nothing is stored for them.
