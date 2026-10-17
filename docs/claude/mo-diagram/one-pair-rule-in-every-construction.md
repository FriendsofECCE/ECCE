---
type: rule
title: "One pair rule for every MO diagram construction (#301)"
area: mo-diagram
paths: [include/tdat/MoPairs.H, src/tdat/chemistry/MoPairs.C, src/tdat/chemistry/MoDiagram.C, src/tdat/chemistry/FoDiagram.C, tests/modiagram/check_pairs.py]
issues: [301]
---
Per symmetry label, n fragment orbitals give n MOs. A pair is a bonding
level below both parents and its antibonding partner (`*`) above both; any
other drawn level is `nb`. There is no MIXED / "weakly bonding" class.

- `MoPairs` (tdat, no wx) holds the shared code: `pairByParents` (the
  fragment-orbital diagram's pairing, moved out of `FoDiagram::build`),
  `countPairs` (the tier-B readout; `FoDiagram::countPairs` forwards) and
  `settle`, which `classifyByEnergy` runs last for GROUPS, EQUIVALENT_SETS,
  CENTRAL, HALVES, SKELETON and CHOSEN: MIXED becomes nb, a bonding or
  antibonding level whose partner (same pairing number, same degeneracy,
  bonding one lower) is not among the levels becomes nb, and the `*`/`nb`
  suffix is rewritten from the final character. Its messages go to tier B.
- Consequence: a pi manifold made of ONE fragment set (benzene's six C pz
  TASOs: a2u, e1g, e2u, b2g) has no partner pairs by irrep, so every level
  is nb. The overlap-population sign no longer shows as `*`.
- The "below both parents" test is only applied by `pairByParents` (the
  fragment-orbital route, where the parents' shares are known); the others
  test bonding < antibonding of the same pairing number.
- `tests/modiagram/check_pairs.py` checks every golden (no MIXED, every
  bonding level has its antibonding partner) and BF3's pairs.
