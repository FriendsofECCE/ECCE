---
type: pitfall
title: "A homonuclear diatomic is classified by phase (g/u), not by energy against the fragments"
area: mo-diagram
section: "The MO correlation diagram (#132)"
paths: ["src/tdat/chemistry/MoDiagram.C", "tests/modiagram/check_pairs.py"]
issues: [132]
---
s-p mixing puts C2's out-of-phase 2s pair (2sigma-u) below the 2p it
borrows from, so "below its parents is bonding" called it bonding and the
pairing (lowest with highest within a parity-less key) coupled it with
3sigma-u*. `classifyLinearByPhase()` decides from the label instead: the
g combination of a sigma or delta orbital is in phase (bonding), for pi
it is the u one; the k-th bonding level of each type is paired with the
k-th antibonding one (2sg/2su*, 3sg/3su*, 1pu/1pg*). Without linear-group
labels the old energy rule still runs. Heteronuclear columns never get
here. `tests/modiagram/check_pairs.py` asserts equal counts and the pairs
for C2, N2 and He2.
