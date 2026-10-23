---
type: pitfall
title: "GROMACS Ensemble row (NVE/NVT/NPT) is only a view of UseNVT and UseNPT; NPT sets both, and the barostat still lives on Run settings (#478)"
area: codereg
paths: [src/wxgui/mdtools/DynamicsPanel.C, include/wxgui/DynamicsPanel.H, src/apps/mddynamics/MDDynamics.C, tests/gromacs_md/ensemble_test.py, scripts/parsers/ai.gromacs]
issues: [478]
---
`DynamicsPanel::buildEnsembleRow` (GROMACS profile only) puts three radio
buttons at the top of the page; `setEnsemble` writes `UseNVT` (NVT, NPT) and
`UseNPT` (NPT) into the model and mirrors them into the two sub-tab check
boxes, which are hidden for GROMACS. The model stays the one source; the
radios are re-read in `showEnsemble` whenever the panel loads. `ai.gromacs`
writes `tcoupl` only with UseNVT and `pcoupl` only with UseNPT, so NVE has
neither and NPT without a thermostat is made impossible in the UI.
Test hook of the Dynamics editor: `ensemble`, `ensemble-set nve|nvt|npt`.
Barostat and coupling type are still `GromacsRunPanel` fields.
