# ECCE-QM input and output files

ECCE-QM is the Hartree-Fock and density functional program that comes with
ECCE, a built-in teaching engine for small molecules. ECCE writes its input
file for you when you launch a job, so you normally never type one. This page
describes the two files, for reading what a job did, for checking a
hand-edited input, and for running the program yourself.

- The input file is `ecceqm.qmin`.
- The output file is `ecceqm.qmout`.

The program is `ecce-qm`, installed in the `bin` folder of the ECCE client:

```
ecce-qm ecceqm.qmin
ecce-qm -o result.qmout --molden result.molden ecceqm.qmin
```

With no `-o`, the output goes to the terminal. `--molden FILE` also writes
the orbitals in Molden format (see below). The exit status is 0 when the SCF
converged, 1 when it did not, and 2 when the input was refused.

## The input file

The input is plain text, one keyword per line. Keywords may be written in
capitals or small letters. Everything after a `#` on a line is a comment.
A keyword the program does not know stops the job at once, and the message
names the line.

### Keywords

| Keyword | Value | Default |
|---|---|---|
| `title` | any text | none |
| `charge` | whole number | 0 |
| `multiplicity` | whole number, 2S+1 | 1 |
| `method` | `hf`, `svwn`, `pbe`, `b3lyp` or `pbe0`, optionally with `r`, `u` or `ro` in front | `hf` |
| `basis` | a basis set name from ECCE's library, such as `6-31G*` | `STO-3G` |
| `spherical` | `true` or `false` | `true` |
| `grid` | two whole numbers: radial points, angular points | `99 590` |
| `grid_prune` | `true` or `false`: thin the angular grid near the nuclei | `true` |
| `units` | `angstrom` or `bohr` (`au` also means bohr) | `angstrom` |
| `charges` | any of `loewdin`, `chelpg`, `mk`: extra atomic charges besides Mulliken (see below) | none |
| `maxiter` | whole number | 150 |
| `runtype` | `energy`, `gradient`, `optimize` (or `opt`), `frequency` (or `freq`), or `optfreq` (optimise, then frequencies at the optimised geometry) | `energy` |
| `maxsteps` | whole number: geometry optimisation steps (energy and gradient evaluations) | 100 |
| `opt_conv` | `normal` or `tight`: optimisation convergence criteria | `normal` |
| `polarizability` | `true` or `false`: also compute the static polarizability tensor at the final geometry, by finite field (six more SCF calculations; see below) | `false` |
| `raman` | `true` or `false`: with `frequency` or `optfreq`, also compute Raman activities and depolarization ratios (see below) | `false` |
| `field_step` | number, in atomic units: electric field of the finite-field polarizability | 0.001 |
| `hessian_step` | number, in bohr: displacement for the finite-difference frequencies | 0.005 |
| `numeric_hessian` | `true` or `false`: closed-shell Hartree-Fock frequencies use the analytic Hessian; `true` forces the finite-difference one (used anyway for DFT, open-shell and Raman) | `false` |
| `temperature` | number, in K: for the thermochemistry | 298.15 |
| `pressure` | number, in atm: for the thermochemistry | 1 |
| `symmetry_number` | whole number: rotational symmetry number for the thermochemistry | found from the geometry |
| `molden` | no value: accepted and ignored; the Molden file is written by the command line option `--molden` | none |
| `parallel_points` | `true` or `false`: with `frequency` or `optfreq`, run the displaced geometries at the same time (the environment variable `ECCE_QM_SERIAL` also runs them one after another) | `true` |
| `conv_energy` | number, in Hartree | `1e-10` |
| `conv_grad` | number | `1e-7` |

Four blocks follow the same rule, each closed by a line `end`:

- `geometry` is followed by one atom per line: the element symbol and the
  three coordinates, in the unit given by `units`. The symbol `X` is a dummy
  atom: it has no nucleus, electrons or basis functions and takes no part in
  the SCF, the integration grid, the charges, the dipole or the vibrations. It
  keeps its number, so the atoms are numbered as in the Builder, and it is
  listed in the geometry, the gradient (zero) and the charges (0). The
  vibration tables list the real atoms only.
- `constraints` holds one constraint per line and applies to `runtype
  optimize`:

  ```
  constraints
    bond 1 2 1.05
    angle 5 1 2 110
    torsion 3 1 2 4 90
    fix 3
  end
  ```

  `bond` takes two atom numbers and a length in the unit given by `units`,
  `angle` three atoms and a value in degrees, `torsion` (or `dihedral`) four atoms and a value
  in degrees, `fix` one atom that stays where it is. Atoms are counted from 1
  in the order of the geometry block, dummy atoms included. The value is
  optional; without it the constraint keeps the value of the starting
  geometry. A value that the starting geometry does not have is imposed before
  the first energy. A dummy atom that no bond, angle or torsion names is fixed
  in space; one that is named moves as the constraints require. The optimiser
  holds every constraint exactly at every step, and the gradient it reports
  as converged is the part not absorbed by the constraints.
  `optfreq` with constraints is refused: frequencies at a constrained
  geometry are not at a minimum. Optimise with the constraints, then run
  `frequency` without them.
- `scan` is a relaxed scan of one internal coordinate and applies to `runtype
  optimize`:

  ```
  scan
    torsion 3 1 2 4 60 180 13
  end
  ```

  The line is `bond`, `angle` or `torsion` with its atom numbers, then the
  first value, the last value and the number of points; both end values are
  points (60 to 180 in 13 points is a step of 10 degrees). Bond values are in
  the unit given by `units`, angles and torsions in degrees. Each point is a
  constrained optimisation that starts from the geometry and orbitals of the
  previous point; the starting geometry is brought to the first value before
  the first energy. Constraints in a `constraints` block stay in force at
  every point. The output has a table of the points (value, energy,
  whether the optimisation converged) and the optimised geometry of each.
- `basis_data` gives the basis set in full: a line `shell <element> <letter>`
  with the letter S, P, D, F, G, H or I, followed by one line
  `exponent coefficient` for each primitive of that shell.

A geometry block is required. Everything else has a default.

### Methods

`method` names the functional. `hf` is Hartree-Fock; the others are density
functionals: `svwn` (Slater exchange with VWN5 correlation, `lda` is the same
thing), `pbe`, `b3lyp` (with the VWN5 form of the correlation, which is what ORCA uses; Gaussian's B3LYP differs slightly and is not offered)
and `pbe0`.

A letter in front chooses the orbitals: `r` restricted (closed shell), `u`
unrestricted, `ro` restricted open shell. Without a letter the program takes
restricted for multiplicity 1 and restricted open shell for anything else. So
`hf` with multiplicity 3 is ROHF, and `ub3lyp` is unrestricted B3LYP.
In the Calculation Editor, **Theory** selects HF or DFT, the functional is in
**Theory Details**, and the tick for unrestricted orbitals puts the `u` in
front.

### Basis set

ECCE always writes the basis set into the input in a `basis_data` block, so a
job does not depend on a library on the machine that runs it, and a basis set
you edited in ECCE is the one that is used. The `basis` line then only names
the set in the output. If you run the program on an input without
`basis_data`, it looks the name up in ECCE's basis set files, where
polarisation sets are separate files: `6-31G*` is `6-31G` plus its `*` set.
Elements that need an effective core potential are not supported.

### Example: water, B3LYP, 6-31G*

This is the input ECCE writes for water with the B3LYP functional and the
6-31G* basis set. The long `basis_data` block is shortened here to the shells
of the hydrogen atoms and the first oxygen shells.

```
title water
charge 0
multiplicity 1
method b3lyp
spherical true

basis 6-31G*
basis_data
shell H S
  18.731137 0.0334946
  2.8253937 0.23472695
  0.6401217 0.81375733
shell H S
  0.1612778 1
shell O S
  5484.6717 0.0018311
  825.23495 0.0139501
  188.04696 0.0684451
  52.9645 0.2327143
  16.89757 0.470193
  5.7996353 0.3585209
  ...
end
geometry
 O     0.0000000000     0.0000000000     0.1173000000
 H     0.0000000000     0.7572000000    -0.4692000000
 H     0.0000000000    -0.7572000000    -0.4692000000
end
```

### Checking an input

The **Verify** button of the Calculation Editor checks this file. It reports
a missing geometry, a method or keyword the program does not have, a shell
line it cannot read, a block without its `end`, and a charge and
multiplicity that cannot both be true for the number of electrons. It cannot
tell whether a basis set from the library exists for every element.

## The output file

The output is plain text. Each line is a keyword and its value, and a few
parts are blocks that start with `begin <name>` and finish with
`end <name>`. The text after a `#` on a `begin` line explains the columns.
The last line is always `end_of_output`; a file without it is a job that did
not finish.

### Summary lines

For water at B3LYP/6-31G*:

```
program ecce-qm 0.1
title water
method b3lyp
reference rks
basis 6-31G*
spherical true
grid 99 590
charge 0
multiplicity 1
nbf 18
nalpha 5
nbeta 5
converged yes
iterations 9
energy_total -76.369668959397
energy_nuclear 9.189533762640
energy_one_electron -123.132992285354
energy_coulomb 46.899273345729
energy_exchange_hf -1.792380208889
energy_xc -7.533103573523
grid_electrons 10.000000025279
s_squared 0.000000000000
dipole_au -0.000000000000 -0.000000000000 -0.815482704756
dipole_debye -0.000000000000 -0.000000000000 -2.072750288606 2.072750288606
wall_seconds 0.32
```

| Line | Meaning |
|---|---|
| `reference` | `rhf`, `uhf` or `rohf`; `rks`, `uks` or `roks` for a density functional |
| `nbf` | number of basis functions, which is also the number of orbitals |
| `nalpha`, `nbeta` | electrons of each spin |
| `converged`, `iterations` | whether the SCF converged, and in how many cycles |
| `energy_total` | the total energy, in Hartree |
| `energy_nuclear` | nuclear repulsion |
| `energy_one_electron` | kinetic energy and attraction to the nuclei |
| `energy_coulomb` | electron-electron Coulomb energy |
| `energy_exchange_hf` | the Hartree-Fock exchange that is part of the energy: the whole exchange for HF, the fraction in a hybrid functional, zero for `svwn` and `pbe` |
| `energy_xc` | the exchange-correlation energy of the functional (zero for HF) |
| `grid_electrons` | the number of electrons found by integrating the density on the grid; it should be very close to the true number, and is a check on the grid (DFT only) |
| `s_squared` | the spin expectation value S^2 |
| `dipole_au`, `dipole_debye` | the dipole moment vector, and in Debye its three components followed by its length |
| `wall_seconds` | time in the SCF, in seconds |

The five energy parts add up to `energy_total`. All energies are in Hartree,
as everywhere in ECCE.

### Atoms and Mulliken charges

```
begin atoms  # index symbol Z x y z (angstrom) mulliken_net_charge
0 O 8 0.000000000000 0.000000000000 0.117300000000 -0.797224003047
1 H 1 0.000000000000 0.757200000000 -0.469200000000 0.398612001524
2 H 1 0.000000000000 -0.757200000000 -0.469200000000 0.398612001524
end atoms
```

One line per atom, counted from 0: element, atomic number, position in
Angstrom, and the Mulliken net charge. The charges add up to the charge of
the molecule.

### Other atomic charges

With the keyword `charges loewdin chelpg mk` (any of the three) the output has
one more block for each, in the same form, with the net charge per atom:

```
begin charges loewdin  # index symbol net_charge
0 O -0.651047134805
1 H 0.325523567403
2 H 0.325523567403
end charges
```

The blocks are `loewdin`, `chelpg` and `mk`. Loewdin charges come from the
orbital populations after symmetric orthogonalisation of the basis. CHELPG and
Merz-Kollman charges are fitted to the electrostatic potential on points around
the molecule, with the sum fixed to the charge of the molecule; the header of
the block gives the number of points. They differ from Mulliken charges, and
from each other, by amounts that depend on the basis set. In the Calculation
Editor the three are tick boxes in **Theory Details**, and the Viewer lists
them under Charges beside Mulliken.

### Polarizability and Raman activities

With `polarizability true`, or `raman true` in a frequency run, the output has
the static dipole polarizability of the final geometry:

```
polarizability_field_step 0.001000000000
polarizability_isotropic 4.694934150974
polarizability_anisotropy 3.418529097586
begin polarizability  # rows xx xy xz / yx yy yz / zx zy zz (a.u., bohr^3)
2.698588703901 0.180182509339 0.004449254052
0.180182509339 6.626745686923 -0.047262469958
0.004449254052 -0.047262469958 4.759468062098
end polarizability
```

The tensor is the change of the dipole moment with a uniform electric field,
taken from the dipole moments at plus and minus the field step along each
axis (central differences, six extra SCF calculations started from the
field-free orbitals). It works with every method and reference. ECCE shows it
as the polarizability ellipsoid. In `raman true` runs the same calculation is
made at every displaced geometry of the frequency calculation, and the
derivatives along the normal modes give the activity S = 45 a'^2 + 7 g'^2 in
angstrom^4/amu and the depolarization ratio 3 g'^2 / (45 a'^2 + 4 g'^2), the
definitions ORCA prints. They are written in a block with a line for each of
the 3N modes, in the order of the `vibrations` block (zero for translations
and rotations):

```
begin raman  # index activity(A^4/amu) depolarization_ratio, rows as in vibrations
...
7 5.613610 0.526162
end raman
```

`raman` is refused for other run types. Within a set of degenerate modes
(methane, ammonia) the split into individual modes is arbitrary, so compare
the sum of the activities, not the single values. Against ORCA 6.1.1 the
polarizability agrees to 1e-3 bohr^3 per component and the activities to
0.1 %.

### Orbital energies and occupations

```
begin orbitals restricted  # index energy(hartree) occupation
1 -19.123747224922 2.000000000000
2 -0.997410433906 2.000000000000
3 -0.518447040204 2.000000000000
4 -0.366490089462 2.000000000000
5 -0.287294487557 2.000000000000
6 0.071506984861 0.000000000000
...
end orbitals
```

One line per orbital, from the lowest energy: its number, its energy in
Hartree and its occupation. With restricted orbitals the occupation is 2, 1
(an unpaired electron of a restricted open-shell calculation) or 0, and there
is one block, named `restricted`. With unrestricted orbitals there are two
blocks, `alpha` and `beta`, each with occupations 1 or 0. The highest
occupied and lowest empty orbitals, here 5 and 6, give the HOMO and the LUMO.

### Orbital coefficients

```
begin ao_labels  # index label, order of the MO coefficients
1 0O 1s
2 0O 2s
...
end ao_labels
begin mo_coefficients restricted  # one line per MO: index then nbf coefficients
1 -0.9950426140 -0.0279429372 0.0132556513 ...
...
end mo_coefficients
```

`ao_labels` names the basis functions: the atom number (from 0), its element,
the shell number and the function (`s`, `px`, `dxy`, ...). Each line of
`mo_coefficients` is one orbital: its number followed by its coefficient on
each of those basis functions, in the order of `ao_labels`. ECCE reads these
to draw the orbitals and the orbital diagram. There is one block for
restricted orbitals and two (`alpha`, `beta`) for unrestricted ones.

### Molden file

`ecce-qm --molden FILE input` writes the orbitals in the Molden format as
well, for programs that draw or analyse them. A job started from ECCE does not
write it. The file has the sections `[Atoms]` (in Angstrom), `[GTO]` (the
basis set of each atom) and `[MO]` (for each orbital its energy `Ene=`, its
spin `Spin=`, its occupation `Occup=` and its coefficients). It uses spherical
d, f and g functions, so it needs `spherical true`. The keyword `molden` is
accepted in an input file but does nothing; use the command line option.

### When a job does not converge

`converged no` and an exit status of 1 mean the SCF reached `maxiter` cycles
without meeting the energy and gradient limits. The orbitals in the file are
those of the last cycle and should not be used. Try another geometry, or a
larger `maxiter`.

### Size and speed

The two-electron integrals are kept in memory, so the size limit is memory:
about 250 basis functions need 4 GB. Integrals that cannot matter are skipped,
and the DFT grid is pruned (fewer angular points close to the nuclei). Pruning
changes a B3LYP energy by less than 1e-8 Hartree against `grid_prune false`; use `false` to compare with the full grid.

Wall-clock times on one machine (8 cores available, 3 used), 6-31G*,
before and after these changes:

| Molecule (basis functions) | Calculation | Before | After |
|---|---|---|---|
| benzene (102) | HF energy | 0.7 s | 0.7 s |
| benzene | B3LYP energy | 14 s | 6 s |
| benzene | B3LYP gradient | 21 s | 10 s |
| benzene | B3LYP geometry optimisation + frequencies | 28 min | 13 min |
| caffeine, 24 atoms (216) | HF energy | 24 s | 19 s |
| caffeine | HF gradient | 91 s | 58 s |
| caffeine | B3LYP energy | 188 s | 54 s |
| caffeine | B3LYP gradient | 280 s | 101 s |
