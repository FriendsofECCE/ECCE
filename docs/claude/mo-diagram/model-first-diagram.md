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
route; the rest of a ligand goes to the right fragment). Also "pi system only"
(`Input::piOnly`, see `pi-only-through-the-model-diagram.md`). Not without a
basis/overlap (MOPAC); that, and elements without parameters (Ag,
refusal text plus "use the MOs panel"), keep the old drawing.

- Middle column = the molecule's own EHT run (`FragmentOrbitals::solveWhole`);
  side columns = the fragments' EHT orbitals; same basis, so the lines are the
  exact (Loewdin) projection, share >= 10 %.
- Bonding/antibonding: sign of the inter-fragment overlap population (|OP| > 0.02),
  paired strongest-with-strongest per degeneracy; same irrep unless the two
  fragments are equal (halves combine g/u). Unpaired = nb. This is the model's
  reading, not the book's: N2 2su comes out nb (sp mixing), pair 2sg/4su*.
- Calculation column (layout C): its own energy axis in Hartree, to scale, ticks
  on the right, broken like the model's for the high empty levels; no lines to
  the model. Each model level is matched to the calculated level (the panel's
  `wholeSpectrum`) with the largest subspace overlap, via the mixed STO/Gaussian S;
  distinct calculated levels are drawn once, at their own energy, in the colour of
  the model partner, labelled like the model (lower-case Mulliken, `*` from the
  partner) with the energy below; overlap < 0.80 adds a grey "≈" (value in the hover
  text only). Levels whose boxes would overlap vertically go side by side in lanes
  (`MoDiagramCanvas::layoutCalc`). `MoLevel::orbitals` are the calculated MOs, so a click
  shows them. Verdict: only the occupied levels, plus the LUMO when the model's
  LUMO matches the calculation's; a different LUMO is named; order above the LUMO
  is only in About. Delta_o for t2g/eg.
- Hover: a model level shows the model's fragment composition and "Matches your MO a-b";
  a calculation-column level shows its own MOs-panel composition (`MoPanel::compositionText`)
  and "matches model <label>, overlap x". The calculation's HOMO and LUMO are always drawn:
  without a model partner they sit in `MoColumn::calcOnly`, in grey ("no model partner").
  Fragment-column symmetry labels are lower-case (a1g, t2g); AO names stay (3d). Test hooks:
  builder scene `modiagram <n> tip|ctip <i>`.
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
- Character tags (`MoLevel::tag`): every model level carries the fragment-orbital
  characters with >= 20 % of it ("Cr d + CO π*"): metal = symbol + shell letter;
  ligand = one-atom unit "F 2p", diatomic unit "CO σ"/"CO π" (p along/across the
  bond; `*` when the intra-ligand overlap population is < -0.10), larger unit
  "NH3 σ". Drawn as a third grey line under the energy (`drawLabel` measures the
  block, right-aligned left of the centre column) and first in the hover text.
- Ligand-field view (`Input::ligandFieldView`, check box, `ECCE_MODIAGRAM_LFVIEW=1`;
  transition-metal centre + >= 2 ligand atoms only): empty levels above the
  metal-based HOMO with >= `LF_LIGAND_SHARE` (50 %) on the ligands are folded into a
  band "ligand π* levels: N levels, empty (Ligand-field view)". The threshold is
  50 %, not 80 %: the model's 3t2g* of Cr(CO)6 is 61 % ligand (the DFT's 78 % CO π*
  is a different number). The calculation's unmatched LUMO is not drawn and the
  LUMO is not judged in this view. check_pairs.py skips the pair rule for `*-lfview`
  goldens (the antibonding partners are hidden on purpose).
- Phase sketches: a fragment column of several atoms of one element carries the
  sign and size of each atom's s coefficient, or its p vector, of the set's first
  orbital (`sketchPhases`, drawn by `MoDiagramCanvas::drawSketch`), in the
  atoms' own principal-axis frame (`principalAxes`), so a structure stored in any
  orientation is drawn flat. They came from the symmetry projection
  (`MoFragments`) before the model built its own sets, and so vanished from every
  model-first diagram. Not for mixed fragments, d sets or one atom.
- Ligand ns is core-like in a central-atom diagram (one atom against >= 2): the ns
  of a ligand atom that also has np (F 2s, Cl 3s; not H 1s, not the CO sigma
  unit) is `SetD::nsShare`; a model level with >= 50 % of it is `Cluster::core`,
  never a bonding or antibonding partner, folded with the internal levels (so
  BF3's sigma bond is F 2p-sigma with B, as in the textbooks, and the pair drawn
  against 5a1'* is a1' of 2p). The fragment column does not draw such sets either.
- What is folded as internal is folded on both sides: a calculated level whose
  model partner is folded is not drawn as "your HOMO/LUMO" either, and the note
  and About say so once.
- Fragment electrons: neutral ligands, by energy; when that leaves a degenerate
  set half full (three F atoms are 21 electrons) the sets are chosen as close as
  possible to the populations the occupied model orbitals put on them
  (`fillByPopulation`: whole sets, one electron on one non-degenerate set at most),
  so total = B 3 + F3 21 = 24 and no e' set is half filled.
- Middle column in lanes (`MoDiagramCanvas::layoutCentreLanes`): the lines of levels
  whose arrows would meet go in side-by-side lanes, every line at its own energy
  (the column is not opened out any more). Labels are one row each ("label energy
  tag") in a column left of the lanes, moved apart only as far as they must and
  joined to their lines by a thin leader, on a white patch so connectors run behind
  the text. At most as many lanes as fit in 55 % of the width; a denser spectrum
  (a whole big complex) shares the last lane. Test hook: scene
  `modiagram <name> overlaps 0` prints the number of overlapping pairs
  (`orca-ptcl4-overlaps` in tests/modiagram/capture.py).- Electron bookkeeping (summary part "electrons: ..."): per fragment drawn + folded
  as core-like ns + folded as internal = its total (the About's), and the model
  column's drawn + folded = the molecule's total; the calculation column folds the
  same levels. `check_pairs.py::check_electrons` asserts it on every golden. The
  band's level count is MOs, not fragment orbitals (BF3: 6 + 6 occupied MOs; the 6
  folded are 3 core-like F 2s + 3 lone pairs). A neutral odd fragment (F3, 21 e)
  keeps one unpaired electron, on the highest-lying non-core non-degenerate set,
  and the note says "(neutral: 1 unpaired e)".
