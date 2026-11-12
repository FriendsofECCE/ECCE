# Umbrella sampling with GROMACS

Umbrella sampling gives the free energy (the potential of mean force, PMF)
along a distance between two groups, for example between two protein chains
or between two ions. The procedure has three steps, all done in ECCE:

1. **Pull run.** A dynamics task pulls group 2 away from group 1 at a steady
   rate. Its trajectory passes through every distance you want to sample.
2. **Windows.** Frames of the pull trajectory are taken at a chosen spacing of
   the distance (0.1 nm by default). Each frame starts a window: a short
   equilibration and a production run, with the two groups held at the
   window's distance (0.4, 0.6, ... nm) by a harmonic spring. The spring's
   centre stays there for the whole window, also on a barrier, where the
   system would rather slide into the well beside it.
3. **WHAM.** `gmx wham` combines the windows' histograms into the PMF. ECCE
   plots the PMF and the histograms, and warns where neighbouring windows
   barely overlap.

## Before the pull run

Prepare and equilibrate the system as for any GROMACS study (see [Preparing a
protein for GROMACS](gromacs-prepare.md) and [Your first GROMACS
calculation](gromacs-first-calculation.md)). For pulling one protein chain
away from another:

- Untick **Merge chains into one molecule** in the Prepare, so that each chain is a molecule of
  its own with its own position restraints.
- Leave room in the box along the direction of the pull. The distance GROMACS
  measures is the shortest one through the periodic box, so the pull stalls
  at half the box length. In the Prepare, give the box a **Box size (x y z)**
  in nm, long along the pull, and a **Solute centre (x y z)** near one end
  of it, so that the other end is free: for example a 6.6 × 4.4 × 12 nm box
  with the solute centred at z = 2.4 nm. The editor warns when the solute
  reaches outside the box.
- Define a group for each chain under **Groups by residue name** on the run
  settings page: `Chain_A = mol:Protein_chain_A`,
  `Chain_B = mol:Protein_chain_B`.

## 1. The pull run

Add a **GROMACS MD Dynamics** task after the equilibration and open its
editor. On the run settings page:

- **Centre-of-mass pulling**: tick **Pull group 2 away from group 1** and
  enter the two group names (group 1 is the reference, group 2 is pulled).
- **Pull coordinate**: `distance` follows the line between the two centres
  of mass; `direction along axis` pulls along x, y or z only.
- **Rate** (nm/ps) and **force constant** k (kJ/mol/nm²). A slow pull, about
  0.01 nm/ps, keeps the system close to equilibrium; 0.1 nm/ps drags a
  protein chain through the water much faster than it can follow.
- To keep the reference group in place, restrain it: tick **Restrain chain B**
  (this adds `-DPOSRES_B` to the defines).
- Tick **Write trajectory (.xtc)** on the Files page and choose an interval small enough to give
  frames every few hundredths of a nanometre of the pull.

When the run is complete, its **Properties** list has **Pull Force**: the
pull force and the centre-of-mass distance against time, with **Export
.csv...**. **RMSD...** and **Radius of gyration...** open **MD Analysis** on
the same run, to check whether a chain unfolded or stayed compact while it
was pulled (the run needs a stored trajectory).

## 2. The windows

On the **Umbrella sampling** page of the Pull Force panel (the **Pull run** page has the plots above):

1. Enter the **Spacing** (nm). 0.1 nm suits a force constant of
   1000 kJ/mol/nm²: a window then spreads about 0.05 nm either side of its
   distance at 310 K, so windows 0.2 nm apart barely overlap and WHAM gets
   gaps. An ion pair needs about 0.05 nm. Leave
   **From** and **To** blank to cover the whole pull, or limit the range.
2. Click **Find windows**. The list shows one row per window: the frame of
   the trajectory, its time and its distance. A distance the pull passed
   too quickly to have a frame nearby is left out.
3. Enter the **Equilibration steps** and **Production steps** per window and
   click **Create window calculations**. ECCE adds two tasks per window to
   the study, named after the pull run: `<pull>-w01-eq` (equilibration,
   starting from the frame) and `<pull>-w01` (production, continuing from
   it). They keep every setting of the pull run, with the pull rate set to
   0: each window is held at the distance its frame started with.
4. In the Organizer, select the window tasks and choose **Run Mgmt > Launch
   Selected**. Windows are independent of each other and can run at the same
   time, on any machine.

## 3. The PMF

When the production windows are complete, click **Run WHAM** on the same
panel. ECCE runs `gmx wham` on each window's run input (`topol.tpr`) and pull
force output (`pullf.xvg`) and shows:

- the **PMF** in kJ/mol against the distance in nm, set to zero at the
  largest distance (the separated groups). The binding free energy is the
  depth of the minimum below that plateau, if the PMF has levelled off;
- the **histograms**, one per window. Neighbouring histograms must overlap.
  Where two barely do (less than 5 % shared), a warning names the distance:
  add a window there (a smaller spacing over that range, or a window cut from
  a frame near that distance) and run WHAM again;
- a summary: the deepest minimum, the other minima and maxima that differ
  from their neighbouring extremum by more than the **Noise (kJ/mol)** field
  (empty: kT at the run temperature, 2.6 kJ/mol at 310 K), each maximum's
  barrier above the deepest minimum, and the highest minus the lowest PMF
  value. Smaller wiggles are statistical noise and are not listed.

**Details...** opens the full text in a window of its own, which can be
resized and from which text can be selected and copied: every minimum and
maximum found (significant ones marked `*`), the range shown, the overlap of
each pair of neighbouring windows in per cent, the warnings, the window
calculations used and the `gmx wham` command. It has its own **Export
.csv...**. The picture of the page (the PMF, the histograms and the short
summary) leaves these details out.

The PMF is shown only where the windows sampled: at the two ends, bins in
which the summed histograms hold less than 1 % of the fullest bin are cut,
since `gmx wham` extrapolates there and the profile shoots up.

**Export .csv...** writes the whole PMF, including the cut ends (column
`supported` is 0 there), every minimum and maximum found (`extremum`, with
`significant` 1 for those in the summary) and the histograms.

**Remove the 2RT ln r volume term.** Along a distance between two centres of
mass, more space is available at larger distances (a shell of radius r has
an area proportional to r²). `gmx wham` leaves this term in its profile.
Ticking the box adds 2RT ln r, which gives the PMF of the pair itself: for
an ion pair this is the curve usually published, flat at large distance.
Leave it unticked to get the numbers `gmx wham` alone gives.

## An example: a sodium chloride ion pair

The test system of this procedure is Na⁺ and Cl⁻ in a 3.2 nm box of TIP3P
water (AMBER99SB-ILDN ions), held at contact during the NPT equilibration and
pulled at 0.01 nm/ps from contact to 1.3 nm. Windows every 0.04 nm from 0.24
to 1.2 nm (24 windows, k = 6000 kJ/mol/nm², 5 ps equilibration and 100 ps
production each) overlap their neighbours everywhere. With the 2RT ln r term
removed and the PMF zero at large distance, it shows the contact ion pair at
0.27 nm (about −7 kJ/mol), the barrier at 0.37 nm (about +8 kJ/mol) and the
solvent-separated pair at 0.52 nm (about 0 kJ/mol): the positions known from
simulations of NaCl in water. Windows of 100 ps leave the curve noisy by about
1 kJ/mol.

## Worked example: two strands of an amyloid fibril (2BEG)

PDB entry 2BEG is a fibril of five Aβ(17–42) chains (A–E). The free energy of
taking chain A off the end of the fibril is found as follows; every step is
done in ECCE.

1. **New GROMACS MD Study**, then add a Prepare, an Optimize, an Equilibrate
   and a Dynamics task.
2. **Prepare**: Source Protein, the first model of 2BEG as the structure,
   force field `gromos53a6`, water `spc`, **Ignore hydrogens** ticked,
   **Merge chains into one molecule** unticked. **Box size (x y z)**
   `6.560 4.362 12` and **Solute centre (x y z)** `3.280 2.181 2.4775`, so the
   fibril sits near one end of a box that is long in z. **Neutralise** ticked
   and a salt concentration of 0.1 mol/L (NaCl). Launch it.
3. **Optimize**: steepest descent, 50000 steps at most, with
   `emtol = 1000` under the extra lines. Launch.
4. **Equilibrate**: constant pressure (NPT) at 310 K, every chain restrained
   (**Restrain chain A** to **E**). The lab protocol runs 0.5 ns; 100 ps is
   enough for the box to settle.
5. **Dynamics (the pull run)**: NPT at 310 K, **Restrain chain B** only.
   Groups `Chain_A = mol:Protein_chain_A` and
   `Chain_B = mol:Protein_chain_B`; tick **Pull group 2 away from group 1**
   with group 1 `Chain_B` (the restrained reference) and group 2 `Chain_A`;
   coordinate `distance`,
   rate 0.01 nm/ps, k = 1000 kJ/mol/nm². Add the extra lines
   `pull-coord1-dim = N N Y` (along z only) and `nstxout-compressed = 2500`
   (a frame every 5 ps, 0.05 nm of pull). 200000 steps (400 ps) take chain A
   from 0.45 nm to about 5 nm; a longer pull stops when the distance reaches
   half the box length.
6. When the pull run is Complete, open **Pull Force** in its Properties.
   Check the force and distance plots, and **RMSD...** and **Radius of
   gyration...** for the chains.
7. On the **Umbrella sampling** page: spacing 0.1 nm, **Find windows**, then
   **Create window calculations** (equilibration and production steps as
   wanted). Launch the -eq tasks, and each production task when its -eq is
   Complete.
8. **Run WHAM** when the production runs are Complete. Read the binding free
   energy as the depth of the deepest minimum below the plateau at large
   distance, if the PMF has levelled off; if a warning names a gap, add a
   window there and run WHAM again.

## Following a protein-pair umbrella lab step by step

A lab that pulls one chain off a fibril (the 2BEG example above) usually has
these steps. Each one is done in ECCE as follows.

| Lab step | In ECCE |
|---|---|
| Load the structure and count the chains | **Prepare** task, **Source: Protein**, **Structure: PDB file...**; the chains are listed under **Restrain chain** on the run pages |
| Prepare: force field, box, ions, remove the starting structure | the same Prepare: **Force field**, **Water model**, **Box size (x y z)** and **Solute centre (x y z)**, **Neutralise** and the salt concentration. Launch; the solvated system is its output |
| Energy minimisation | **GROMACS MD Optimize** task: steepest descent, steps and `emtol` under the extra lines. Launch |
| NPT equilibration with every chain restrained, 310 K | **GROMACS MD Equilibrate** task: ensemble NPT, temperature 310 K (the starting velocities follow it), **Restrain chain A** to **E**. Launch |
| Density, pressure and temperature plots | **MD Analysis** on the Equilibrate task: **Energy** (pressure, temperature) and **Density** |
| Index groups for the two chains | **Groups by residue name** on the run settings page: `Chain_A = mol:Protein_chain_A`, `Chain_B = mol:Protein_chain_B` |
| Pull run with chain B restrained | **GROMACS MD Dynamics** task: **Restrain chain B**, **Pull group 2 away from group 1** (`Chain_B`, `Chain_A`), rate, force constant, energy and trajectory output intervals. Launch |
| Run the heavy steps on a cluster | register the cluster under **Machine Registration** with its queue manager (Slurm); the task's Launch dialog then sends the job there and brings the files back. Nothing is copied by hand |
| Force against time, RMSD and radius of gyration of the pull | **Pull Force** in the pull task's Properties: **Export .csv...** for the force, **RMSD...** and **Radius of gyration...** |
| Choose the starting conformations (distance range and spacing) | **Umbrella sampling** page: **Spacing**, **From**, **To**, **Find windows** |
| Equilibrate and run every window | **Create window calculations**, then **Run Mgmt > Launch Selected** (windows run side by side on a cluster) |
| WHAM, the histogram plot and the PMF | **Run WHAM**: the PMF and the histograms are on the page, **Details...** has each pair's overlap |
| Gaps in the sampling | the warning names the distance. Run **Find windows** with a smaller spacing over that range and **Create window calculations** again: the new windows are numbered after the ones that exist. Launch them, then **Run WHAM** again |
| Binding free energy | the deepest minimum of the PMF below its plateau at large distance, in the summary and in **Export .csv...** |

Reading the binding free energy needs the PMF to level off at large distance.
If it is still rising where the sampling stops, pull further (a longer box and a
longer pull run) before extracting windows.

## Command line

`ecce-umbrella` does the same on stored calculations and prints text, CSV
(`--csv`) or JSON (`--json`):

```
ecce-umbrella windows <pull calc> [--spacing 0.1] [--from D] [--to D]
ecce-umbrella extract <pull calc> --out DIR [--spacing 0.1]
ecce-umbrella wham <window calc>... [--temp 300] [--bins 200] [--jacobian]
```
