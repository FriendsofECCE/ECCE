# Preparing a protein for GROMACS

A GROMACS MD study starts with a **GROMACS MD Prepare** task. It turns a
protein structure into what the later tasks need: a topology, a structure in a
box of water, and restraints. The tasks after it find those files on their
own, so nothing has to be attached to them.

## Make the task

1. In the Organizer, select a GROMACS MD study and choose **New >
   GROMACS MD Prepare**. It is the first task of the study.
2. Choose **GROMACS MD Prepare Editor** in the Tools menu.

If the structure is to come from the Builder, draw or import it in the Builder
first. A protein needs hydrogens only if you clear **Ignore hydrogens**.

## The editor

**Structure**

- **Source:** *Protein (pdb2gmx)* or *Mineral (MINFF)*. Choosing the mineral
  hides the settings that belong to a protein (structure, force field,
  histidines, box type); see "A mineral" below.
- **Structure:** *from the Builder* uses the structure of the task, as it is
  when you launch. *PDB file...* asks for a file and stores a copy in the
  task, so later changes to the file do not matter. **Browse...** chooses
  another one.

**Force field**

- **Force field:** the force fields that come with GROMACS. *Other (GMXLIB)*
  takes the name of a force field folder (without `.ff`) that GROMACS finds
  in its library or in the folders named by `GMXLIB`.
- **Water model:** only the water models the chosen force field offers are
  listed. AMBER force fields start with TIP3P, CHARMM27 with TIP3P, OPLS-AA
  keeps TIP3P if it is chosen and otherwise offers TIP4P first, and the GROMOS
  force fields offer SPC and SPC/E.
- **Histidines:** automatic. A histidine already named HID, HIE or HIP in the
  PDB file keeps that name; otherwise pdb2gmx chooses.
- **Ignore hydrogens in the PDB** (on by default): pdb2gmx builds the
  hydrogens itself, with the names its force field uses.
- **Merge chains into one molecule** (on by default).

**Box and water**

- **Box type:** rhombic dodecahedron (the default, with the fewest water
  molecules for a given distance), cubic, truncated octahedron or triclinic.
- **Distance to edge:** the least distance between the protein and the box
  edge, in nm. The default is 1.0 nm.

**Ions**

- **Neutralise with counter-ions:** ticked by default, so the system ends with
  net charge zero. Untick it to keep the net charge of the structure, for
  example a single ion in water; the tasks after the Prepare then do not stop
  at GROMACS's warning about a charged system.
- **Salt concentration:** added on top, in mol/L. The default is 0.15. Give
  0 to neutralise only (or, with Neutralise unticked, to add no ions).
- **Positive ion:** sodium or potassium. **Negative ion:** chloride.

Save with the **Save** button, or let Launch save for you.

## A mineral

*Mineral (MINFF)* prepares a clay or other mineral with the MINFF force field.
The topology is built on your computer when you press **Build topology**, and
is stored in the task; Launch then only adds water and ions.

- **Unit cell:** a `.pdb`, `.gro` or `.cif` file of the mineral's unit cell,
  with its cell. **Repeat (a b c)** repeats it along the cell axes, for
  example `4 3 3`. A layer or a stack drawn in the Builder, saved as a PDB
  file with its cell, works too.
- **Substitutions:** atoms replaced at random, as `FROM,TO,COUNT` separated by
  semicolons. `Al,Mgo,4` puts four Mgo in place of Al, which gives the layer a
  charge of -4. **Random seed** makes the draw repeatable.
- **Parameters:** the general set (GMINFF), or the set tailored to one mineral
  (TMINFF), whose name you give, for example `Montmorillonite`. **O-M-O angle
  constant** is the force constant of the angles in the mineral, in
  kJ/mol/rad2.
- **Water model:** the water models of MINFF. The ions in the topology are the
  ones that match it.
- **Build topology** runs the build, and says how many atoms the mineral has,
  its charge, and whether some atoms found no MINFF type. Press it again after
  changing any of the settings above; the stored files are the ones of the
  last build.

The mineral keeps its cell. **Water layer above the mineral** (0 by default)
makes the cell longer along its third axis (c) by that many nm and fills the
new space with water, so the mineral should be one block along c, which the
build arranges. With no water layer, no water is added; a charged mineral then
keeps its charge and the later tasks need grompp's warning about a net charge
to be allowed.

**Ions:** with a water layer, the charge of the mineral is neutralised by
counter-ions chosen from MINFF's ions (sodium, potassium, calcium, copper(II)
and others); the salt concentration is added on top, 0 by default.

The job has the same four steps as for a protein: the first reads the stored
mineral instead of running pdb2gmx. The tasks after it run with periodic
molecules switched on, because the mineral is bonded across the cell.

## What the job does

Launch runs four steps, and the output file tells which one is running
(*step 1/4* and so on):

1. `pdb2gmx` builds the topology with the force field and water model.
2. `editconf` puts the protein in a box of the chosen type and size.
3. `solvate` fills the box with water of the chosen model.
4. `genion` replaces water molecules with ions to neutralise the system (if
   ticked) and to reach the salt concentration.

A task that stops says why, in the words of the GROMACS tool that stopped.
The most common reasons are atoms missing from a residue in the PDB file and a
residue name the force field does not have.

When the task is complete, its results are the topology, the structure in the
box, the position restraints and `chemsys.pdb`.

## What the results show

**Progress.** While the job runs, the Progress box of the Organizer shows a
*Prepare* stage with the step under way (*step 3 of 4 · solvate*) and a bar
that fills as the steps finish. The steps are named *pdb2gmx*, *box*,
*solvate* and *ions*. When the task has finished the box keeps the stage. A
task that stopped says at which step.

**Summary.** A complete task lists, under the Progress box, the number of atoms,
protein residues and water molecules, how many positive and negative ions were
added, the net charge before and after the ions, and the type and size of the
box. The numbers are taken from the files and messages of the GROMACS tools, not
counted again by ECCE. The same values are properties of the task: in the Viewer
they are listed under **Run Statistics** (number of atoms, protein residues,
water molecules, ions added, net charge before and after, box lengths and
angles).

**The box in the Viewer.** Open the finished task in the Viewer to see the
solvated protein. Choose **Tools > Periodic Builder** to draw the cell around
it; its lengths and angles are listed there. A rhombic dodecahedron or a
truncated octahedron is a slanted cell, so its angles are not 90 degrees: the
dodecahedron made here has angles of 60, 60 and 90 degrees.

## The tasks after it

Optimize, Equilibrate and Dynamics tasks added after a Prepare in the same
study start from its structure and topology, and use its position restraints.
Nothing has to be attached to them. To run a later task from a different
structure, attach that structure to the task as usual.

## Limits

- Proteins and minerals only. Ligands and force fields built by ECCE are not
  offered yet.
- A single amino acid on its own (glycine, for instance) cannot be prepared:
  pdb2gmx needs a chain.
- GROMOS force fields make `grompp` print one warning about the parametrisation;
  ECCE allows it.
