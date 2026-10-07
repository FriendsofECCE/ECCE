# ecce-qm: the built-in HF/DFT engine (stage 1)

A small quantum chemistry engine for teaching-size molecules (tens of atoms
at most), meant to ship with ECCE so that a calculation needs no external
code. Stage 1 (this directory tree) is single-point energies and orbitals.
Planned: analytic gradients and geometry optimisation (stage 2), numerical
Hessian, frequencies and IR (stage 3), ECCE registration as a code (stage 4).

Source `src/qm/` (library `eccems`, no wx; command line `ecce-qm`), tests
`tests/qm/`.

## Build

    cmake -S src/qm -B build-qm -G Ninja
    cmake --build build-qm -j2
    ctest --test-dir build-qm             # qm_unit, qm_scf_quick, qm_scf

or inside the ECCE build with `-DECCE_BUILD_QM=ON` (not yet exercised there).

| dependency | how | licence |
|---|---|---|
| libcint 6.1.3 (integrals) | always fetched (not packaged in Debian) | Apache-2.0 |
| libxc 7.0.0 (functionals) | system `libxc-dev` (>= 5.1, Debian: `apt install libxc-dev`) if found, else fetched; `-DECCE_QM_VENDOR_LIBXC=ON` forces the fetch | MPL-2.0 |
| Eigen 3.4.0 (linear algebra) | system `libeigen3-dev` if found, else fetched | MPL-2.0 |
| OpenMP | optional | |

The Lebedev angular grids (`lebedev_data.inc`) are generated from
`scipy.integrate.lebedev_rule` (BSD-3, derived from the Lebedev-Laikov tables)
by `src/qm/tools/gen_lebedev.py`; scipy is not needed to build.
Everything is static, so the same CMake works on Linux, macOS and MSYS2
(not yet tried on the last two).

## Input

    title water
    charge 0
    multiplicity 1
    method b3lyp          # hf svwn pbe b3lyp pbe0; prefix r, u or ro (rhf, uhf, rohf, rob3lyp ...)
    basis def2-SVP        # any ECCE library name: 6-31G*, STO-3G, def2-TZVP, ...
    spherical true        # default; false = Cartesian d, f
    grid 99 590           # radial x Lebedev points per atom (DFT)
    geometry              # angstrom (units bohr to change)
    O 0 0 0.1173
    H 0 0.7572 -0.4692
    H 0 -0.7572 -0.4692
    end

`ecce-qm [--basis-dir DIR] [--molden FILE] [-o OUT] [-v] [--probe x y z] input`.
Basis sets are read from ECCE's own `data/admin/basissets/*.BAS`
(`ECCE_BASIS_DIR`, `$ECCE_HOME/data/admin/basissets` or `--basis-dir`).
ECCE stores polarisation sets as separate files, so `6-31G*` means
`6-31G.BAS` plus `6-31GS.BAS`, `6-31G**` adds `6-31GSS.BAS`; `a+b` combines files
explicitly. ECPs (the def2 heavy elements) are not supported.
Without a prefix the reference is restricted for multiplicity 1 and
restricted-open otherwise.

## What it computes

* RHF, UHF, ROHF; Kohn-Sham equivalents with SVWN (Slater + VWN5), PBE,
  B3LYP (libxc `HYB_GGA_XC_B3LYP5`, i.e. VWN5, as ORCA and GAMESS; Gaussian's
  B3LYP uses VWN3 and gives different energies; not provided), PBE0.
* Initial guess: spin-averaged atomic HF densities; Pulay DIIS (8 vectors);
  convergence |dE| < 1e-10 and max orbital gradient < 1e-7.
* Two-electron integrals in core (8-fold packed), so the size limit is memory:
  benzene def2-SVP (114 functions) needs 170 MB, about 250 functions 4 GB.
* DFT grid: Treutler-Ahlrichs M4 radial points x Lebedev, Becke partition
  with Bragg-radius size adjustment, no pruning. Default 99 x 590 per atom.
* Output: total and component energies, orbital energies and occupations,
  MO coefficients, Mulliken net charges, dipole, <S^2>; a Molden file.

### Restricted open shell (ROHF, ROKS)

One set of orbitals, doubly occupied core, singly occupied open shell. The
effective Fock matrix has core-core = (Fa+Fb)/2, open-open = virtual-virtual =
Fa, core-open = Fb, open-virtual = Fa, core-virtual = (Fa+Fb)/2. The energy
does not depend on the diagonal blocks, the orbital energies do. Against ORCA
6.1.1 (`ROHF`, `ROKS`) the energies agree to 1.5e-9 (HF) and 1.5e-6 (B3LYP),
the occupied orbital energies to 3e-5 and the lowest virtual to 1.6e-4; higher
virtuals differ by up to 1e-2 because ORCA's canonicalisation of the virtual
block is not documented and not Fa. The textbook alternative
(all diagonal blocks (Fa+Fb)/2) puts the singly occupied orbitals about
0.3 Eh higher than ORCA does for O2.

### Orbital order and the AO list

MOs are numbered from the core up, in the order of the eigenvalues of the
effective Fock matrix. AOs: atoms in input order; within an atom shells
sorted by angular momentum (stable), the order ECCE's basis storage uses;
spherical shells s; px,py,pz; then real harmonics m=-l..l (d: xy yz z2 xz
x2-y2), Cartesian shells in libcint order. `ao_labels` in the output gives
the ORCA-style label of every coefficient row.

## Output format (version 1)

Line oriented text, key value pairs, then blocks
(`begin NAME` ... `end NAME`). Keys: `format_version program title method
reference basis spherical grid charge multiplicity nbf nalpha nbeta converged
iterations energy_total energy_nuclear energy_one_electron energy_coulomb
energy_exchange_hf energy_xc grid_electrons s_squared dipole_au dipole_debye
wall_seconds`. Blocks: `atoms` (index symbol Z x y z[angstrom] mulliken),
`ao_labels` (index label), `orbitals SPIN` (index energy occupation; SPIN is
`restricted` with total occupation 0/1/2, or `alpha` and `beta`),
`mo_coefficients SPIN` (one line per MO: index then nbf coefficients).
The file ends with `end_of_output`. ECCE reads MO coefficients from a code's
output text (`scripts/parsers/orca.mo` for ORCA); stage 4 adds a parser for this
format. The Molden file (`--molden`) uses the 5D 7F 9G convention and was
checked against Multiwfn 2026.9.20 (density and orbital values at a point,
including signs).

## Verification

`tests/qm/oracle.json` holds, for 67 cases, the energies and orbital energies
printed by ORCA 6.1.1 (program, version and keywords recorded per case), and
NWChem 7.2.3 energies for 22 of them (spherical basis, xfine grid, B3LYP written
out with VWN5). `tests/qm/gen_oracle.py` regenerates them.
`tests/qm/run_tests.py` runs `ecce-qm` and compares; tolerances are in its
header. `qm_unit` checks the Lebedev rules against analytic integrals and the
AO values (used for DFT) against libcint's overlap matrix by quadrature.

PBE differs between programs: for water def2-SVP libxc gives -76.272001, NWChem
agrees to 2e-8, Gaussian 16 C.01 (ultrafine grid) gives -76.271983, ORCA -76.272015. The difference
to ORCA grows with the number of electrons (1.4e-5 for water, 5.5e-5 for
benzene); the tests therefore hold ORCA's PBE and PBE0 to 1e-4 and NWChem's to
1e-6.
