---
type: map
title: "ORCA dispersion is ES.Theory.SCF.Dispersion; old functional names are split on read"
area: codereg
paths: ["scripts/codereg/orcatheory.py", "scripts/codereg/orcatheory.dlg", "scripts/codereg/templates.py", "src/wxgui/details/DetailsModel.cpp", "scripts/parsers/ai.orca", "tests/orca_dispersion/run_tests.py"]
issues: [593]
---
Choice None / D3BJ / D3(0) / D4 (D3(0) is written as ORCA's `D3ZERO`), offered
for SCF and DFT; ORCA 6.1.1 accepts `HF D3BJ` and `HF D4` and prints a
dispersion correction for them. A functional stored as `PBE0 D3BJ` or
`B3LYP-D3` (D3 alone means zero damping) is split in three places that must
agree: `ai.orca` `SplitDispersion` (deck), `templates.py`
`SplitLegacyDispersion` and `DetailsModel::initialize` (dialog restore).
`B97-D3` and `wB97X-D3` are functionals and are never split. Without the
dialog split the stored name matched no combo entry and re-saving silently
fell back to B3LYP with no dispersion. The Organizer summary shows only the
stored theory ("DFT/RDFT"), not the functional, so it has no dispersion line.
