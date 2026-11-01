# CP2K: the SCF loop

In **Theory Details** the **SCF** box sets how CP2K finds the electronic
ground state. With **Orbital transformation (OT)** checked, which is the
default, CP2K minimises the energy directly and is robust for molecules and
other systems with a band gap. Uncheck it to use diagonalisation with
mixing, which is the choice for metals together with a smearing temperature.
The diagonalisation-only fields are disabled while OT is on:

- **Mixing alpha** (0.4 by default) is the linear mixing fraction CP2K uses
  in Broyden mixing before it has a history to improve on.
- **Broyden history** (8 by default) is the number of previous iterations
  the Broyden mixer keeps, CP2K's `NBROYDEN`.
- **Added MOs** (0 by default) is the number of unoccupied orbitals CP2K
  computes beyond the occupied ones, its `ADDED_MOS` keyword; 0 leaves the
  keyword unwritten, and a non-zero value is written with or without
  smearing.

- **Print HOMO-LUMO gap** makes CP2K print the molecular-orbital eigenvalues
  at each SCF step, and ECCE reads the gap of the last step into the
  results. When **Added MOs** is 0 it writes `ADDED_MOS 1` so there is an
  unoccupied orbital to form the gap.

**Outer SCF loop** is available with OT and is on by default. CP2K then runs
several short OT minimisations one after another, up to **Max. outer
cycles** (10 by default), and stops when the SCF Convergence threshold is
met. **Max. SCF Iterations** is then the limit of each inner minimisation,
30 by default for a new calculation; unchecking OT sets it to 100, which
diagonalisation needs, and checking OT again sets it back to 30, unless you
had changed it. This often converges large cells, such
as oxides, where one long OT loop of 100 steps does not. Calculations you
saved earlier keep the iteration limit they were saved with. Uncheck the
outer loop to run a single OT loop with the limit you set.
