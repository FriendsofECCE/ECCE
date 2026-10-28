# xTB: tight-binding calculations with xtb

xTB is the code button for the `xtb` program of the Grimme group. It
computes the energy and the optimised geometry of a molecule with a
tight-binding method in a fraction of a second to a few seconds, which makes
it a quick way to pre-optimise a structure before a DFT calculation, or to
look at a molecule of a few hundred atoms.

On Debian and Ubuntu the program is the package `xtb`
(`sudo apt install xtb`). Register it like any other code: in
**Register Machines** the path of the code is `xtb` on a machine where it
is on the PATH, or the full path such as `/usr/bin/xtb`.
[TO CHECK: the field name in Register Machines, and the xTB button in the
code bar on a build with this code.]

## Theory

The calculation's **Theory** is the method:

- **GFN2-xTB**, the default.
- **GFN1-xTB**, the earlier parametrisation.
- **GFN-FF**, a force field of the same family; it gives energies and
  geometries but no orbital gap.

The charge and the multiplicity are those of the calculation, as for every
other code. xtb has no basis set, so the Basis Set Tool is not used.

**Theory Details** has:

- **Implicit Solvent**: None, ALPB or GBSA, and the solvent. The list is
  that of ALPB; GBSA has parameters for fewer solvents, and a deck written
  with a solvent GBSA lacks is refused with a message when the input is
  generated.
- **Electronic temperature** (300 K by default) and **Accuracy** (1 by
  default, lower is tighter). They are written to the input only when
  changed.

## Run types

- **Energy**: a single point.
- **Geometry**: an optimisation. **Runtype Details** has the **Level**:
  crude, sloppy, loose, normal (the default), tight, verytight or extreme.
  An optimisation that reaches xtb's cycle limit without converging ends as
  Incomplete, with that as the reason.
- **Vibration**: the frequencies at the geometry as it is (xtb `--hess`). The
  geometry should already be optimised at the same level, or the frequencies
  are not meaningful and some come out imaginary.
- **Geometry + Vibration**: an optimisation followed by the frequencies
  (xtb `--ohess`), with the same **Level**.

## The input file

The input is a single file. Its first line is the command line for xtb,
beginning with `xtb`, and the rest is the geometry in xyz form:

    xtb --gfn 2 --chrg 0 --uhf 0 --opt normal
    3
    water
    O    0.000000000     0.000000000     0.117300000
    H    0.000000000     0.757200000    -0.469200000
    H    0.000000000    -0.757200000    -0.469200000

`--chrg` is the charge and `--uhf` the number of unpaired electrons, the
multiplicity minus one. Anything else xtb accepts on its command line can be
added to the first line by hand; the input check does not judge the options,
only that the file has this shape and that charge and multiplicity agree with
the number of electrons.

## Results

The Builder shows the total energy, the HOMO-LUMO gap and the dipole moment,
and for an optimisation the geometry at every cycle, with the energy of each
cycle in the Geometry Trace plot. The Progress box follows the optimisation
cycles and their gradient norm against the convergence limit.

### Charges

The atomic charges are read from the `charges` file xtb writes and appear
under **Mulliken Charges**. For GFN2-xTB and GFN1-xTB they are xtb's
Mulliken-type charges; for GFN-FF they are the charges of its own
electronegativity-equalisation model.

### Frequencies

A Vibration or Geometry + Vibration run adds the frequencies, the IR
intensities and the normal modes (animated in the Builder), and the
thermochemistry at 298.15 K: zero-point energy, enthalpy, Gibbs energy and
T*S, all in Hartree. The job script adds xtb's `vibspectrum` and `g98.out`
files to the output file (between lines beginning with `ECCE-XTB-VIBSPECTRUM`
and `ECCE-XTB-G98`). xtb prints the normal modes to two decimals, so the
animation is a little less exact than the frequencies. xtb prints no internal
energy, so that value is not shown. The five or six translations and
rotations are listed first at zero, as for the other codes.

### Orbitals

GFN2-xTB and GFN1-xTB are run with `--molden`, and the job script adds xtb's
Molden file to the end of the output file (between lines beginning with
`ECCE-XTB-MOLDEN`). ECCE reads the orbital energies, occupations and
coefficients from it, together with the basis xtb uses: each Slater function
is expanded in Gaussians (STO-nG), so the basis is not orthonormal, which is
what the fragment MO diagram needs. The p functions are x, y, z; the d
functions are Cartesian. A GFN-FF run has no orbitals and stores none.
Open-shell runs have one set of orbitals with fractional occupations, as xtb
writes them.

xtb itself prints no coordinates, so the job script adds the structures to
the end of the output file, between lines beginning with `ECCE-XTB-FRAME`.
