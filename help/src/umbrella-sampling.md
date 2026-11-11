# Umbrella sampling with GROMACS

Umbrella sampling gives the free energy (the potential of mean force, PMF)
along a distance between two groups, for example between two protein chains
or between two ions. The procedure has three steps, all done in ECCE:

1. **Pull run.** A dynamics task pulls group 2 away from group 1 at a steady
   rate. Its trajectory passes through every distance you want to sample.
2. **Windows.** Frames of the pull trajectory are taken at a chosen spacing of
   the distance (0.2 nm by default). Each frame starts a window: a short
   equilibration and a production run, with the two groups held near that
   frame's distance by a harmonic spring.
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
  at half the box length. An off-centre placement (**Solute centre (x y z)** in the Prepare) puts the
  solute near one side so that the other side is free.
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
.csv...**.

## 2. The windows

On the **Umbrella sampling** page of the Pull Force panel (the **Pull run** page has the plots above):

1. Enter the **Spacing** (nm). 0.2 nm suits two protein chains with a force
   constant of 1000 kJ/mol/nm²; an ion pair needs about 0.05 nm. Leave
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
- the minima and maxima found on the PMF.

**Export .csv...** writes the PMF and the histograms.

**Remove the 2RT ln r volume term.** Along a distance between two centres of
mass, more space is available at larger distances (a shell of radius r has
an area proportional to r²). `gmx wham` leaves this term in its profile.
Ticking the box adds 2RT ln r, which gives the PMF of the pair itself: for
an ion pair this is the curve usually published, flat at large distance.
Leave it unticked to get the numbers `gmx wham` alone gives.

## An example: a sodium chloride ion pair

The test system of this procedure is Na⁺ and Cl⁻ in a 3.2 nm box of TIP3P
water (AMBER99SB-ILDN ions), pulled at 0.01 nm/ps from contact to 1.3 nm,
with windows every 0.05 nm and k = 4000 kJ/mol/nm². The PMF shows the
contact ion pair near 0.28 nm, a barrier near 0.36 nm and the
solvent-separated pair near 0.50 nm, the shape known from simulations of
NaCl in water.

## Command line

`ecce-umbrella` does the same on stored calculations and prints text, CSV
(`--csv`) or JSON (`--json`):

```
ecce-umbrella windows <pull calc> [--spacing 0.2] [--from D] [--to D]
ecce-umbrella extract <pull calc> --out DIR [--spacing 0.2]
ecce-umbrella wham <window calc>... [--temp 300] [--bins 200] [--jacobian]
```
