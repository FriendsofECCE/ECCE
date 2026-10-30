---
type: map
title: "results.h5: ECCE's internal results format (format_version 1), canonical AO order, converters"
area: codereg
paths: ["include/tdat/ResultsH5.H", "src/tdat/resultsh5", "tests/resultsh5"]
issues: []
---
**Purposes.** (1) A later calculation in any code can start from what an
earlier one left: geometry always, Hessian and orbitals as guesses where
the bases allow. (2) One small file replaces bulky ones (G16 .chk, ORCA
.gbw), so `results.h5` must be smaller than the source it replaces.

Library `ecceresultsh5` (no wx, no ECCE object graph; compiles out with
`-DECCE_WITH_HDF5=OFF`), CLI `ecce-results`, test `ctest -R resultsh5`
(`tests/resultsh5/run_tests.py`, one `ecce-results selftest` per fixture).
Overlaps come from `EspField::overlapOf`, the viewer's own code: there is
no second overlap implementation.

### Layout (atomic units: bohr, hartree, e; frequencies cm-1)
Root attributes: `format_version`=1, `nelectron_alpha/beta` when known.
Gzip-9 + shuffle on arrays above 1 kB; smaller ones are compact datasets.
Empty datasets are never written.

- `provenance` (attributes): code, code_version, method, basis_name,
  source_file, source_sha256, converter_version, ao_convention (which
  source normalisation the converter detected), and anything else.
- `nucleus`: atomic_number, charge (only if an ECP/GTH changes it), label.
- `geometry`: coord [nframes, natoms, 3], energy [nframes]. Scans, IRC and
  optimisation paths are further frames; MOs, Hessians, charges attach to
  a frame by a `geometry` attribute. TREXIO `nucleus/coord` is frame 0.
- `cell`: lattice [3,3] rows a,b,c, attribute pbc (bit mask x=1,y=2,z=4).
- `basis`: attributes type (GTO|STO|none), orthonormal (semiempirical:
  S = 1, no AO overlap exists), name. Datasets shell_nucleus (0-based),
  shell_ang_mom, shell_cartesian (only if any), shell_n (STO), shell_nprim,
  exponent, coefficient (flat over primitives). TREXIO uses shell_index per
  primitive; this layout uses a count per shell, which is smaller.
- `ecp`: attribute kind (ECP|GTH|PP), datasets nucleus, name, ncore,
  parameters. Stored, never interpreted by the invariants.
- `mo/<n>`: attributes spin (0 alpha, 1 beta), geometry; datasets
  coefficient [nmo, nao], energy, occupation, symmetry (if the source had
  labels). TREXIO has one flat `mo` group with a spin array; here each
  spin/geometry is its own subgroup.
- `hessian/<n>` (dataset: upper triangle of the 3N x 3N Cartesian Hessian,
  row-wise, Eh/bohr2; attributes geometry, method), `gradient/<n>`,
  `charges/<scheme>`, `multipoles/<n>` (dipole, quadrupole xx xy xz yy yz
  zz), `vib/<n>` (frequency, ir_intensity, reduced_mass, mode [nmodes,3N]),
  `tensors/<name>` (polarizability, Raman, quadrupole), `bands`
  (plane-wave codes: kpoint, weight, energy/occupation [spin,k,band],
  fermi_energy; never wavefunctions), `trajectory/<n>` (reference only:
  file, sha256, nframes, dt; coordinates stay in .xtc/.trr), `series/<n>`
  (name, unit, time, value: MD energies, temperature, MSD).
- Derived quantities (density, overlap) are not stored.

### Canonical AO order and normalisation
Atom-major as the basis lists them, shell by shell. Spherical shells:
m = 0, +1, -1, +2, -2, ... (the Gaussian/ORCA/Molden order) except l = 1,
which is x, y, z everywhere. Real solid harmonics (Helgaker 6.4.47,
no Condon-Shortley phase). Cartesian shells: powers in descending order
xx xy xz yy yz zz (TREXIO), not Gaussian's xx yy zz xy xz yz; the readers
permute. Every AO has unit self-overlap. Shell coefficients are stored on
UNNORMALISED primitives (only their ratios matter). This is NOT ECCE's
per-code TGBSConfig column order (MoAoOrder/BasisFlatten), which depends
on the code's angular function table: the viewer needs a map from this
order to its own before it can read an h5 directly (not done in stage 1).

Source conventions differ (primitive normalisation, whether Cartesian
xy is scaled as xx, ORCA's f(+3)/f(-3) signs), so each reader tries the
candidates and keeps the one whose occupied MOs are orthonormal; the
choice goes into `provenance/ao_convention`. A wrong guess cannot pass:
the gates below fail it.

### Converters (stage 1)
| source | read | write |
|---|---|---|
| Molden (xtb --molden, orca_2mkl -molden) | yes, 5D/7F/9G flags, Cartesian too | yes |
| Gaussian .fchk | yes: basis (incl. SP), MOs, force constants, gradient, charges, dipole, Vib-E2/Vib-Modes | no |
| ORCA .hess | yes: merges Hessian, frequencies, modes, IR into an existing file | no |
Not done: MOPAC (STO), CP2K, QE bands, GROMACS references, NWChem, ECCE-QM
(writes h5 natively later), Cartesian g in fchk, ECPs from Molden.

### Gates every converter must pass (tests/resultsh5)
trace(P S) = N per spin (1e-6), C'SC = 1 over occupied MOs (1e-6),
frequencies from the stored Hessian within 0.5 cm-1 of the code's own,
h5 write/read identical, Molden export/re-import coefficients to 1e-10,
results.h5 no larger than the parsed sources plus 4 kB. The 4 kB is HDF5's
fixed cost (about 0.2 kB per group/dataset/attribute list), which only a
3-atom file notices. Each converter needs an asymmetric fixture (low
symmetry with d and f functions and an ECP atom): water-like symmetry hides
p/d swaps and sign errors.
