---
type: map
title: "Thermochemistry contributions and Energy Calculator temperature (#1044, #1043): one implementation, JSON fields read by ThermoRun"
area: codereg
section: "How a property gets from disk into the Properties menu"
paths: ["src/tdat/thermo/ThermoChem.C", "src/apps/thermo/ecce-thermo.C", "src/util/genutil/ThermoRun.C", "src/apps/builder/ThermoPanel.C", "src/apps/organizer/CompareEnergiesGather.C", "src/tdat/reaction/EnergySums.C", "scripts/parsers/ecceqm.desc", "scripts/parsers/ecceqm.parse", "tests/apps/thermo_panel_test.py", "tests/apps/thermo_temperature_test.py"]
issues: [1044, 1043]
---
`computeThermo` fills U (thermalTrans/Rot/Vib, hartree), S and Cv per
contribution; `ecce-thermo --json` appends them to each treatment's object
after H, S, G on the same line, and `runThermo` reads them by key from that
line, so the panel's second grid and the CLI cannot disagree. Cv_vib follows
the treatment's thermal energy (Truhlar raised modes, Head-Gordon damped
toward R/2), so Cv = dU/dT holds for every treatment (unit-tested).
`ThermoInput::atom` (one atom, set by ThermoGather) allows no frequencies
and gives no rotation; without it a single atom took the linear branch
with an infinite rotational constant.
The Thermochemistry panel has two wxGrids now, so `thermolayout` reports
both (the test counts 15 widths per call).
The Energy Calculator's T / K: `applyThermoTreatments(..., kelvin)` reruns
ecce-thermo at kelvin when it differs from `gTemperature`, overwrites the H
and G rows and their labels and sets `gTemperature`, so the 1 M correction,
log K and the S row (in `sumTable`, (dH - dG)/T) follow. "The code's own"
falls back to qrrho-grimme:100 off the stored temperature.
ECCE-QM's `symmetry_number` line is THERMOSIGMA through `ecceqm.desc`; the
stored ECCE-QM examples got the file added by hand from their `.qmout`, since
stored calculations are not re-parsed.
