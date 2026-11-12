# MLP (MACE): energies, geometries, vibrations and dynamics with a machine-learned potential

The **MLP (MACE)** code button runs a machine-learned interatomic potential.
The network runs in Python (PyTorch and `mace-torch`); the optimisers, the
cell relaxation, the molecular dynamics and the vibrations are ECCE's own
(`ecce-mlp-driver`). The first potential is MACE, trained on a large set of DFT calculations
of crystals (MACE-MP-0 and its successors). It gives an energy, the forces
and, for a periodic cell, the stress in seconds to minutes where a DFT
calculation takes hours. It is meant for screening: relaxing many candidate
structures, such as ion positions in a large oxide cell, before the best ones
go on to CP2K or Quantum ESPRESSO.

**The energies are for screening, not for reporting.** They rank structures
that are alike. They are not DFT energies and are not a result to quote.

## Setting it up

ECCE does not ship PyTorch, MACE or any model weights. **Register Machines**
builds the environment for you: on the **Codes** tab of the machine, select
**MLP (MACE)** and press **Set up...** next to **Program**. Give a folder on
that machine (`~/ecce-mlp` by default), then the PyTorch build:

- **CPU** (the default): about 1.5 GB in all; right for most machines.
- **NVIDIA GPU, CUDA**: several GB more. Choose it only when the machine
  that runs the calculations (on a cluster, the compute nodes) has an NVIDIA
  GPU.

ECCE then, on that machine:

1. creates a Python environment in `<folder>/venv` (this needs Python 3 with
   its `venv` module; on Debian or Ubuntu: `apt install python3-venv`);
2. installs PyTorch, the build you chose;
3. installs `mace-torch` and ASE;
4. downloads the default model, `medium-mpa-0`, into `<folder>/venv/cache`,
   where the job looks first, so compute nodes need no internet access;
5. checks that the new environment imports them and builds ECCE's
   optimiser and dynamics program, `ecce-mlp-driver`, into
   `<folder>/venv/bin` with the machine's C++ compiler (`c++`, C++17; on
   Debian or Ubuntu: `apt install g++`),

and sets **Program** to `<folder>/venv/bin/python`. A window shows the step
and the last line of the log, `<folder>/setup.log`; **Cancel** stops the
set-up. The CPU-only environment takes about 1.5 GB of disk and several minutes,
most of it the PyTorch download. For a cluster, choose a folder the compute
nodes can read (usually under your home directory): the set-up runs on the
login node over the machine's own connection.

The same steps without the window, on any Linux or macOS machine:

    sh $ECCE_HOME/scripts/ecce-mlp-setup ~/ecce-mlp

This builds the CPU version. For the GPU build name it, `cuda`; `auto` picks
CUDA when `nvidia-smi` finds a GPU on the machine where the script runs:

    sh ecce-mlp-setup ~/ecce-mlp medium-mpa-0 cuda

The script builds `ecce-mlp-driver` only when `ecce-mlp-driver.cpp` is in the
folder; copy it there from `$ECCE_HOME/scripts` first.

**Without `ecce-mlp-driver`** (no C++ compiler on the machine, or a
hand-made environment) the job falls back to ASE's optimisers and says so in
the output (`Engine: ASE`); Energy, Geometry and Vibration still work, but
there is no Molecular Dynamics. On the machine ECCE runs on, the driver
installed with ECCE is used.

To set up by hand instead, make an environment that has `ase` and
`mace-torch` and set **Program** to its Python, such as
`/home/you/mlp-venv/bin/python3`. Add `torch-dftd` if you want the D3
dispersion correction (the set-up does not install it). The runner itself is
written into the run directory by the job script, so ECCE need not be
installed on a cluster.

A model that is not in `<folder>/venv/cache` (another model, or a hand-made
environment) is downloaded the first time it is used, into the cache of the
user the job runs as; a node without internet access then needs it
downloaded beforehand.

## In the Calculation Editor

The Notes row under the controls says that **MLP energies are for screening,
not for reporting**. When the structure has a charge or a multiplicity above
1, it also says **MACE-MP has no charge or spin; this result ignores both**.
The calculation can still be run: the potential sees a neutral closed-shell
structure.

## Theory: MACE

**Theory Details** has:

- **MACE model.** The default is `medium-mpa-0`, trained on the Materials
  Project and Alexandria data and released under the MIT licence, as are
  `medium-0b3`, `small`, `medium` and `large`. The models marked (ASL)
  (`medium-omat-0`, `small-omat-0`, `mace-matpes-pbe-0`,
  `mace-matpes-r2scan-0`) are under the Academic Software License, which does
  not allow commercial use; the panel says so when one is chosen, and so does
  the output file. Which model ran, the version of the `mace-torch` package and
  its licence are stored with the result and printed at the top of the output.
- **Precision**, `float32` (the default) or `float64`. `float32` is faster
  on a CPU, and is enough for screening. Use `float64` for a
  tight optimisation near the minimum or to check a result.
- **D3 dispersion.** Adds Grimme's D3 correction on top of the model. It needs
  the `torch-dftd` package in the same environment; without it the job stops
  with a message saying so. Most of these models were trained on PBE data,
  which has no dispersion, so the option is for systems where van der Waals
  attraction matters, such as layers or molecules on a surface.

There is no basis set, charge or multiplicity: the potential knows only the
elements and the positions.

## Run types

**Energy** gives the energy, the forces and, for a periodic structure, the
stress, at the given geometry.

**Geometry** relaxes the atoms. **Runtype Details** has:

- **Optimizer**: FIRE (the default, robust for a poor starting structure) or
  BFGS (fewer steps near the minimum). Both follow ASE's algorithms and
  settings step for step, so they take the same path as ASE would.
- **Relax the cell as well as the atoms.** For a periodic structure the cell
  vectors and the atoms are relaxed together (the cell is varied through the
  logarithm of its deformation, as in ASE's `FrechetCellFilter`), so the
  volume and the shape change until the stress balances the pressure. This needs a
  cell: create one in the Builder's Periodic Builder, or the input is
  refused with a message. Without the tick the cell is kept.
- **External pressure** (GPa, 0 by default; with the cell relaxed only): the
  relaxation minimises the enthalpy E + pV, so the cell ends where the stress
  equals this pressure. A negative value stretches the cell.
- **Force limit** (eV/Å, 0.05 by default): the run has converged when the
  largest force on an atom, and with the cell relaxed the corresponding stress
  term, is below this.
- **Maximum steps** (200 by default) and **Time limit** in minutes (0 for
  none). A run that reaches either limit stops and keeps its last structure:
  the geometry trace, energy and forces are those of the last step, and the
  calculation ends as not converged so the result is not taken for a finished
  optimisation.

A large cell is slow on a CPU. A fixed-cell FIRE relaxation of a box with several
hundred atoms in `float64` can run for an hour or more without finishing. Use
`float32`, a time limit, a cluster with a GPU where one is available, and look
at the Progress box, which shows the step, the energy and the largest force
as the run goes. The number of CPU threads is the processor count chosen in
the Launcher.

**Vibration** gives the harmonic frequencies and normal modes at the given
geometry, for a molecule or a periodic structure. The Hessian is built from
central differences of the forces: each atom is moved by plus and minus a
small step along x, y and z, so the run takes 6N force calculations for N
atoms. **Runtype Details** has:

- **Displacement** (A, 0.01 by default): the step. Smaller steps follow the
  curvature more closely but magnify the noise in the forces; between 0.005
  and 0.02 A the frequencies agree to well within 1 cm⁻¹ for `float64`.

Optimise the structure first (a Geometry run, then a new calculation from its
last structure): at a geometry that is not a minimum the frequencies have no
meaning, and the output warns when the largest force is above 0.05 eV/Å. The
translations, and for a molecule the rotations, are removed from the Hessian
and listed first with a frequency of 0, as for the other codes; an imaginary
frequency is shown as a negative number. For a periodic structure the
frequencies are those at the centre of the Brillouin zone (the Gamma point)
of the cell as given: a larger supercell gives more of the phonon spectrum.
There is no dipole moment, so there are no IR intensities, and no
thermochemistry. The modes appear in the Builder's Vibrations panel like those
of the other codes. `float32` changes the frequencies by about 0.1 cm⁻¹ or
less for small systems; use `float64` to be sure.

**Molecular Dynamics** integrates Newton's equations with velocity Verlet,
for a molecule or a periodic box (periodic boundary conditions in all three
directions). The run starts from random velocities at the given temperature
(Maxwell-Boltzmann, with no overall drift). **Runtype Details** has:

- **Ensemble**: **NVT (Bussi thermostat)**, the default, keeps the
  temperature by rescaling the velocities stochastically (Bussi, Donadio and
  Parrinello, 2007), which samples the canonical ensemble; **NVE** keeps the
  total energy, with no thermostat.
- **Temperature** (K, 300 by default): the thermostat's, and the starting
  velocities' for NVE.
- **Time step** (fs, 1 by default): 1 fs for structures with hydrogen, up
  to 2 to 5 fs for heavier atoms only. A time step that is too long shows as
  a total energy that drifts in NVE.
- **Steps** (1000 by default): the length of the run is steps x time step.
- **Thermostat time constant** (fs, 100 by default; NVT only): how fast the
  temperature is brought back. Shorter holds it more tightly but disturbs the
  dynamics more.
- **Save a frame every** (steps, 10 by default): how often a structure goes
  into the trajectory and a line into the output. Each frame is stored, so
  keep the number of frames (steps / interval) to a few thousand at most.
- **Random seed** (1 by default): the same seed gives the same starting
  velocities and the same run.
- **Time limit** in minutes (0 for none): the run stops and keeps its last
  structure.

The output has one line per saved frame with the time, the potential,
kinetic and total energy and the temperature, and at the end the mean
temperature and the drift of the total energy (eV per atom per ps). In ECCE
the Builder's Geometry Trace plays the trajectory, and the energy plot is
against time in ps. The cell is fixed during MD; atoms that leave the cell
are not wrapped back, so molecules stay whole.

## The input file

The input is one file. Line 1 is `mlp` followed by the settings, and the rest
is the structure as extended xyz, with the cell in the `Lattice=` entry of the
comment line when there is one:

    mlp runtype=Geometry model=medium-mpa-0 dtype=float32 dispersion=off relax=cell optimizer=FIRE fmax=0.05 steps=200 timelimit=0
    2
    Lattice="4.0 0 0 0 4.0 0 0 0 4.0" Properties=species:S:1:pos:R:3 pbc="T T T"
    Na 0.0 0.0 0.0
    Cl 2.0 2.0 2.0

A Vibration run adds `delta=<step>` to line 1, a cell relaxation
`pressure=<GPa>`, and a Molecular Dynamics run `runtype=MD` with
`ensemble=NVT|NVE timestep=<fs> temperature=<K> tau=<fs> mdsteps=<n>
every=<n> seed=<n>`.

## Results

The output lists each optimiser step with its energy and largest force, and
then the energy, the forces and the stress of the final structure. The same
structures are written to `ecce-mlp.extxyz` in the run directory.

In ECCE the energies are in Hartree like those of the other codes (the output
file prints eV). The forces are stored as a gradient, in Hartree/Bohr. The
stress is in kbar with the sign of the pressure, positive when the cell is
compressed, as for Quantum ESPRESSO. The Builder's Geometry Trace shows the
steps, with the cell when it changes.

## Site screening

Site screening asks where an extra ion or atom fits in a periodic host, or
which pair of atoms of one element is the easiest to remove. It makes one MLP
calculation per symmetry-distinct choice, plus the host itself, so that the
energies can be compared and the best few passed on to CP2K or Quantum
ESPRESSO.

Start from an MLP (MACE) calculation of the host with its cell; its run type,
model and cell relaxation are used for every site (a Geometry run with the
atoms relaxed and the cell fixed is the usual choice). In the Organizer,
right-click it and choose **Screen Sites with MLP...**. For a host set up for
another code, duplicate it as an MLP calculation first.

- **One atom per void**: the element to insert, and two distances.
  **Minimum distance to host atoms** (1.6 Å by default) keeps only voids at
  least that far from every atom of the host. **Minimum distance between
  sites** (1.0 Å by default) merges void positions closer than that into one.
- **Vacancy pairs**: the element whose atoms are removed in pairs, and
  **Maximum pair distance** (Å, 0 for no limit).

Positions that the host's symmetry makes equivalent are calculated once. The
symmetry is the host's space group found from its atoms (all operations that
map the cell onto itself within 0.05 Å); the number of equivalent positions in
the cell is listed as the site's weight. Voids are the points farthest from
their nearest host atom, moved onto their exact special positions. The cell
should be large enough that an inserted atom or a vacancy pair does not see
its own periodic images: use a supercell.

The calculations go into a new folder named after the host, and are launched
like any series (select the folder, Launch Selected).

**Site Screening Results** (right-click the folder) lists the finished sites
sorted by energy: E(host+X) − E(host) for an inserted atom, or E(host without
the pair) − E(host) for a vacancy pair, in eV, with the site's environment
(the nearest neighbours and their distances, and the coordination number:
the neighbours within 10 % of the nearest distance). As for a CREST ensemble,
rows selected in the table are superposed in the Builder, and **New
calculation from selected** passes them on to another code.

A site whose Geometry run stopped at its step limit before converging is
still listed, with the energy of its last structure, and marked **not
converged** in the Environment column; the report says how many there are.
Its energy is an upper bound for that site, so it may rank too high. Raise the
step limit of the host calculation and screen again if the order matters.
If the host itself did not converge, every difference carries that error.

These energies rank sites; they are not formation energies for a report.
The comparison is between structures with the same atoms, so the chemical
potential of X cancels only within one screen.

On the command line, `ecce-sites interstitial host.xyz --element Li` and
`ecce-sites vacancy host.xyz --element Cl --max-dist 6` print the distinct
sites (add `--out DIR` for one structure file per site); the host is an
extended xyz file with a `Lattice=` entry.
