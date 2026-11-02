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

`run_scf` therefore keeps aufbau, and switches to the maximum overlap method
(MOM, `mom_reorder`) only when, from cycle 20 on, aufbau has moved a whole
orbital (`moved > 0.5`) at least twice in the last five cycles. MOM then takes
the orbitals with the largest overlap with the previous cycle's doubly and
singly occupied densities, and the columns of C are reordered so that
`[0, nb)` is doubly and `[nb, na)` singly occupied. As a result the reported
ROHF orbital energies need not be in ascending order, and code that reads the
occupied set must use the column index or `occ_a`/`occ_b`, never a sort by
energy.

Do not start MOM as soon as two cycles fill the same spaces. A first version
did that and locked doublet NO (ROHF/def2-SVP, 1.5 A) at cycle 6 into a
configuration 50 mEh above the aufbau result. Swaps in the first ~12 cycles
are normal for NO, which then converges by aufbau. When a MOM solution has an
open or virtual orbital more than 1e-3 Eh below an occupied one, the SCF
relaxes with aufbau for up to 20 cycles and keeps the lower converged energy.
The atoms trip this check legitimately (their doubly occupied p lies 0.06 to
0.13 Eh above an open p), so their cycle counts include those 20 cycles.

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
too. `tests/qm/roks_atoms_test.py` therefore covers H-Ar, Ca, Cr and Cu,
plus NO, O2 and CH2 pinned to the aufbau energies. It runs single-threaded,
because two-thread XC sums made the O UKS reference fail now and then.

## Energy-plateau convergence (all SCF types)

A non-spherical density on the pruned grid leaves the orbital gradient on a
floor a little above `conv_grad` (O ROKS aug-cc-pVTZ: |grad| 2.1e-7, dE 1e-12
from cycle 27), and DIIS then drifts until max_iter. `run_scf` therefore also
accepts a run when five successive energies agree to `conv_energy` (four
differences) and |grad| < 10 * `conv_grad`. Both thresholds scale with the
input's `conv_energy`/`conv_grad`. Such a run writes `converged yes` as usual, adds
`convergence_note SCF converged (energy stable to ...; gradient ... above the
... target)` to the .qmout and `criterion=energy_plateau` to the progress
line. `ecceqm.parse` and `JobProgress.C` key on `converged`, so the Organizer
still shows the job as converged.

On such a floor the energy is only defined to ~1e-8: the near-free rotation
of the atom against the grid shifts it. With the rule, ROKS def2-SVP O moved by
-9.5e-9 and F by +3.3e-8 against strict convergence. Molecules and spherical
atoms were unchanged.

`qm_roks_atoms` includes O ROKS B3LYP/aug-cc-pVTZ (49 cycles, about 6 s on one
thread). Ti def2-TZVPD on the pruned grid: UKS converges by the plateau rule
(121 cycles, -849.282354857), while ROKS still stops at 150 cycles near
-849.281601 (#797).
