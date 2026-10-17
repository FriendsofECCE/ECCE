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
  fragment set with share >= 0.10, all lines one width and dashed like the
  other constructions (a thicker line looked like a rendering artifact);
  the share is in the tooltip (`MoLevel::detail`).
- Placement: a set sits at e_k = <FO~_k|F|FO~_k>, the diagonal of the parent
  Fock operator in the Loewdin-orthogonalised FO basis, = sum_m eps_m
  |<FO~_k|psi_m>|^2 over the VALENCE MOs (capture >= 50%). Core MOs are left
  out of the sum: an EHT STO is not orthogonal to the parent's 1s-3s and a
  0.5% overlap with a -7 Ha MO moved Co 4s by 0.04 Ha. It is the F that
  MoLigandField::buildFock() writes out, without forming it. No fit:
  the old linear scale put Cr(CO)6's t2g above both d and pi*. The set's
  capture (norm of its projection on the MOs) is in its caption when below
  90%; EHT H_ii / FO energy stays in the hover as qualitative.
- Symmetry: levels are labelled in the point group of the coordination
  skeleton (metal + donor atoms, `MoFragments::coordinationSkeleton`, else
  every atom), found in `FoDiagram.C` from the atoms alone (an operation is
  fixed by where it sends two bond vectors; the group is the table whose
  classes match). A FO group is projected on each irrep with the function
  rotation matrices, so the metal 3d splits into an eg and a t2g set (they
  have different Fock diagonals). An MO takes the irrep with most share.
  Unlabelled levels (ORCA without UseSym) are regrouped by that irrep's
  dimension, so Co(NH3)6's split t2g is one level.
- Pairs: two levels of one skeleton irrep and degeneracy that mix the same
  metal set, the lower below that set's Fock level and the higher above it,
  both with >= 5% of one ligand set of that irrep (and >= 10% of each parent
  in one of them), best score first. The partners share a colour
  (`MoLevel::pairing`), both are linked to both parents even when one's own
  share is under 10%, the antibonding partner of a drawn bonding level is
  always drawn (past the levels cut-off too: the panel passes the whole
  spectrum), labels follow the other constructions (`*`, ` nb`).
- Pair count, every drawn diagram: per irrep, drawn bonding = drawn
  antibonding (`FoDiagram::countPairs`); a mismatch is one line in the
  readout and says whether a partner is past the cut-off or hidden by the
  default filter. Warn, never block.
- Not drawn by default, counted in the note: ligand-class MOs with < 10%
  metal ("Show all levels" draws them), "core" MOs (capture < 50%), and
  fragment sets more than 25% of the MO range outside it (pair parents are
  exempt). With Show all levels on the columns are crowded; the canvas
  squeezes a column that has more levels than room.
- Pitfall: the semicore rule (#175: occupied, >= 90% on one atom, below
  twice the atom's VOIE) folds away the Co(III) t2g of Co(NH3)6 3+, the
  very levels the diagram is about. With FO wanted the panel keeps them
  (core is told by capture) and adds their orbitals to the
  `hideBeyondValence` room, otherwise TiF6's eg* is cut.
- Metal electrons: n = group number - oxidation state (ionic; the metal's
  charge is what `autoFragments` leaves after the ligands'), with
  multiplicity - 1 unpaired; ns/np empty; outside 0-10 or a multiplicity the
  count cannot have draws none and the note says why. The d sets are filled
  from the OCCUPIED MOs, not by Fock-diagonal aufbau: per spin, a set's
  population is sum over occupied MOs of occupation x the MO's share of the
  set, and the sets are filled whole in the order of population per orbital
  until the spin's total (alpha = (n+u)/2, beta = (n-u)/2) is placed. Rounding
  each set alone would give Cr(CO)6's eg its sigma-donation electrons. Gives
  [PtCl4]2- b1g empty, [CoCl4]2- e4 t2^3, ferrocene e2'/a1' filled and e1''
  empty (the Fock-diagonal order did the reverse). The note's "Electrons:" line compares metal d
  + ligand-FO electrons with the electrons in the valence MOs (capture >=
  50%) and says when they differ. The panel does not know the multiplicity
  and passes 1.
- `tools/modiagram/fragorb CALC --diagram [--all]` prints the same model (sets,
  levels, pairs, lines); the headless PNG route is `plotshot`
  (`ECCE_PLOTSHOT_POS=x,y` moves the floating panel off the Builder's own
  windows, `ECCE_MODIAGRAM_SHOWALL=1` ticks Show all levels).
