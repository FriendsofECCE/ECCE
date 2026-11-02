# xTB: tight-binding calculations with xtb

xTB is the code button for the `xtb` program of the Grimme group. It
computes the energy and the optimised geometry of a molecule with a
tight-binding method in a fraction of a second to a few seconds, which makes
it a quick way to pre-optimise a structure before a DFT calculation, or to
look at a molecule of a few hundred atoms.

On Debian and Ubuntu the program is the package `xtb`
(`sudo apt install xtb`; Debian 13 has version 6.7.1). Register it like
any other code: on the **Codes** tab of the machine in **Register
Machines**, select **xTB** and set **Program** to the full path of
the program, such as `/usr/bin/xtb`; **Find** looks for it. The xTB button
in the code bar then starts a calculation with this code.

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
- **Molecular Dynamics**: a dynamics run (xtb `--md`) from the structure of
  the calculation. **Runtype Details** has:
    - **Temperature** (K, 300 by default): the thermostat temperature.
    - **Time** (ps, 1 by default): the length of the run.
    - **Time step** (fs, 1 by default): the integration step. xtb advises
      about 2 fs with SHAKE and 1 fs without.
    - **Dump interval** (fs, 20 by default): how often a structure is
      stored. It may not be shorter than the step, and the run may not be
      shorter than one interval.
    - **SHAKE**: off, X-H bonds, or all bonds. SHAKE holds those bond
      lengths fixed so that a longer step can be used. xtb 6.7.1 crashes when
      SHAKE is combined with GFN-FF, so that choice is disabled for GFN-FF and
      a deck that asks for it is refused when the input is generated.
  The method and the implicit solvent are chosen as for the other run types.
- **Conformer search (simple)**: a basic conformer search for a flexible
  molecule that needs only xtb, not CREST. It rotates the rotatable single
  bonds (not in a ring, with a heavy atom with further heavy-atom neighbours
  on each side, and not double, partial-double or linear bonds) to random
  angles, discards starts with atoms on top of each other, optimises each
  start with xtb, and keeps the distinct minima. The structure of the
  calculation is always one of the starts. **Runtype Details** has:
    - **Random samples** (30 by default): the number of start structures.
      A molecule with more rotatable bonds needs more.
    - **Energy window** (kcal/mol, 6 by default): minima higher than this
      above the lowest are dropped.
    - **Heavy-atom RMSD threshold** (A, 0.25 by default): two minima closer
      than this, after superposition of the heavy atoms, count as the same
      conformer. Mirror images are different conformers, so the two gauche
      forms of butane are both listed.
    - **Random seed** (682 by default): the same seed with the same input
      gives the same starts; it is printed in the output.

  GFN-FF is the quick choice for a first look; GFN2-xTB gives better
  relative energies. The ensemble is stored like a CREST ensemble: the
  conformers are the geometry trace in the Builder, ordered by relative
  energy (kcal/mol, within this one method), and the workflow steps that take
  the lowest conformers or one conformer per coordination pattern work on
  it unchanged. The step runs `python3` on the machine that runs the
  job, and uses the processors of the job to optimise several starts at
  once. This search is much simpler than CREST's (no metadynamics, no
  topology check beyond bonds being unchanged), so for a large or very
  flexible molecule use CREST.

- **Docking (aISS)**: docks a ligand onto a receptor with xtb's docking
  module (`xtb dock`, the aISS algorithm) and lists the poses it finds. The
  method is GFN2-xTB or GFN-FF. **Runtype Details** has:
    - **Ligand atoms**: the atom numbers of the ligand, such as `1-5,9`. Left
      empty, the smallest separate molecule is the ligand, which is right for
      a guest on a host such as K+ with 18-crown-6.
    - **Ligand charge** (0 by default): the receptor gets the rest of the
      calculation's charge.
    - **Poses to optimise** (4 by default) and **Fast search**, which is
      suitable for small molecules.

  The poses are the geometry trace in the Builder, best first, with the total
  energy of each. The **Interaction energy (kcal/mol, monomers not relaxed)**
  of each pose is stored beside it; as xtb itself says, the monomers are not
  optimised, so these energies are for ranking the poses and are not physical
  interaction energies. A workflow step can take the best N poses.

  Docking needs an xtb newer than the 6.7.1 release: `xtb dock` of 6.7.1,
  which is the version in Debian 13, crashes after the search. Build or
  download a current xtb and register that program for the machine that runs
  the job (**Register Machines**, **Codes** tab, **xTB**, **Program**), as
  described above.

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

A dynamics run has its settings in a block of lines below the first one,
between `$md` and `$end`, which the job script hands to xtb as its control file:

    xtb --gfn 2 --chrg 0 --uhf 0 --md --input xtb.md
    $md
      temp=300
      time=1
      step=1
      dump=20
      shake=0
    $end
    3
    water
    ...

## Results

The Builder shows the total energy, the HOMO-LUMO gap and the dipole moment,
and for an optimisation the geometry at every cycle, with the energy of each
cycle in the Geometry Trace plot. The Progress box follows the optimisation
cycles and their gradient norm against the convergence limit.

### Molecular dynamics

The trajectory (`xtb.trj`) comes back as the frames of the Geometry Trace,
the first being the starting structure, one every dump interval. The plots
are against the simulation time in ps and show the potential energy xtb
records with each frame. xtb 6.7.1 prints no temperature per frame, so there
is no temperature plot. The Progress box shows the line xtb prints at the
start of the run and, when the run ends, that it has finished; xtb prints
nothing else while it runs.

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

## Importing a run made outside ECCE

The output of an `xtb` run can be imported with **File > Import Calculation
from Output File** in the Organizer (see
[Importing a calculation](importing-a-calculation.md)). The output itself
holds no coordinates, so ECCE reads the geometry from the files xtb wrote
beside it: `xtbopt.log` for an optimisation, otherwise the coordinate file
the run was given. Orbitals (`molden.input`, from a run with `--molden`),
atomic charges (`charges`) and for a frequency run the modes (`vibspectrum`
and `g98.out`) are read from the same folder, so import the output from the
folder the run was made in. Files left by another molecule are ignored.
Theory (GFN2-xTB, GFN1-xTB or GFN-FF), run type, charge, multiplicity and
an implicit solvent are read from the command line xtb prints. Molecular
dynamics, scans and path runs are not imported.
