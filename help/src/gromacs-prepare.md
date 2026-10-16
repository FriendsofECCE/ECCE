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

- **Source:** *Protein (pdb2gmx)*. This is the only source for now.
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

- The system is always neutralised.
- **Salt concentration:** added on top, in mol/L. The default is 0.15. Give
  0 to neutralise only.
- **Positive ion:** sodium or potassium. **Negative ion:** chloride.

Save with the **Save** button, or let Launch save for you.

## What the job does

Launch runs four steps, and the output file tells which one is running
(*step 1/4* and so on):

1. `pdb2gmx` builds the topology with the force field and water model.
2. `editconf` puts the protein in a box of the chosen type and size.
3. `solvate` fills the box with water of the chosen model.
4. `genion` replaces water molecules with ions to neutralise the system and
   to reach the salt concentration.

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

- Proteins only. Ligands and force fields built by ECCE are not offered yet.
- A single amino acid on its own (glycine, for instance) cannot be prepared:
  pdb2gmx needs a chain.
- GROMOS force fields make `grompp` print one warning about the parametrisation;
  ECCE allows it.
