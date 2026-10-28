# Results file format (results.h5)

Draft for the wiki. Developer-facing; the user page is `help/src/results-file-format.md`.

## Purposes

1. **Workflows.** A later calculation in any code can start from what an earlier one left: geometry always, Hessian and orbitals as guesses where the bases allow. The reader/writer pair and "export as a start for code X" matter more than completeness of stored properties.
2. **Space.** `results.h5` replaces bulky files (G16 `.chk`, ORCA `.gbw` kept only for MORead). It must be no larger than the smallest source format it replaces.

## Layout version 1

One HDF5 file per calculation (serial C API, `-DECCE_WITH_HDF5=ON`, compiled out when OFF). Atomic units throughout; frequencies in cm-1. Root attribute `format_version` = 1; a reader refuses other values. Arrays above 1 kB are chunked with shuffle + gzip 9; smaller ones are compact datasets (data inside the object header); empty datasets are never written; file created with the latest library format and small metadata blocks.

The full group/dataset list with shapes and units is in the user page. Source of truth for the code is `include/tdat/ResultsH5.H` (the model structs) and `src/tdat/resultsh5/ResultsH5Io.C` (the layout).

### Mapping to TREXIO

| TREXIO | results.h5 | note |
|---|---|---|
| `nucleus/charge`, `coord`, `label` | `nucleus/charge` (only if an ECP/GTH changes it), `nucleus/atomic_number`, `nucleus/label`; coordinates in `geometry/coord[frame]` | several frames; `nucleus/coord` is frame 0 |
| `basis/shell_ang_mom`, `shell_factor`, `nucleus_index` | `basis/shell_ang_mom`, `shell_nucleus` | no shell_factor: every AO is renormalised to 1 |
| `basis/shell_index` per primitive, `exponent`, `coefficient` | `shell_nprim` per shell, `exponent`, `coefficient` | a count per shell is smaller; coefficients on unnormalised primitives |
| `ao/cartesian` | `basis/shell_cartesian` per shell | mixed bases allowed |
| `ecp/*` | `ecp/*` | kind may also be GTH or PP |
| `mo/coefficient`, `energy`, `occupation`, `spin`, `symmetry` | `mo/<n>/...` | one subgroup per spin and geometry |
| `electron/up_num`, `dn_num` | root attributes `nelectron_alpha`, `nelectron_beta` | |

ECCE groups with no TREXIO counterpart: `hessian/<n>`, `gradient/<n>`, `charges/<scheme>`, `multipoles/<n>`, `vib/<n>`, `tensors/<name>`, `cell`, `bands`, `trajectory/<n>`, `series/<n>`, `provenance`. Hessians store the upper triangle of the Cartesian matrix row by row.

The model covers all target codes now, even where the converter is still missing: `basis/type` is GTO, STO (MOPAC; needs `shell_n`) or none (plane waves); `basis/orthonormal` marks semiempirical bases whose S is 1; `ecp/kind` allows GTH; `cell` holds lattice and pbc; `bands` holds QE band energies per k point and spin; `trajectory` and `series` hold MD (reference to the `.xtc`/`.trr`, never the coordinates); several `geometry` frames hold scans, IRC and optimisation paths, and MOs, Hessians and charges attach to a frame by a `geometry` attribute.

## Canonical AO order and normalisation

- Atom-major in basis order, shell by shell.
- Spherical shells: m = 0, +1, -1, +2, -2, ... (Gaussian, ORCA and Molden order), real solid harmonics without Condon-Shortley phase. l = 1 is x, y, z.
- Cartesian shells: powers xx xy xz yy yz zz (TREXIO order), not Gaussian's xx yy zz xy xz yz. Readers permute.
- Each AO has unit self-overlap. Shell coefficients are for unnormalised primitives.

This is not the viewer's per-code column order (`MoAoOrder`, `BasisFlatten`, which depends on each code's angular function table). Stage 1 does not connect the viewer; that needs a map from this order to TGBSConfig's.

### Source conventions

Programs disagree on primitive normalisation, on whether Cartesian xy is scaled like xx, and (ORCA) on the signs of f(+3) and f(-3). The readers do not trust documentation: `normaliseSource` tries raw/normalised primitives x unit/monomial AOs, keeps the one whose occupied MOs are orthonormal, then (only if still above 1e-6) searches a shared sign pattern over the spherical f functions. The result is written to `provenance/ao_convention`. ORCA 6.1.1 water-like: raw primitives, unit AOs; f sign pattern 96 (m = +3 and -3 flipped). G16 and xtb: normalised primitives, unit or monomial AOs.

## Invariants (library `check()`, `ecce-results check`)

- tr(P S) = N per spin, from the occupations, 1e-6.
- C'SC = 1 over occupied MOs, 1e-6. S is built from the canonical basis with `EspField::overlapOf`, the same integrals the viewer uses.
- Frequencies from the stored Hessian (mass weighted, translations and rotations projected out; most abundant isotope masses for Gaussian, standard atomic weights otherwise) against the code's own, 0.5 cm-1.

## Model coverage and converter plan

| Code | What is stored | Source | Converter |
|---|---|---|---|
| xtb | geometry, basis, MOs | `--molden` | done |
| ORCA | geometry, basis, MOs, Hessian, vib | `orca_2mkl -molden`, `.hess` | done |
| Gaussian 16 | geometry, basis, MOs, force constants, gradient, charges, dipole, vib | `.fchk` | done (no Cartesian g) |
| ECCE-QM | MOs, Hessian, CPCM, polarizabilities and Raman tensors (`tensors/`) | written natively | later, no converter |
| MOPAC | STO basis (`basis/type` = STO, `shell_n`), `orthonormal`, MOs, charges | `.aux` | planned |
| NWChem | MOs from its Molden output or ECCE's parsed MO properties; Hessian from `.hess`/`.fd_ddipole` | Molden, `.hess` | planned |
| CP2K | GTO basis, GTH `ecp`, `cell`, k = Gamma, MOs | Molden output | planned |
| Quantum ESPRESSO | `cell`, k-points and weights, `bands`, Fermi energy, cutoffs, pseudopotential names and checksums; no wavefunctions (the `.save` directory is referenced in `provenance`) | XML output | planned |
| GROMACS | `cell`, `series` (energy, temperature, pressure, MSD, speed), `trajectory` reference | `.edr`, `.xtc` | planned |
| Any MD (xtb, CP2K) | `trajectory` reference | | same as GROMACS |
| Scans, IRC, optimisation | several `geometry` frames with energies | | with each code's converter |

## Sizes

Bytes, measured by `ecce-results selftest` (the test prints them). "h5" is with shuffle + gzip 9; "h5 raw" is without. Fixtures: the `.chk`/`.gbw` column is the real file of the same calculation, not in the repository.

| Fixture | Source parsed | h5 | h5 raw | .chk / .gbw |
|---|---|---|---|---|
| xtb water | molden 3115 | 6184 | 6184 | |
| xtb CH2FCl | molden 18951 | 9789 | 9981 | |
| ORCA water, B3LYP/def2-SVP | molden 17505 + hess 5932 | 13430 | 13604 | gbw 1009836 |
| ORCA HOI, HF/def2-SVP (ECP) | molden 56062 | 22240 | 23479 | gbw 1012239 |
| ORCA HOCl, HF/def2-TZVP (f) | molden 144988 | 48029 | 51999 | gbw 1045892 |
| G16 water, B3LYP/def2SVP freq | fchk 109917 | 13909 | 16124 | chk 679936 |
| G16 water, HF/6-31G(d) 6D | fchk 95131 | 10189 | 11244 | chk 651264 |
| G16 HOI, HF/def2SVP (ECP) | fchk 167216 | 20678 | 25275 | chk 757760 |
| G16 HOCl, HF/def2TZVP (f) | fchk 107018 (trimmed, 307809 untrimmed) | 40210 | 53389 | chk 970752 |

The rule is `h5 <= parsed sources + 4096`. The 4 kB is HDF5's fixed cost of about 0.2 kB per group, dataset or attribute list; only the 3-atom xtb file (3.1 kB of text) exceeds its source, by 3.1 kB. Precision: MOs are float64 everywhere; the sources have 8 (fchk) to 12 (molden) digits. float32 is not used because fchk has 8 digits.

## Adding a converter: checklist

1. Copy the nearest reader in `src/tdat/resultsh5/ResultsConvert.C` (`readMolden` for text with per-AO blocks, `readFchk` for section-based files). Declare it in `include/tdat/ResultsH5.H` and add the extension in `readAny` in `ecce_results.C`.
2. Fill the model fields and nothing else: `atomicNumber`, `label`, `coord` (bohr, one frame per geometry), `basis.shells` (0-based atom, `l`, `cartesian`, `exponent`, `coef` as printed), `mos` (`coef` in canonical AO order after permuting from the source order; `occupation`; `energy`), then `hessian`/`vib`/`charges` if the source has them. Convert units at read time (Angstrom, kcal, amu -> atomic units). Call `stamp()` for provenance.
3. Do not normalise by hand. Call `normaliseSource(m, note, err)` after the basis and MOs are filled and store `note` in `provenance/ao_convention`. If the invariants still fail, the usual causes are: AO order within a shell (d, f, Cartesian), the sign of some spherical functions, or a basis the reader dropped (SP shells, ECP-only atoms).
4. Fixtures, small (under 200 kB; trim unused sections and say so): the water-like one, and an asymmetric one (no symmetry, d functions, f functions via a TZVP basis, one ECP atom, e.g. HOI with def2-SVP and HOCl with def2-TZVP). Symmetry hides p/d swaps and sign errors, so the asymmetric fixture is what proves the reader.
5. Register the fixture in `tests/resultsh5/run_tests.py` (parsed inputs; extra files only for size).
6. Gates, all of which `ecce-results selftest` runs and the test fails on:
   - tr(P S) = N and C'SC = 1 to 1e-6 per spin
   - frequencies from the Hessian against the code's, 0.5 cm-1, if the source has both
   - h5 write and read identical
   - Molden export and re-import: coefficients equal to 1e-10
   - results.h5 no larger than the parsed sources + 4096 bytes
7. Put the per-code row in the coverage tables (user page and this page) and in `docs/claude/codereg/results-h5-format.md`.

A converter is done only when all of step 6 passes on both fixtures. Negative check: break the conversion (swap two components, flip one sign) and confirm the gate fails; the ORCA f-function convention was found exactly that way.
