# CP2K: the SCF loop

In **Theory Details** the **SCF** box sets how CP2K finds the electronic
ground state. With **Orbital transformation (OT)** checked, which is the
default, CP2K minimises the energy directly and is robust for molecules and
other systems with a band gap. Uncheck it to use diagonalisation with
mixing, which is the choice for metals together with a smearing temperature.

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
