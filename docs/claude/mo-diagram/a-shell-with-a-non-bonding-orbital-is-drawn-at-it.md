---
type: map
title: "An atom's shell with a non-bonding orbital is drawn at that orbital (#140)"
area: mo-diagram
section: "The MO correlation diagram (#132)"
paths: ["src/tdat/chemistry/MoDiagram.C", "include/tdat/MoDiagram.H"]
issues: [140, 132]
---
`placeFragments()` draws every component of an atom's shell on one row.
The row is the mean of the orbitals the components became, except when
the shell has a non-bonding orbital: then the row is at that orbital's
energy, so it sits level with its parent (water's 1b1 on O 2p).

Non-bonding means `nonbondingShellFraction() >= NONBONDING_SHELL_SHARE`
(0.99): the share on that one shell over the share on everything the
diagram draws. Polarisation functions of column atoms (O d, H p) are
left out of the denominator, because without them symmetry-forbidden
mixing gives exactly 1 in any basis; atoms in neither column count
against. Because of that, every b1 orbital of water is "pure O p",
including diffuse virtuals, so only the lowest qualifying orbital per
irrep pins the row.

A lone pair from s/p mixing (water 3a1, NH3 3a1) is not single-shell
(f = 0.76 and 0.86 in ORCA def2-SVP; 0.93 and 0.94 in extended Huckel)
and is drawn in the usual way, connected to s, p and the ligand set, as
in Albright-Burdett-Whangbo (2013) Figs. 7.2 and 9.4. classify() tags it
MIXED, not nb, when its irrep is on both sides and it holds at least
`LINK_SHARE` (0.05) of each column's drawn shells; with a composition,
connect() draws every side at or above `LINK_SHARE`, nb or not. Before
this, water 3a1 was tagged nb by the count and drawn to O only.

The panel prints the fraction per level and shell as `[MOSHELL]` under
`ECCE_DEBUG_MOSYM`. The canvas opens the molecular column out too (it
calls `columnGeometry()` with spreading on for it), so a pinned row and
its orbital can still be drawn a few pixels apart when other molecular
levels crowd it: 4 px in extended Huckel water at 1400x950.
