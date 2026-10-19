# Your first GROMACS calculation

This tutorial takes a small protein, the Trp-cage (Protein Data Bank entry
1L2Y, 20 residues), through the four tasks of a GROMACS study:

1. **Prepare**: put the protein in a box of water with ions;
2. **Optimize**: energy minimisation, to remove bad contacts;
3. **Equilibrate**: a short run that brings the system to 300 K;
4. **Dynamics**: the production run.

The runs are very short, 200 to 2000 steps, so the whole tutorial takes
a few minutes on a laptop, the preparation included. They show how the tasks
fit together; they are far too short to tell you anything about the protein.
Read [Your first calculation](first-calculation.md) first if you have not
used the Organizer before.

## What you need

- ECCE installed and started with `ecce` (see [Installation](installation.md)).
- GROMACS. On Debian:

  ```
  sudo apt install gromacs
  ```

  Check that `gmx` is found with `which gmx`. The machine `localhost` must
  know where it is: the file `~/.ECCE/CONFIG.localhost` has a line
  `GROMACS: /usr/bin/gmx` (see [Machines](machines.md)).
- The protein as a PDB file. 1L2Y can be downloaded from the Protein Data Bank
  (`https://files.rcsb.org/download/1L2Y.pdb`). The model with the lowest
  number is the one to keep if the file holds several.

[Preparing a protein for GROMACS](gromacs-prepare.md) describes every setting
of the Prepare, and [GROMACS MD settings](gromacs-md-settings.md) those of the
tasks after it.

## 1. Create the study

1. In the Organizer, select your home folder and choose **File > New >
   Project...**. Enter `gromacs-tutorial` and click **OK**.
2. Select the project and choose **File > New > GROMACS MD Study...**.
   Enter `trpcage` and click **OK**.
3. Select the study and choose **File > New > GROMACS MD Prepare...**. Make
   in the same way a **GROMACS MD Optimize**, a **GROMACS MD Equilibrate**
   and a **GROMACS MD Dynamics**, in that order. The study shows them as
   Prepare, Optimize, Equilibrate and Dynamics.

Each task finds the topology and the structure of the one before it by
itself. Nothing is attached by hand.

## 2. Prepare

1. Select **Prepare** and choose **Tools > GROMACS MD Prepare Editor**.
2. Under **Structure**, choose **Source:** `Protein (pdb2gmx)`, and
   **Structure:** `PDB file...`. A file dialog opens: choose your 1L2Y file.
   The name of the file's copy, `1l2y.gmxpdb`, is shown under **Browse...**.
3. Leave the other settings at their defaults: **Force field:**
   `AMBER99SB-ILDN`, **Water model:** `TIP3P`, **Box type:** `Rhombic
   dodecahedron`, **Distance to edge:** `1` nm, **Neutralise with
   counter-ions** ticked, **Salt concentration:** `0.15` mol/L, sodium and
   chloride.
4. Click **Verify...** to read the commands the job will run: `pdb2gmx`,
   `editconf`, `solvate` and `genion`.
5. Click **Launch...**, then **Launch** in the Launcher window that opens.
   **Machine:** is `localhost`.

The task turns to **completed** after a short while. The protein
(304 atoms) is now in a box of 5913 atoms in all, with one chloride ion that
makes up for the net charge of the protein, and salt.

## 3. Optimize

1. Select **Optimize** and choose **Tools > GROMACS MD Optimize Editor**.
2. The **Inputs** page says `topology: ecceSession.gmxtop (from Prepare)`,
   `structure: ecceSession_ecceMdPrepare.gro (from Prepare)` and `Ready`.
3. On the **Optimize** page, set the number of steps to `200`. The method
   is steepest descent.
4. Click **Launch...**, then **Launch**.

It completes in a few seconds. Open the task in the Builder (**Tools >
Builder**) to see the minimised structure, and the energy with the
**Properties** panel. In the run of this tutorial the potential energy falls
from about -19 000 to -86 000 kJ/mol in 200 steps, as the water relaxes
around the protein.

## 4. Equilibrate

1. Select **Equilibrate** and choose **Tools > GROMACS MD Equilibrate
   Editor**. The **Inputs** page now takes the structure from Optimize.
2. On the **Dynamics** page, set **Data Collection Steps:** to `1000`. The
   **Time Step** is `0.002` ps, so this is 2 ps. The **Temperature** tab
   holds the 300 K the system is brought to.
3. Click **Launch...**, then **Launch**.

When it is done, the temperature of the task is close to 300 K (303 K at
its last step in the run of this tutorial).

## 5. Dynamics

1. Select **Dynamics**, open its editor in the same way and set **Data
   Collection Steps:** to `2000` (4 ps).
2. The box **Continue from the previous task's structure and velocities**
   is not ticked: the run then starts from the structure of Equilibrate and
   draws new velocities at 300 K. Tick it to keep the velocities.
3. Click **Launch...**, then **Launch**.

This is the longest task, about 15 seconds.

## 6. The results

- Select a task and open the Builder (**Tools > Builder**): it shows the
  structure the task ended with. The status line counts the atoms of the
  whole system, water included.
- In the **Properties** panel of the Builder, choose **Geometry Step Plots**
  and pick what to plot on the Y-axis, for example **Pressure Vector**. The
  tasks store the total energy, the pressure and, after Equilibrate and
  Dynamics, the temperature.

The energies a task stores are the ones `gmx energy` reads from the run.

## Example

The files of this calculation are in `data/client/examples/gromacs-trpcage`
in the ECCE installation: the PDB file, the Prepare recipe, the three
`.mdp` files and the energies of each run. They are a reference to check
your own settings against.

## What the runs are not

Steps of 2 ps and 4 ps are not a simulation of a protein: the Trp-cage folds
in tens of nanoseconds, a million times as long. Use these settings to
learn the workflow, then raise the number of steps, and the **Dynamics**
page's other tabs, for a real run.
