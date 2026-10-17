---
type: map
title: "The model-first MO diagram: extended Hueckel for every column, the calculation as a column of its own (#301)"
area: mo-diagram
paths: ["include/tdat/ModelDiagram.H", "src/tdat/chemistry/ModelDiagram.C", "src/apps/builder/MoDiagramPanel.C", "src/apps/builder/MoDiagramCanvas.H", "tests/modiagram/check_pairs.py"]
issues: [301]
---
`ModelDiagram::build` (tdat, no wx) draws the diagram for every construction
whose two fragments cover the molecule and have EHT parameters (CENTRAL,
HALVES, GROUPS, EQUIVALENT_SETS, CHOSEN, SKELETON and the fragment-orbital
route; the rest of a ligand goes to the right fragment). Not pi-only, and not
without a basis/overlap (MOPAC); those, and elements without parameters (Ag,
refusal text plus "use the MOs panel"), keep the old drawing.

- Middle column = the molecule's own EHT run (`FragmentOrbitals::solveWhole`);
  side columns = the fragments' EHT orbitals; same basis, so the lines are the
  exact (Loewdin) projection, share >= 10 %.
- Bonding/antibonding: sign of the inter-fragment overlap population (|OP| > 0.02),
  paired strongest-with-strongest per degeneracy; same irrep unless the two
  fragments are equal (halves combine g/u). Unpaired = nb. This is the model's
  reading, not the book's: N2 2su comes out nb (sp mixing), pair 2sg/4su*.
- Calculation column: each model level is matched to the calculated level
  (the panel's `wholeSpectrum`) with the largest subspace overlap, via the mixed
  STO/Gaussian S. Labels and irreps are the matched level's; `MoLevel::orbitals`
  are the calculated MOs, so a click shows them. Energy (Hartree) and overlap
  are text. Verdict: inversions of the calculated order among drawn levels,
  overlap range, Delta_o for t2g/eg.
- Energies: model in eV on the axis. Empty levels > 30 eV above the HOMO are
  drawn at the top of the axis, marked with an up arrow, true energy beside
  them (EHT puts sigma* tens of eV up; the partner of a drawn level is never
  hidden). Ligand-internal levels (>= 80 % on the right fragment, a lone
  central atom against >= 2 others) are grey bars with level/electron counts.
- Electrons: model aufbau (alpha/beta for open shells); fragments' own, metal
  ionic (group - charge) in the one d set.
- Test hook: `fragorb CALC --model [--left 1,2]`; dump has `verdict:` and
  `calcEnergy/calcOverlap`; PNGs with `ECCE_XVFB_SCREEN=1900x1100x24`
  (the panel is wider than the default screen).
