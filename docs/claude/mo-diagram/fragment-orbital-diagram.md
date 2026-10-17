---
type: map
title: "The fragment-orbital diagram in the MO Diagram panel (#301, route B stage 2)"
area: mo-diagram
section: "The MO correlation diagram (#132)"
paths: ["include/tdat/FoDiagram.H", "src/tdat/chemistry/FoDiagram.C", "src/apps/builder/MoDiagramPanel.C", "src/apps/builder/MoDiagramCanvas.H", "tools/modiagram/fragorb.C", "tests/fragorb/run_tests.py"]
issues: [301]
---
`MoFragments::FRAGMENT_ORBITALS` ("Fragment orbitals (extended Hückel)") is
the default construction for a metal complex whose parent has non-orthogonal
coefficients and a basis (not MOPAC; the panel says why); the other
constructions stay in the Built as menu (`ECCE_MODIAGRAM_CONSTRUCTION=
skeleton|central|halves|groups|sets|fragorb` picks one in tests).
`FoDiagram` (tdat, no wx) builds it; the panel only gathers inputs.

- Columns: metal valence AOs (left), ligand-shell FOs (right), the parent
  MOs in the middle at their own energies. A line goes from an MO to every
  FO group with share >= 0.10 (`MoConnection::weight`, line width follows
  it); the tooltip appends class, shares, capture, largest FOs
  (`MoLevel::detail`).
- Placement: ONE linear map for all FOs, least squares of each FO's
  share-weighted mean MO energy (over the DRAWN MOs) on its EHT energy,
  weights = the FO's total share. Fit r is in the note (0.66 Cr(CO)6,
  0.86 Co(NH3)6, 0.89 TiF6). A single map cannot keep a bonding MO below
  both its parents: Cr(CO)6's t2g sits ABOVE the fitted d and pi* levels.
- Not drawn, and counted in the note: ligand-class MOs with no >= 10% line
  to a metal orbital (ligand internals), "core" MOs (capture < 50%), and
  fragment levels the map puts more than 25% of the drawn MO range outside
  it (a high ligand sigma* would stretch the axis).
- Pitfall: the semicore rule (#175: occupied, >= 90% on one atom, below
  twice the atom's VOIE) folds away the Co(III) t2g of Co(NH3)6 3+, the
  very levels the diagram is about. With FO wanted the panel keeps them
  (core is told by capture) and adds their orbitals to the
  `hideBeyondValence` room, otherwise TiF6's eg* is cut.
- FOs carry no symmetry labels; the right column names them FO n-m with the
  dominant AO character, so the textbook a1g/eg/t1u names are not on them.
- `tools/modiagram/fragorb CALC --diagram` prints the same model; the
  headless PNG route is `plotshot` (`ECCE_PLOTSHOT_POS=x,y` moves the floating
  panel off the Builder's own windows).
