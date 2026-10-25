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
  know where it is: choose **Tools > Register Machines...**, select
  `localhost`, and on the **Codes** tab select GROMACS and press **Find**,
  then save (see [Machines](machines.md)). A job fails with "Path for GROMACS
  not found" without it.
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
3. Add the five tasks (the File menu holds the same entries). Each time, select the study first, then choose:
   1. **File > New > GROMACS MD Prepare...**
   2. **File > New > GROMACS MD Optimize...**
   3. **File > New > GROMACS MD Equilibrate...** (constant volume)
   4. **File > New > GROMACS MD Equilibrate...** again (constant pressure)
   5. **File > New > GROMACS MD Dynamics...**

Each task finds the topology and the structure of the one before it by
itself. Nothing is attached by hand.

## 2. Prepare

1. Select **Prepare** and click the **GROMACS MD Prepare Editor** button below
   its details in the Organizer.
2. Under **Structure**, choose **Source:** `Protein (pdb2gmx)`, and
   **Structure:** `PDB file...`. A file dialog opens. Choose `1l2y.pdb`:
   ECCE ships it in its examples folder, on Linux
   `/opt/ecce/data/client/examples/computational/gromacs/gromacs-trpcage/1-prepare/1l2y.pdb`
   (in the file dialog, type Ctrl+L and paste the path). A file downloaded
   from the Protein Data Bank works as well. The name of the file's copy,
   `1l2y.gmxpdb`, is shown under **Browse...**.
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

1. Select **Optimize** and click **GROMACS MD Optimize Editor**.
2. The **Inputs** page shows the topology and the starting structure, each
   marked `(from Prepare)`, and the status `Ready: a topology and a starting
   structure are attached.`
3. On the **Optimize** page, set the number of steps to `200`. The method
   is steepest descent.
4. Click **Launch...**, then **Launch**.

It completes in a few seconds. Open the task in the Builder (**Tools >
Builder**) to see the minimised structure, and the energy with the
**Properties** panel. In the run of this tutorial the potential energy falls
from about -19 000 to -86 000 kJ/mol in 200 steps, as the water relaxes
around the protein.

## 4. Equilibrate

1. Select the first **Equilibrate** and click **GROMACS MD Dynamics Editor**.
   The **Inputs** page now takes the structure from Optimize.
2. On the **Dynamics** page, set **Data Collection Steps:** to `1000`. The
   **Time Step** is `0.002` ps, so this is 2 ps. The **Temperature** tab
   holds the 300 K the system is brought to.
3. Click **Launch...**, then **Launch**.

When it is done, the temperature of the task is close to 300 K (303 K at
its last step in the run of this tutorial).

## 5. Equilibrate at constant pressure

The run above holds the temperature only, so the box keeps its volume. A
second Equilibrate lets the density settle at 1 bar before the production
run.

1. Select the second **Equilibrate** and click **GROMACS MD Dynamics Editor**.
   Tick **Continue from the previous task's
   structure and velocities**, so that it carries on from the first
   Equilibrate instead of drawing new velocities.
2. On the **Dynamics** page, set **Data Collection Steps:** to `1000`.
3. On the **Pressure** tab, tick **Use Constant Pressure Algorithm** and set
   **Pressure:** to `100000` N/m**2 (1 bar).
4. On the **Run settings** page, choose the **Barostat:** under **Pressure
   coupling**: C-rescale, the default, or Parrinello-Rahman.
5. Click **Launch...**, then **Launch**.

## 6. Dynamics

1. Select the last task, **Dynamics**, open its editor in the same way (**GROMACS MD Dynamics Editor**) and
   set **Data Collection Steps:** to `2000` (4 ps).
2. The box **Continue from the previous task's structure and velocities**
   is ticked, because the task before it is an Equilibrate: the run starts
   from the structure and the velocities the equilibration ended with.
   Untick it to draw new velocities at 300 K instead.
3. Click **Launch...**, then **Launch**.

This is the longest task, about 15 seconds.

## 7. The results

- Select a task and click **Builder**: it shows the
  structure the task ended with. The status line counts the atoms of the
  whole system, water included.
- In the **Properties** panel of the Builder, choose **Geometry Step Plots**
  and pick what to plot on the Y-axis, for example **Pressure Vector**. The
  tasks store the total energy, the pressure and, after Equilibrate and
  Dynamics, the temperature.

The energies a task stores are the ones `gmx energy` reads from the run.

## Example

The files of this calculation are in `data/client/examples/computational/gromacs/gromacs-trpcage`
in the ECCE installation: the PDB file, the Prepare recipe, the three
`.mdp` files and the energies of each run. They are a reference to check
your own settings against. To open it in the Organizer, choose
**File > Open Example Calculations** and the **Computational** set.

## What the runs are not

Steps of 2 ps and 4 ps are not a simulation of a protein: the Trp-cage folds
in tens of nanoseconds, a million times as long. Use these settings to
learn the workflow, then raise the number of steps, and the **Dynamics**
page's other tabs, for a real run.
