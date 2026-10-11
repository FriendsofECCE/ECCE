# ecce-qm: the built-in HF/DFT engine (stage 1)

A small quantum chemistry engine for teaching-size molecules (tens of atoms
at most), shipped with ECCE so that a calculation needs no external code.
Stage 1 is single-point energies and orbitals; stage 2 (below) adds analytic
gradients and geometry optimisation; stage 3 harmonic frequencies, IR
intensities and thermochemistry; stage 4 registers it in ECCE as the code
ECCE-QM.

Source `src/qm/` (library `eccems`, no wx; command line `ecce-qm`), tests
`tests/qm/`.

## Build

    cmake -S src/qm -B build-qm -G Ninja
    cmake --build build-qm -j2
    ctest --test-dir build-qm             # qm_unit, qm_scf_quick, qm_scf

or inside the ECCE build, where `ECCE_BUILD_QM` is ON by default and
`ecce-qm` is installed in the client package (`bin/ecce-qm`).

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
    runtype optimize      # energy (default), gradient, optimize, frequency or optfreq
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

## Gradients and geometry optimisation (stage 2)

`runtype gradient` adds the analytic nuclear gradient, `runtype optimize` moves
the nuclei to a minimum; `maxsteps N` (default 100 energy+gradient evaluations)
and `opt_conv normal|tight` (default normal) tune the latter.

* Gradient (`src/qm/gradient.cpp`, XC part in `xc.cpp`): libcint derivative
  integrals (`int1e_ipovlp`, `int1e_ipkin`, `int1e_iprinv` for the nuclear
  attraction of each nucleus with its operator-centre term, `int2e_ip1`), the
  energy-weighted density `1/2 sum_s (S^-1 F_s D_s + D_s F_s S^-1)` for the
  overlap term (also right for ROHF/ROKS), and the LDA/GGA terms of the XC
  energy on the grid. For RHF, UHF, ROHF and SVWN, PBE, B3LYP, PBE0 with
  restricted, unrestricted and restricted-open orbitals. The derivative of the
  grid weights is neglected.
* Optimiser (`optimizer.cpp`): Cartesian BFGS, Lindh model Hessian
  (stretches, bends, torsions) as the start, rational function steps in the
  space with translation and rotation removed, trust radius 0.3 bohr (at most
  1.0), a step that raises the energy is retried shorter. Converged when the
  maximum and RMS gradient, the maximum and RMS of the next step and the last
  energy change are all below the thresholds: `normal` is ORCA's Opt
  (3e-4, 1e-4 hartree/bohr; 4e-3, 2e-3 bohr; 5e-6 hartree), `tight` is
  TightOpt (1e-4, 3e-5; 1e-3, 6e-4; 1e-6). No redundant internal coordinates,
  so a floppy molecule needs more steps than ORCA's.
* Output: one block per accepted geometry, written as soon as it is known:
  `opt_step N`, `energy_step E`, `opt_criteria ...`, `begin geometry`
  (index symbol x y z, angstrom), `begin gradient` (hartree/bohr). Then the
  usual output for the final geometry, plus `runtype`, `opt_converged` and
  `opt_evaluations`. A gradient run has the one block. ECCE turns the blocks
  into GEOMTRACE, TEVEC (the energy trace; TE is the final energy), EGRAD and
  EGRADVEC (`ecceqm.desc`).
* Tests: `tests/qm/gradient_test.py` (analytic gradient against central
  differences of the energy), `tests/qm/opt_test.py` (final energy, distances
  and angles against ORCA TightOpt, `opt_oracle.json`, made by
  `gen_opt_oracle.py`).

## Frequencies, IR intensities, thermochemistry (stage 3)

`runtype frequency` computes the harmonic vibrations at the given geometry,
`runtype optfreq` optimises first (and does nothing more if the optimisation
did not converge). Further keywords: `hessian_step` (bohr, default 0.005),
`temperature` (K, 298.15), `pressure` (atm, 1), `symmetry_number` (default: worked out).

* Hessian (`vibration.cpp`): central differences of the analytic gradient over
  every Cartesian coordinate, step 0.005 bohr, so 6N gradient evaluations
  (`2 x 3N`). The same displaced calculations give the dipole derivatives.
  Every displaced SCF starts from the orbitals of the reference geometry and
  is converged to 1e-8 (orbital gradient) and 1e-11 hartree, tighter than an
  energy needs, so that the noise in the Hessian stays below 0.1 cm-1. The
  step is a compromise: smaller ones amplify the SCF noise, larger ones the
  cubic terms; 0.005 reproduces ORCA's analytic frequencies to 0.2-0.8 cm-1.
  With DFT the Hessian is the derivative of the gradient that neglects the
  grid-weight terms, so translations and rotations carry a residual of a few
  cm-1 (printed, see below).
* Analysis: masses of the most abundant isotopes (hydrogen 1.00783, not
  deuterium or the average; elements up to Kr), mass-weighted Hessian,
  translations and rotations about the centre of mass projected out (3 for an
  atom, 5 for a linear molecule, else 6), the rest diagonalised. Frequencies
  in cm-1, imaginary ones negative. `transrot_residual` lists the smallest
  frequencies of the unprojected mass-weighted Hessian: translations come out
  below 1 cm-1 for HF and up to a few cm-1 for DFT, rotations a few to 20 cm-1
  because the gradient at a converged geometry is not exactly zero (the
  rotational term is proportional to it); `max_gradient` is printed too.
* IR intensity: with the Cartesian dipole derivative `dmu/dx` and the mode's
  Cartesian displacement `dx` (mass-weighted eigenvector over sqrt(m)),
  `I = N_A/(12 eps0 c^2) |dmu/dQ|^2` in km/mol (the integrated absorption
  coefficient, what ORCA and Gaussian print).
* Thermochemistry at `temperature`, `pressure`: ideal gas, rigid rotor,
  harmonic oscillator (no quasi-RRHO treatment of low modes; ORCA applies
  Grimme's by default, switch it off with `%freq QuasiRRHO false end` to
  compare). Translation Sackur-Tetrode; rotation with the rotational symmetry
  number sigma (linear `q = T/(sigma Theta)`, otherwise
  `q = sqrt(pi)/sigma T^1.5/sqrt(Theta_A Theta_B Theta_C)`); vibration over
  the real modes only (imaginary ones are left out); electronic `ln(2S+1)`
  from the multiplicity (spin only, no orbital degeneracy). Reported:
  `zero_point_energy` (the correction), `thermal_energy` (electronic energy +
  ZPE + thermal, ORCA's "total thermal energy"), `enthalpy` (+ kT),
  `entropy_ts` (T S) and `gibbs_energy` (H - T S), all in hartree, with the
  entropy parts in hartree/K. The symmetry number is the number of proper
  rotations that map the molecule onto itself (0.02 bohr tolerance), 2 for a
  linear molecule with an inversion centre; `symmetry_number N` overrides it.
  A DFT geometry that is a little distorted can still be found symmetric by
  ecce-qm where ORCA, with a tighter test, finds a subgroup (CH4 B3LYP: ORCA
  sigma 4, ecce-qm 12).
* Output: `vib_transrot`, `vib_imaginary`, `hessian_step`, `max_gradient`,
  `begin transrot_residual`, `begin vibrations` (every one of the 3N modes:
  index, frequency, IR intensity; the translations and rotations first with
  0, as in ORCA, so ECCE's VIBFREQ, VIBIR and VIB line up row by row),
  `begin normal_modes` (per mode the x y z of every atom, Cartesian
  displacements of unit length), then the thermochemistry keys above. With
  `--molden` the file also gets `[FREQ]`, `[FR-COORD]`, `[FR-NORM-COORD]`
  and `[INT]` for the real vibrations.
* Cost: `6N` gradient evaluations at the price of one SCF each (the guess makes
  them cheap); benzene B3LYP/6-31G* takes minutes, see the table in
  `docs/qm/benchmarks.md` for the cap proposed for student machines.
* Tests: `qm_unit` (model Hessians, symmetry numbers, Sackur-Tetrode),
  `tests/qm/freq_test.py` (frequencies, IR intensities, ZPE and the thermal
  quantities against ORCA 6.1.1 at ORCA's own TightOpt geometry,
  `freq_oracle.json`, made by `gen_freq_oracle.py`; triplet O2 restricted
  open-shell against the curvature of its own energies, since ORCA has no
  ROHF Hessian).

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

## In ECCE: the code ECCE-QM

The engine is registered as the code **ECCE-QM** (`data/client/cap/
ECCE-QM.edml`; the wiring is listed in `docs/claude/codereg/
ecce-qm-is-a-bundled-code.md`). A student chooses New ECCE-QM Calculation in the
Organizer; the Calculation Editor offers

* theory: DFT (the default) or HF, and for DFT the functional SVWN, PBE,
  B3LYP (default) or PBE0 in Theory Details, which also has one tick for
  unrestricted orbitals (UHF/UKS; open shells are otherwise restricted
  open-shell, ROHF/ROKS);
* run type: Energy, Gradient, Geometry (optimisation, the geometry trace and
  energy plot appear as for any other code), Vibration (frequencies, IR
  intensities, normal modes and thermochemistry at the given geometry) or
  GeoVib (optimisation, then the same);
* basis set: STO-3G, 3-21G, 6-31G* (default), cc-pVDZ or def2-SVP from the
  Basis Set menu (the Basis Set Tool is not used); an element a basis does not
  cover, or that would need a core potential (def2 beyond Kr), is named before
  launch;
* charge and multiplicity as for any code.

The job runs on `localhost` with no registration, no server and no queue, in
local-data mode and with a data server alike; `ecce-qm` is `$ECCE_HOME/bin/
ecce-qm` (`ECCE_ECCEQM` or an `ecce-qm:` line in CONFIG.<machine> replaces it).
The deck is written by `scripts/parsers/ai.ecceqm`; the basis set is always
written out in a `basis_data` block (`shell <El> <S|P|D|F|G>` then one
`exponent coefficient` row per primitive) so the run needs no library. The
output is parsed by `ecceqm.desc`/`ecceqm.parse`: total and component energies,
orbital energies and occupations, MO coefficients, Mulliken charges, dipole,
S**2. The MO correlation diagram is not offered for this code
(`<SupportsMODiagram>false</SupportsMODiagram>`, a flag any code can set);
orbital lists, energies and orbital pictures are.

Tests: `tests/parsers` (fixtures and goldens), `tests/basis`, `tests/dialogs`,
`ctest -R launch_ecceqm` (water and triplet O2 through Launch in a local data
folder, energies against `tests/qm/oracle.json`, every orbital's c.S.c = 1 from
ECCE's own stored basis, so trace(P S) is the electron count).
