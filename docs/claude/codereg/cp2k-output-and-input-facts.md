---
type: pitfall
title: "CP2K 2025.1: a trajectory to the output is FILENAME __STD_OUT__ (not =__STD_OUT__), an unconverged SCF aborts the run, OT line-search rows have no energy change, and three more"
area: codereg
section: "Pitfalls found more than once (check for siblings)"
paths: ["scripts/parsers/ai.cp2k", "scripts/parsers/cp2k.desc", "scripts/parsers/cp2k.geomtrace", "scripts/parsers/verifyinput"]
issues: [329]
---
Found by running CP2K 2025.1 (Debian `cp2k` 2025.1-1.1), not from the manual:

- **`FILENAME __STD_OUT__` writes the frames into the output; `FILENAME =__STD_OUT__` makes a file
  named `__STD_OUT__`** (a leading `=` means "this exact name, no project prefix"). The
  frames then arrive as an atom count, ` i = N, E = <energy>`, the atom rows and a blank line,
  so one `.desc` entry (`[GEOMTRACE][TEVEC]`, End = blank line) reads geometry and energy
  together and the two vectors have the same length. The starting geometry is not a frame:
  the first is printed after the first move (the same offset as in `-pos-1.xyz`). The
  energy on a frame line is that of the frame's own geometry.
- **An SCF that does not converge in `MAX_SCF` stops the whole run** (`[ABORT] SCF run NOT
  converged`, then `MPI_ABORT`), unless `IGNORE_CONVERGENCE_FAILURE` is set. FM NiO and MnO with
  U, UKS and an atomic guess did not converge with DIIS and `FULL_SINGLE_INVERSE` in 100 steps;
  CG with `FULL_ALL` converged in about 190 steps for the first geometry and 30 to 50 after
  that. Silicon with an SZV basis did not converge at all with OT/DIIS; DZVP did. Hence the OT
  minimizer and preconditioner are choices in Theory Details and `MAX_SCF` defaults to 100.
- **OT line-search rows** (`38 OT LS  0.57E+00  0.3   <blank>   -184.9...`) have no convergence
  and no change column; the SCF-step patterns require both and skip them. With OT off the
  method column is `Broy./Diag.` (one word), with OT it is `OT DIIS` (two).
- **`cp2k --check <file>`** parses an input and prints `SUCCESS` without running it, so every deck of
  the test is checked by CP2K's own parser.
- **The input checker counts "element and three numbers" lines as atoms**, and `A`, `B` and `C`
  of a `&CELL` are elements (boron, carbon): the cell vectors are written `A [angstrom] x y z`,
  which the pattern does not match.
- **Forces are printed as `FORCES| Atomic forces [hartree/bohr]` only when `&FORCE_EVAL/&PRINT/&FORCES`
  asks** (ENERGY_FORCE and GEO_OPT write it, ENERGY alone does not); the gradient is minus
  that, in the same unit. The Mulliken table of an unrestricted run has four columns after
  the kind (alpha, beta, net charge, spin moment), a restricted one two; Hirshfeld's net
  charge is the last column in both.
- **A failed job's reason** (`matchCp2kError` in `include/comm/JobFailureReason.H`) comes from the
  `[ABORT]` box every CP2K error ends in: the text beside the `\___/` drawing, wrapped over
  lines. A run that hit `MAX_SCF` gets advice and the step count from the "Leaving inner SCF
  loop after reaching N steps" line before the box. Fixtures: real tails in
  `tests/jobstore/fixtures/synthetic/cp2k-*.cp2kout`.
- **CalcEd's `gui KEY VALUE` script command sets a value but not the field's enabled flag**, and a
  disabled field is not exported to the `.param`. The DFT+U rows start disabled (no element
  typed), so a script cannot switch U on that way; the generator test covers U, and the editor
  test leaves it out.
