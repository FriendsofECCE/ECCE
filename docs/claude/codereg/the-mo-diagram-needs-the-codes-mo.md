---
type: checklist
title: "The MO diagram needs the code's MO coefficients and basis, with their AO order verified, not its population analysis"
area: codereg
section: "Added from integrating MOPAC (issue #86) — the second code through"
paths: [".edml", "Props/MO"]
issues: [86]
---
**The MO diagram needs the code's MO coefficients and basis, and
their AO order verified — not any population analysis from the
code.** ECCE computes Löwdin shares (S^½c) and overlap populations
itself, from `Props/MO` and an overlap matrix it rebuilds from the
stored basis, so a code never has to print them. What it must supply
is the coefficient table plus the basis, with `.edml` `MOOrdering`
correct for every l up to d/f. Check with the `[MOLOC]` line's
`norm=` (cᵀSc, `ECCE_DEBUG_MOSYM_LOG=<file>`): it must be 1.000 for
every orbital, degenerate sets and d-heavy ones included. A norm off
1 means ECCE is pairing coefficients with the wrong functions, and
every share, overlap population and line built on them is wrong
(Cr(CO)₆/ORCA, 2026-09-28: 0.7–2.9 on the Cr p/d sets). A
semiempirical code writes no basis; ECCE rebuilds a Slater one, but
NDDO coefficients belong to an orthogonal basis (S = I), so check
which S such a code's numbers actually assume.
