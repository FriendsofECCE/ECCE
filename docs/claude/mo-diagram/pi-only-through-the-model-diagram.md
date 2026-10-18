---
type: rule
title: "\"pi system only\" goes through the model diagram, as a subset of it"
area: mo-diagram
paths: ["src/tdat/chemistry/ModelDiagram.C", "include/tdat/ModelDiagram.H", "src/apps/builder/MoDiagramPanel.C"]
issues: [301]
---
The panel used to skip `ModelDiagram` when "pi system only" was ticked and draw
the old TASO/H-atom layout: wrong counts (the 6 C column held 4 e), every
molecular level `nb`, labels on the connector lines. `Input::piOnly` now builds the
whole model and keeps what changes sign in the molecular plane:

- the plane's normal is the third principal axis of the atoms (a molecule more
  than 0.15 A off one plane is refused: the old filter then runs);
- a model orbital, a fragment set and a calculated level are pi when more than
  half of its coefficients are odd under the reflection (p along the normal;
  d through the quadratic forms; s is even), `oddFraction`;
- electrons: the model's aufbau over all levels, then the odd levels; a fragment
  takes the electrons the odd occupied model levels put on its sets (benzene: C6
  six, H6 none), filled by energy;
- bonding/antibonding: the overlap population over the BONDED atom pairs
  (`covalentBonds`), because the two fragments (C6, H6) share no pi orbital; the
  pair rule then needs neither equal irreps nor `PAIR_FLOOR` (a2u with b2g, e1g
  with e2u);
- tags: "C 2p pi" whatever the unit (`ligandCharacter(..., piOnly)`).

Benzene gives 1a2u, 1e1g bonding and 1e2u, 1b2g antibonding, 6 pi electrons in
the model and in the calculation column. Goldens `orca-benzene-pi`, `ecceqm-c2h4-pi`.
