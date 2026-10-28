---
type: map
title: "Custom solvent: ES.Theory.SCF.Dielectric + RefractiveIndex, written per code"
area: codereg
paths: ["scripts/codereg/orcatheory.py", "scripts/codereg/orcatheory.dlg", "scripts/codereg/ged16theory.py", "scripts/parsers/ai.orca", "scripts/parsers/ai.gauss16", "scripts/parsers/g16.tpl", "scripts/parsers/orca.solvmodel", "scripts/parsers/gaussian-16.solvmodel", "tests/custom_solvent/run_tests.py"]
issues: [642]
---
Solvent choice `Custom` plus floats `ES.Theory.SCF.Dielectric` (epsilon) and
`ES.Theory.SCF.RefractiveIndex` (n). ORCA 6.1.1: `! CPCM` and
`%cpcm epsilon <e> refrac <n> end` (output prints Epsilon, Refrac, Solvent CUSTOM).
CPCM only; SMD with Custom dies in ai.orca. Gaussian 16:
`SCRF=(<model>,Solvent=Generic,Read)` and, via the `##SCRFInput##` tag in
`g16.tpl` after the basis, `Eps=<e>` / `EpsInf=<n*n>` (output: Solvent Generic,
Eps, Eps(infinity)). The old G16 choice `Manual` wrote the invalid
`Dielectric=` and is now read as Custom by the generator. SOLVMODEL
("CPCM, Custom (e = 30.0, n = 1.42)") comes from the two `.solvmodel` parsers.
Only the G16 generator is changed; ai.gauss03/09 and their dialogs still say Manual.
