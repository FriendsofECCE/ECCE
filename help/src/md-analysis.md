# MD Analysis of a GROMACS run

The **MD Analysis** panel runs GROMACS's own analysis tools on the files a
GROMACS run brought back: the trajectory (`traj.xtc`), the run input
(`topol.tpr`) and the energy file (`ener.edr`). Open it from the
**Properties** list, in the Energies group beside **Speed Distribution** and
**Displacement (MSD)** (see [Speeds and diffusion](md-speeds-and-diffusion.md)).
GROMACS (`gmx`) must be installed on the computer the Builder runs on.

## Using the panel

1. Choose the **Analysis**.
2. Choose the groups. The lists hold every molecule type, residue name,
   element and atom name of the run's topology. A group may also be typed:
   `residue:SOL & element:O` is the oxygen atoms of the water (both conditions
   hold), `element:Na,K` is the sodium and potassium atoms, `all` is every atom.
3. Set the parameters of the analysis, and **From** and **to** (ps) to use
   part of the run only.
4. Press **Run**. The tool runs in the background; the line at the bottom of
   the panel shows that it is running, then how long it took and any notes.

The plot shows the result; the box below it gives the numbers (positions,
averages) and the GROMACS commands that were run, and can be copied.
**Export .csv...** writes the curves and the numbers.

## Analyses

- **Radial distribution g(r)**: `gmx rdf`. g(r) of the **Selection** around
  the **Reference**, and the coordination number cn(r), the number of
  Selection atoms within r of one Reference atom (**Plot** switches between
  the two). The numbers give the first peak, the first minimum after it and
  cn(r) at that minimum: the number of neighbours in the first shell.
  **Cut-off** is the largest r (empty: half the shortest box edge), **Bin
  width** the resolution, 0.002 nm unless set.
- **Energy terms**: `gmx energy`. Every term of the energy file is listed;
  tick those to plot against time. For each the numbers give the average, the
  error estimate, the RMSD and the total drift (the change of a straight line
  fitted to the term from start to end), as GROMACS prints them. Terms of
  different units share one axis, so plot those with similar values together.
- **RMSD from the start**: `gmx rms`. The root-mean-square deviation of the
  **Group** from the starting structure of the run, after a least-squares fit
  on the same group.
- **Radius of gyration**: `gmx gyrate`. The mass-weighted radius of gyration
  of the Group and its components about x, y and z. For a single molecule
  (a protein chain) it measures its size; for a group of separate molecules
  (ions, solvent) it is the spread of the whole group.
- **Density profile**: `gmx density`. The number density (nm⁻³), mass density
  (kg m⁻³) or charge density (e nm⁻³) of the Group in **Slices** along the
  **Axis**, averaged over the frames: layers at an interface or in a slab.
- **Hydrogen bonds**: `gmx hbond`. The number of hydrogen bonds within the
  Group, or between it and the second group, in each frame: donor-acceptor
  distance at most 0.35 nm, hydrogen-donor-acceptor angle at most 30°, donors
  and acceptors N and O.

## When the files are missing

A run fetches its `traj.xtc`, `ener.edr` and `topol.tpr` when it ends. The
panel says which file is missing and why: the task was set to **Leave
trajectory files on the machine**, the file was larger than the fetch limit
(500 MB unless the site sets another; fetch it with `ecce-fetch`), or the run
wrote no compressed trajectory (`nstxout-compressed` of 0). Runs made before
ECCE fetched these files have no MD Analysis data.

## From the command line

`ecce-gmxanalysis <calculation folder> rdf --ref residue:NA --sel "residue:SOL & element:O"`
prints the same numbers; `--csv` writes the curves and `--json` everything.
`ecce-gmxanalysis --help` lists the analyses and their options.
