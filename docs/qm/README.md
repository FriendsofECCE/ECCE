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
explicitly. `+` and `*` in a name are written P and S in the library, so
`6-31+G*` is `6-31PGS-AGG.BAS`; where a whole-set `-AGG` file exists it is
preferred (the plain `AUG-CC-PVDZ.BAS` holds only the diffuse functions), and
an augmented name with no complete set is refused. ECPs (the def2 heavy elements) are not supported.
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
  with Bragg-radius size adjustment. Default 99 x 590 per atom, with the angular
  grid thinned close to each nucleus (SG-1 style regions; `grid_prune false` for the full grid).
  Shell-pair Schwarz screening (1e-12) of the two-electron integrals and gradient.
* Output: total and component energies, orbital energies and occupations,
  MO coefficients, Mulliken net charges (Loewdin, CHELPG, MK on request), dipole, <S^2>; a Molden file.

### Extra charge sets (`charges` keyword)

`charges loewdin chelpg mk` (any subset) adds charge sets to the output
(`begin charges <scheme>`, `src/qm/charges.cpp`); Mulliken is always printed.

* Loewdin: diagonal of S^1/2 P S^1/2 summed per atom.
* CHELPG: the potential from the nuclei and `int1e_rinv` at points on a cubic
  grid of 0.3 A, outside the CHELPG radii (H 1.45, C 1.50, N/O/F 1.70, Bondi
  elsewhere) and within 2.8 A of some atom; least squares with the total charge
  fixed by a Lagrange multiplier. Not rotation invariant, like any CHELPG.
* Merz-Kollman: points on the surfaces at 1.4, 1.6, 1.8, 2.0 times the MK radii
  (H 1.20, C/N 1.50, O 1.40, F 1.35, Bondi elsewhere), about one point per
  square Angstrom from a Fibonacci spiral, minus those inside another atom's
  scaled sphere; same fit.
* Against ORCA 6.1.1 (Loewdin, CHELPG), H2O, NH3, CO, HF at HF and
  B3LYP/6-31G*: Loewdin within 1e-5, CHELPG within 0.005 e
  (`tests/qm/charges_test.py`, ctest `qm_charges`).
* ECCE keys: LOEWDIN, ESPFIT (CHELPG, the key ORCA's CHELPG uses) and MKFIT.

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

### Dummy atoms and constraints (#309)

* `X` in the geometry block is a dummy atom (Z = 0). `main.cpp` hands the SCF,
  gradient, charges and vibrations `Molecule::without_dummies()`; the
  optimiser and the output keep the full list, so atom numbers match ECCE's
  (`begin atoms`, `begin charges`, `begin geometry`, `begin gradient` carry the
  dummy, with charge 0 and gradient 0; `begin vibrations`/`normal_modes` and
  Molden list the real atoms only).
* `constraints ... end` (`bond`, `angle`, `torsion`, `fix`; 1-based, dummies
  counted; see help/src/ecce-qm-files.md). In `optimizer.cpp` the step is
  taken in the null space of the constraint gradients (plus translation and
  rotation when no atom is fixed) and the values are then restored by Newton
  iterations with minimum-norm corrections. A dummy atom no bond/angle/torsion
  names is fixed in space. Convergence uses the gradient projected on that
  space. `optfreq` with constraints is refused.
* Test: `tests/qm/constraint_test.py` (NH3 + X, X-N-H fixed at 100/110/120
  degrees, HF and B3LYP/6-31G* against NWChem zcoord; water bond and angle
  against ORCA `%geom Constraints`; agreement 1e-7 Eh and 1e-5 angstrom).

### Relaxed scan (#450)

* `scan ... end` with one line `bond|angle|torsion <atoms> <from> <to> <n>`
  (runtype optimize only). `main.cpp` loops `optimize_geometry` over the n
  values, adding the scanned coordinate to the constraints with that value; each
  point starts from the previous point's optimised geometry and, through
  `evaluate`, its converged orbitals. A point that does not converge is
  recorded `no` and the scan goes on; the exit status is 1 then.
* Output, after each point: `begin scan_geometry` (point, value, the atoms in
  the `begin geometry` layout), the whole `begin scan_table` rewritten
  (point, value, energy, converged; the unit in its header comment) and
  `progress scan point=k/n`. The final output is that of the last point.
  `ecceqm.desc` makes SCANGEOM of every `scan_geometry`, SCANCOORD and
  SCANENERGY of the last table, and the `scan` stage of the progress box.
* Each point's converged orbitals follow its `scan_geometry`, numbered as it:
  `begin point_orbitals <spin>  # point k` (index, energy, occupation; spin
  restricted, or alpha then beta) and `begin point_mo_coefficients <spin>`
  (the `mo_coefficients` layout). An IRC writes them after its `irc_geometry`
  blocks, in path order. `ecceqm.desc` makes PTORB/PTORBBETA and
  PTMO/PTMOBETA of them, one table per point; the orbital panels read the
  point selected in the Relaxed Scan / IRC Path panel from these.
  `ai.ecceqm` writes the block from the Builder's scan constraint (n steps
  of a given size are n+1 points, as for ORCA); one scanned coordinate only.
* A diatomic whose bond is held has no free coordinate: the optimiser then
  ends at the first evaluation (the model-Hessian step is skipped for an empty
  space).
* `guess_mix true` (unrestricted, na = nb): the alpha HOMO and LUMO of the
  start orbitals are rotated by 45 degrees, at every point of a scan as well,
  so a spin-polarised solution is found where one exists (H2 beyond the
  Coulson-Fischer point) and the SCF returns to the closed-shell one where it
  does not. Input only; the Calculation Editor does not offer it.
* Tests: `tests/qm/scan_test.py` (relaxed scans against ORCA 6.1.1,
  `scan_oracle.json` from `gen_scan_oracle.py`: ethane H-C-C-H HF/STO-3G and
  B3LYP/6-31G*, H2 RHF and UHF), `scan_deck_test.py` (ai.ecceqm to deck to
  engine), the parser case `ecceqm-h2o2-scan` and
  `tests/apps/ecceqm_scan_panel_test.py` (Relaxed Scan panel).

### Transition-state search (#452)

* `runtype ts`: a first-order saddle point from the structure in the input,
  then the frequency run, in the form `optfreq` uses (analytic Hessian for
  closed-shell RHF, finite differences otherwise; `numeric_hessian true`
  forces the latter). No constraints, no dummy atoms.
* The step is partitioned RFO: uphill along one Hessian mode, downhill along
  all others, capped by the trust radius. The Hessian is computed at the
  start; `ts_hessian_every n` recomputes it every n steps (default 1 for
  closed-shell RHF, where the analytic Hessian is cheap, otherwise 0: Bofill
  updates only). The model is also recomputed whenever the updated Hessian has
  lost its last negative eigenvalue. The followed mode at the first step is
  the lowest, or the `ts_mode n` (n-th lowest, from 1), or the one that
  overlaps most with `ts_follow bond 1 2` (also `angle`, `torsion`; for
  instance the coordinate of the scan the start came from; the lowest mode is
  usually the better choice, since a scanned bond overlaps most with the
  symmetric stretch). Afterwards it is the mode with the largest overlap with
  the previous step's (the lowest if none overlaps by more than 0.5). A step
  whose energy change has the opposite sign to the model's is turned down and
  retried shorter (at most three times in a row). Convergence is the
  minimiser's (`opt_conv normal|tight`, `maxsteps`).
* Output as for an optimisation (`progress opt ...` is the Progress box's opt
  stage, GEOMTRACE, TE), then the frequency blocks. The run fails (exit 1,
  message on stderr) when the final frequencies do not show exactly one
  imaginary mode.
* The Calculation Editor offers it as the run type `Transition State
  (Opt-TS)`; the structure in the editor is the guess. The QST and NEB-TS run
  types (reactant/product structures, #419) are not offered for ECCE-QM.
* Tests: `tests/qm/ts_test.py` (HCN to HNC against ORCA 6.1.1 `OptTS`,
  `ts_oracle.json` from `gen_ts_oracle.py`).

## Frequencies, IR intensities, thermochemistry (stage 3)

`runtype frequency` computes the harmonic vibrations at the given geometry,
`runtype optfreq` optimises first (and does nothing more if the optimisation
did not converge). Further keywords: `hessian_step` (bohr, default 0.005),
`numeric_hessian` (`true` forces finite differences; closed-shell RHF is analytic by default, also `ECCE_QM_NUMERIC_HESSIAN` in the environment), `temperature` (K, 298.15), `pressure` (atm, 1), `symmetry_number` (default: worked out).

* Hessian (`vibration.cpp`): central differences of the analytic gradient over
  every Cartesian coordinate, step 0.005 bohr, so 6N gradient evaluations
  (`2 x 3N`). The same displaced calculations give the dipole derivatives.
  Every displaced SCF starts from the orbitals of the reference geometry and
  is converged to 1e-8 (orbital gradient) and 1e-11 hartree, tighter than an
  energy needs, so that the noise in the Hessian stays below 0.1 cm-1. The
  step is a compromise: smaller ones amplify the SCF noise, larger ones the
  cubic terms; 0.0025 and 0.005 agree to 0.05 cm-1 for water B3LYP, 0.02 is off by 0.8, and
  0.005 reproduces ORCA's analytic frequencies to 0.2-0.8 cm-1.
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
* Cost: `6N` gradient evaluations, each an SCF from the reference orbitals plus
  a gradient. Wall time, 6-31G*, default threads, on an 8-core machine that was
  shared with other jobs (load 4 to 16, so read them as a 4-core laptop; the
  frequency times of water and ethanol were measured on a quieter machine):

  | molecule (atoms, basis functions) | method | energy | optimisation | one gradient | frequencies |
  |---|---|---|---|---|---|
  | water (3, 18) | HF | 0.5 s | 1.6 s (5 steps) | 0.2 s | 4 s |
  | water (3, 18) | B3LYP | 1.2 s | 4.9 s (5) | 0.7 s | 19 s |
  | ethanol (9, 54) | B3LYP | | | | 247 s (quiet machine) |
  | benzene (12, 96) | HF | 3.0 s | 28 s (4) | 4.2 s | 604 s |
  | benzene (12, 96) | B3LYP | 19 s | 82 s (3) | 16 s | 1382 s |
  | cyclohexane (18, 108) | HF | 0.9 s | 33 s (6) | | 644 s |
  | cyclohexane (18, 108) | B3LYP | 18 s | 160 s (6) | 31 s | not run (about 6N x 40 s) |

  Frequency time grows as `N^2 x nbf` (grid and integrals per gradient times
  `6N` gradients): about `0.06 N^2 nbf` seconds for DFT on 8 idle cores and a
  third of that for HF. A pointer for a student laptop (about half that
  machine): warn before a frequency run when `N^2 x nbf` exceeds 5000 for DFT
  (ethanol B3LYP is 4400, 4 minutes; benzene B3LYP 13800, 13 to 23 minutes) or
  15000 for HF, and before any run beyond 25 atoms or 150 basis functions (the
  integrals are held in memory: about 170 MB at 114 functions, 4 GB at 250).
  The table is the basis for those numbers; the program itself enforces
  nothing, but ECCE's Launcher shows the warning before Launch ("Launch
  anyway" or Cancel), with the time as the 8-core figure to twice that.
* Tests: `qm_unit` (model Hessians, symmetry numbers, Sackur-Tetrode),
  `tests/qm/freq_test.py` (frequencies, IR intensities, ZPE and the thermal
  quantities against ORCA 6.1.1 at ORCA's own TightOpt geometry,
  `freq_oracle.json`, made by `gen_freq_oracle.py`; triplet O2 restricted
  open-shell against the curvature of its own energies, since ORCA has no
  ROHF Hessian).

## Polarisability and Raman activities (#248, stages 1 and 2)

`polarizability true` (any run type) adds the static dipole polarisability of
the final geometry; `raman true` (`frequency`, `optfreq`) adds Raman
activities and depolarisation ratios. `field_step` (a.u., default 0.001).

* Field (`scf.cpp`, `polar.cpp`): `ScfOptions::field` adds `F.r` to the core
  Hamiltonian (electron energy +F.r, dipole integrals about the coordinate
  origin, the same origin as the printed dipole). `static_polarizability`
  runs six SCFs, +-F along x, y, z, started from the field-free orbitals and
  converged to 1e-8 / 1e-11 like the displaced points of a frequency run;
  `alpha_ij = [mu_i(+F e_j) - mu_i(-F e_j)] / 2F`, symmetrised. Works for RHF,
  UHF, ROHF and every functional (the XC potential is the ordinary one, only the
  density changes). The central difference removes the hyperpolarisability
  term; the remaining error is of order gamma F^2 / 6 (about 1e-4 bohr^3 for
  water).
* Raman (`vibration.cpp`): `finite_difference_derivatives` also differences
  `PointEval::alpha` at the 6N displaced geometries; `analyse_vibrations`
  projects d(alpha)/dx on each mode's `q/sqrt(m)` (bohr^2/sqrt(amu), then
  angstrom^2/sqrt(amu)) and forms a' (mean) and g' (anisotropy, symmetrised
  tensor): S = 45 a'^2 + 7 g'^2 in angstrom^4/amu, depolarisation ratio
  3 g'^2 / (45 a'^2 + 4 g'^2), ORCA's definitions. Cost: seven SCFs per
  displaced geometry instead of one SCF and a gradient (water B3LYP/6-31G*,
  quiet 8-core machine: 4.7 s frequencies, 30 s with Raman).
* Output: `polarizability_isotropic`, `polarizability_anisotropy`,
  `begin polarizability` (3x3, bohr^3), and `begin raman` (index, activity,
  depolarisation, all 3N rows like `vibrations`, translations and rotations 0).
  `ecceqm.desc` reads them into POLARIZ (XX XY XZ YY YZ ZZ, the ellipsoid in
  the Builder), VIBRAM and VIBRAMDEPOL.
* ECCE: Runtype Details (`ecceqmruntype.py`, Vibration and GeoVib) has a
  "Raman activities" tick, `ES.Runtype.Vibration.UseRaman`, which
  `ai.ecceqm` writes as `raman true`; `verifyinput` knows the keywords; the
  Launcher's size warning counts Raman as seven times the frequency run.
* Tests: `tests/qm/raman_test.py` (ctest `qm_raman_quick`, `qm_raman`) against
  `polar_oracle.json` (ORCA 6.1.1 `%elprop Polar 1`, and NWChem 7.2.3 response
  module) and `raman_oracle.json` (ORCA Freq at HF, NumFreq at B3LYP), made by
  `gen_raman_oracle.py`; `qm_raman_deck` for the deck. Measured: polarisability
  within 9e-4 bohr^3 of ORCA (CO B3LYP; the others 3e-4 or better) and 1e-4 of
  NWChem; activities of water, ammonia and methane within 9e-4 relative,
  depolarisation within 2e-4. Degenerate modes are compared as groups (the
  split inside a degenerate set is arbitrary). NWChem's `task scf raman` stops
  in its response driver on these inputs, so Raman has no NWChem reference.
  O2 UHF polarisability agrees with ORCA to 1e-4 bohr^3; ORCA has no ROHF one.

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
The file ends with `end_of_output`.
Lines starting `progress ` are for ECCE's job monitor (the Organizer's
Progress box, #284), written and flushed as the run goes:
`progress scf cycle=N energy=E dE=x/target grad=g/target [converged=yes|no]`,
`progress opt step=N energy=E maxgrad=a/t rmsgrad=a/t maxstep=a/t
rmsstep=a/t [dE=a/t]` and `progress opt step=N converged=yes|no` at the end,
`progress freq point=k/n` and finally `progress freq point=n/n done=yes
real=R imaginary=I zpe=Z`. ECCE reads MO coefficients from a code's
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
agrees to 2e-8, ORCA gives -76.272015. The difference
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
