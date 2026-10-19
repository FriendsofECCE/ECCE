---
type: rule
title: "A single non-metal atom pick can be the centre of the fragment-orbital diagram (#301)"
area: mo-diagram
paths: [src/tdat/chemistry/FoDiagram.C, src/tdat/chemistry/FragmentOrbitals.C, src/apps/builder/MoDiagramPanel.C, tests/fragorb/run_tests.py]
issues: [301]
---
`FoDiagram::build(..., pickAtom)` and `FragmentOrbitals::autoFragments(...,
centre)` put a non-metal atom X where the metal is: left column X's valence
s and p, right column the ligand-shell FOs of everything else (the pieces
left when X is removed, named in the About text). Homolytic: every piece is
neutral (OH is a radical), X keeps the whole charge and its neutral valence
count. X's electrons go in its s and p sets by Fock-diagonal aufbau and the
atom's Hund count; the "Electrons:" line is X + pieces = valence MOs.

- Labels are those of the local skeleton, X plus its bonded atoms. The exact
  group is used unless the skeleton is within 0.15 A of a larger one (methyl
  of methanol: exactly Cs, C3v within its torsional distortion): then the
  atoms are idealised by averaging over the loosely found operations until
  they are exact (`findSkeleton`, `NEAR_TOL`). FOs within 0.01 Hartree are one
  level (`groupFos(..., tolerance)`) and each FO is drawn whole in the irrep
  it is mostly in.
- Fit of an MO to its label = weight of the label's irrep among all irreps its
  FOs reach (unrounded projectors) times its Mulliken weight on the skeleton's
  atoms. Shown plain from 0.9, `~` from 0.5, no label below; `*`/`nb` stay.
- Tier A says "labels in C3v (local, approximate)".
- Fixtures `ecceqm-methanol` (makecalc.py ecceqm ch3oh 0 --no-rotate) picked
  at C1 and O2; `tests/fragorb/fixtures/methanol` with `fragorb --atom N`.
- Pitfall: ligand-only levels are hidden by default (< 10% on X), so the
  methanol HOMO (an O lone pair) is not drawn when X is C; the summary says so.
