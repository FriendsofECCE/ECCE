---
type: pitfall
title: "ECCE-QM ROHF/ROKS on an unevenly filled degenerate shell (O 2p4, F 2p5) never converged: aufbau swaps the doubly and singly occupied members"
area: codereg
section: "Pitfalls found more than once (check for siblings)"
paths: ["src/qm/scf.cpp", "tests/qm/roks_atoms_test.py"]
issues: [790]
---
In a restricted open-shell SCF the doubly occupied set is the lowest `nbeta`
eigenvectors of the effective Fock matrix and the singly occupied set the
next `nalpha - nbeta`. When a degenerate shell is filled unevenly (O and S
p4, F and Cl p5), the members that should be doubly and singly
occupied have eigenvalues within a few hundredths of a hartree of each other,
and their order changes from cycle to cycle. An aufbau pick then moves one
electron pair to a different p (or d) orbital each time it flips, and DIIS
mixes Fock matrices of the different configurations. The energy oscillates by
1e-4 to 1e-3 Eh and the run stops at 150 cycles. Half-filled or less than
half-filled open shells (C p2, N p3, Cr d5 s1) are unaffected: their open
and core orbitals belong to different shells.

How to see it: with `-v`, compare the doubly occupied space of successive
cycles, `nb - tr(Db_old S Db_new S)`. It jumps to 1.0 (a whole orbital moved)
on most cycles.

`run_scf` therefore uses aufbau only until two successive cycles fill the same
doubly and singly occupied spaces (or from cycle 8 at the latest), resets the
DIIS history, and from then on picks the occupied orbitals by maximum overlap
with the previous cycle's doubly and singly occupied densities (MOM,
`mom_reorder`). The columns of C are reordered so that `[0, nb)` is doubly and
`[nb, na)` singly occupied. Because of this the reported orbital energies of a
ROHF run need not be in ascending order, and code that reads the occupied set
must use the column index or `occ_a`/`occ_b`, never a sort by energy.

Tried and rejected: a level shift of the open and virtual blocks (it leaves
the stationary point alone, but it slows the near-free rotation of the
non-spherical atom against the grid and ROKS stalls just above
`conv_grad`), and capping the DIIS coefficients (fewer converged runs).

A ROKS atom with a non-spherical density still takes longer than ROHF: on the
default pruned grid the energy depends very slightly on how the atom is
oriented, so the last decade of the gradient converges slowly, and the cycle
count varies from run to run because the OpenMP XC sums are not reproducible
to the last bit. UKS has the same slow tail.

Not covered by this fix: Sc-Ni. Ti (3d2 4s2) at B3LYP/def2-TZVPD keeps its
occupied space under MOM but stalls at |grad| ~1e-6 on the pruned grid, and
Ti UKS stalls the same way; with `grid_prune off` both converge (ROKS
-849.281610, UKS -849.282371, 29 and 25 cycles). At def2-SVP the 4s/3d
near-degeneracy (3d(n)4s2 against 3d(n+1)4s1) keeps aufbau from settling
in Ti, V and Ni ROHF/ROKS, so MOM is forced on at cycle 8 into whatever
configuration is current and the run wanders; UHF/UKS of Sc, Fe and Co fail
too. `tests/qm/roks_atoms_test.py` therefore covers H-Ar, Ca, Cr and Cu.
