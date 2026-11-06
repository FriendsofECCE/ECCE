# ML Potential: energies and geometries with a machine-learned potential

The **MLP** code button runs a machine-learned interatomic potential through
ASE. The first potential is MACE, trained on a large set of DFT calculations
of crystals (MACE-MP-0 and its successors). It gives an energy, the forces
and, for a periodic cell, the stress in seconds to minutes where a DFT
calculation takes hours. It is meant for screening: relaxing many candidate
structures, such as ion positions in a large oxide cell, before the best ones
go on to CP2K or Quantum ESPRESSO.

**The energies are for screening, not for reporting.** They rank structures
that are alike. They are not DFT energies and are not a result to quote.

## Setting it up

ECCE does not ship PyTorch, MACE or any model weights. Make a Python
environment that has them, for example

    python3 -m venv ~/mlp-venv
    ~/mlp-venv/bin/pip install ase mace-torch

and add `torch-dftd` if you want the D3 dispersion correction. On the
**Codes** tab of the machine in **Register Machines**, register **MLP** and set
**Program** to the Python of that environment, such as
`/home/you/mlp-venv/bin/python3`. A cluster needs the same environment on the
cluster and that path on its machine entry. The runner itself is written into
the run directory by the job script, so ECCE need not be installed there.

The model weights are downloaded the first time a model is used, into the
cache of the user the job runs as. A cluster node without internet access needs
the weights downloaded beforehand.

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

**Geometry** relaxes the atoms with ASE's optimiser. **Runtype Details** has:

- **Optimizer**: FIRE (the default, robust for a poor starting structure) or
  BFGS (fewer steps near the minimum).
- **Relax the cell as well as the atoms.** For a periodic structure the cell
  vectors and the atoms are relaxed together (ASE's `FrechetCellFilter`),
  so the volume and the shape change until the stress is zero. This needs a
  cell: create one in the Builder's Periodic Builder, or the input is
  refused with a message. Without the tick the cell is kept.
- **Force limit** (eV/A, 0.05 by default): the run has converged when the
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

## The input file

The input is one file. Line 1 is `mlp` followed by the settings, and the rest
is the structure as extended xyz, with the cell in the `Lattice=` entry of the
comment line when there is one:

    mlp runtype=Geometry model=medium-mpa-0 dtype=float32 dispersion=off relax=cell optimizer=FIRE fmax=0.05 steps=200 timelimit=0
    2
    Lattice="4.0 0 0 0 4.0 0 0 0 4.0" Properties=species:S:1:pos:R:3 pbc="T T T"
    Na 0.0 0.0 0.0
    Cl 2.0 2.0 2.0

## Results

The output lists each optimiser step with its energy and largest force, and
then the energy, the forces and the stress of the final structure. The same
structures are written to `ecce-mlp.extxyz` in the run directory.

In ECCE the energies are in Hartree like those of the other codes (the output
file prints eV). The forces are stored as a gradient, in Hartree/Bohr. The
stress is in kbar with the sign of the pressure, positive when the cell is
compressed, as for Quantum ESPRESSO. The Builder's Geometry Trace shows the
steps, with the cell when it changes.
