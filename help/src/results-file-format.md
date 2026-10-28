# The results file format (results.h5)

`results.h5` is ECCE's own file for what a calculation produced. It has two
purposes.

1. **Start the next calculation from the last one.** A later calculation in
   any code can take the geometry from an earlier one, and the Hessian and
   the orbitals as a starting guess where the basis sets allow it.
2. **Take less space.** One small file replaces bulky ones such as a
   Gaussian checkpoint file or an ORCA `.gbw` file. A `results.h5` is never
   larger than the smallest file it replaces (for the smallest molecules
   a few kilobytes of fixed file overhead are the only exception).

The file is in HDF5 format, so any HDF5 tool reads it. All values are in
atomic units: lengths in bohr, energies in hartree, charges in electrons.
Frequencies are in cm-1. The attribute `format_version` at the top of the
file is 1.

## What the file holds

| Group | Contents |
|---|---|
| `provenance` | code, code version, method, basis set name, name and sha256 of the source file, converter version |
| `nucleus` | `atomic_number`, `label`, and `charge` only when a pseudopotential changes it |
| `geometry` | `coord` (frames x atoms x 3, bohr), `energy` (one per frame, hartree). Scans and paths are further frames |
| `cell` | `lattice` (3 x 3, bohr), `pbc` flags, for periodic calculations |
| `basis` | shells: atom, angular momentum, Cartesian or spherical, exponents, coefficients |
| `ecp` | pseudopotential kind and parameters |
| `mo/<n>` | `coefficient` (MOs x AOs), `energy`, `occupation`, `symmetry` labels when the code gave them; attributes `spin` and `geometry` |
| `hessian/<n>` | Cartesian Hessian, upper triangle row by row, hartree/bohr2; attributes `geometry`, `method` |
| `gradient/<n>`, `charges/<scheme>`, `multipoles/<n>` | gradient, atomic charges (Mulliken, ESP, ...), dipole and quadrupole |
| `vib/<n>` | `frequency`, `ir_intensity`, `reduced_mass`, `mode` (modes x 3N) |
| `tensors/<name>` | polarizability, Raman and other tensors |
| `bands` | plane-wave codes: k-points, weights, band energies and occupations, Fermi energy |
| `trajectory/<n>`, `series/<n>` | molecular dynamics: a reference to the trajectory file (name, checksum, frames, time step; the coordinates are not copied) and time series such as energy and temperature |

Density and overlap matrices are not stored. They are computed from the
basis when needed.

Everything that belongs to one geometry carries the attribute `geometry`,
the index of the frame in `geometry/coord`. Empty groups are left out.

## Orbitals: the AO order

The columns of `mo/<n>/coefficient` follow one fixed order for every code.

- Atoms in the order of `nucleus`, shells in the order of `basis`.
- Spherical shells: m = 0, +1, -1, +2, -2, and so on. p shells are x, y, z.
- Cartesian shells: xx, xy, xz, yy, yz, zz (and the same pattern for f).
- Every atomic orbital is normalised to 1. The shell coefficients are for
  unnormalised primitives, so only their ratios matter.

The converters take care of the differences between programs. The
attribute `ao_convention` in `provenance` says which convention the source
file turned out to use.

## Looking at a file

```
h5dump -n results.h5
python3 -c "import h5py; f = h5py.File('results.h5'); print(f['mo/0/coefficient'].shape, f['geometry/coord'][0])"
```

## The ecce-results command

```
ecce-results convert calc.molden results.h5
ecce-results convert calc.fchk results.h5
ecce-results convert calc.hess results.h5
ecce-results export molden results.h5 out.molden
ecce-results show results.h5
ecce-results check results.h5
```

`convert` chooses the reader from the file name: `.molden` (also
`.molden.input`), `.fchk` or `.hess`. A `.hess` file is added to an
existing `results.h5`, so convert the orbitals first. `export molden`
writes the geometry, basis and orbitals as a Molden file that ECCE-QM,
xtb, ORCA tools and most viewers read. `show` prints what the file holds.

`check` tests that the conversion kept the physics and exits with 1 when it
did not:

- the number of electrons from the density matrix equals the occupations,
  tr(P S) = N;
- the orbitals are orthonormal, C-transpose S C = 1, to 1e-6;
- frequencies computed from the stored Hessian (mass weighted, rotations and
  translations projected out) agree with the frequencies the code printed to
  within 0.5 cm-1.

## What is covered

| Code | Stored | From | Status |
|---|---|---|---|
| xtb | geometry, orbitals | Molden file | done |
| ORCA | geometry, orbitals, Hessian, frequencies, modes | Molden file, `.hess` | done |
| Gaussian 16 | geometry, basis, orbitals, Hessian, gradient, charges, dipole, frequencies | formatted checkpoint (`.fchk`) | done |
| ECCE-QM | everything it computes | written directly later | planned |
| MOPAC | geometry, orbitals in an orthonormal Slater basis, charges | output file | planned |
| NWChem | geometry, orbitals, Hessian | Molden output, `.hess` | planned |
| CP2K | geometry, cell, pseudopotentials, orbitals | Molden output | planned |
| Quantum ESPRESSO | cell, k-points, band energies, Fermi energy, cutoffs, pseudopotential names; no wavefunctions | XML output | planned |
| GROMACS | cell, energies, temperature, pressure series; a reference to the trajectory | `.edr`, `.xtc` | planned |

The same rule holds for every code: the file is smaller than the smallest
format it replaces.
