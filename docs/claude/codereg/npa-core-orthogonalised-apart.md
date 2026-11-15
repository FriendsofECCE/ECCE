---
type: pitfall
title: "NPA: the core set is orthogonalised apart from the valence minimal basis, or charges miss NBO by ~0.002 e"
area: codereg
paths: ["src/tdat/resultsh5/NaturalPopulation.C", "include/tdat/NaturalPopulation.H", "src/tdat/resultsh5/ecce_npa.C", "tests/npa"]
issues: [1054]
---
`ecce-npa` (library `npa::compute` in eccetdat, wx-free) follows Reed,
Weinstock, Weinhold 1985. Done literally -- one occupancy-weighted
symmetric orthogonalisation (OWSO) of the whole natural minimal basis --
water's O comes out -0.8864 against NBO 7's -0.8886, and the O core
population stays 1.99983 for every choice of weights while NBO's is 1.99996.
That invariant core was the clue: NBO keeps the core atomic. Orthogonalise
the core pre-NAOs on their own (OWSO), Schmidt the valence minimal basis
against them, then OWSO the valence set; Rydbergs are Schmidted against the
whole minimal basis, re-diagonalised per atom (their occupancies are the
weights) and OWSO'd. With that, water matches NBO to 1e-5 in every column.

Checked and rejected (do not retry): per-m own occupancies as weights
(-0.871), iterating weights to self-consistency (diverges: O -1.6), one OWSO
over minimal + Rydberg without Schmidt (-0.840, Rydberg populations wrong).
Pre-NAOs were checked directly: NBO's `DMPNAO` diagonal equals ours to 1e-4.
To get NBO's internals for a run: ORCA leaves `FILE.47` in the run directory;
edit its `$NBO $END` line (e.g. `$NBO DMPNAO $END`) and run
`gennbo.i8.exe FILE.47` with NBOEXE set (the i4 build refuses ORCA's file).

The job store's results.h5 has no `ecp` group (ResultsFromCalc does not
write one), so `compute` infers def2 (Stuttgart) core sizes when the
all-electron count is off by more than 8 and says so in `Result::note`.
The job store also writes no results.h5 at all for a calculation whose basis
was only a route-card name (`useRouteCard def2-SVP` with no stored basis):
"the stored basis set does not match". `tests/npa/make_fixtures.py` stores
the basis through `launchjob setup` (an MVM .frag + library-name .gbs) first.
