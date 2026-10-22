---
type: map
title: "Reaction Energies panel: a TS job's barrier from three calculations, listed by TE but only for QST/NEB-TS"
area: codereg
section: "How a property gets from disk into the Properties menu"
paths: ["src/apps/builder/ReactionPanel.C", "src/apps/builder/ReactionEnergies.C", "data/client/config/PropertyPanelDescriptor.xml", "tests/apps/rxn_energies_panel_test.py", "tests/rxnenergies/reaction_energies_test.C"]
issues: [419]
---
The panel is triggered by `TE` like Energies, so its entry sits before
Energies in `PropertyPanelDescriptor.xml` (Energies is a consumer and would
take `TE` first); `ReactionPanel::isRelevant` then keeps it to jobs whose
run type starts "Transition State (QST)" or "(NEB-TS". The reactant and
product are read from `ES.Runtype.GeomOpt.QST{Reactant,Product}` or
`ES.Runtype.NEB.{Reactant,Product}` in the job's SetupParams. A picked
calculation counts only if it is complete and stored a `GEOMTRACE` (the
parsers have no separate "converged" flag); otherwise the panel shows no
energies and names the calculation and the reason. `EZEROPT` is the
correction, `ENTHALPY`/`EGIBBS` are absolute (see the thermochemistry
entry). The arithmetic is `ReactionEnergies.C`, wx-free; `rxnenergies
<path> [kcal]` in the Builder script exports the CSV.
