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
| `units` | `angstrom` or `bohr` (`au` also means bohr) | `angstrom` |
| `maxiter` | whole number | 150 |
| `conv_energy` | number, in Hartree | `1e-10` |
| `conv_grad` | number | `1e-7` |

Two blocks follow the same rule, each closed by a line `end`:

- `geometry` is followed by one atom per line: the element symbol and the
  three coordinates, in the unit given by `units`.
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
