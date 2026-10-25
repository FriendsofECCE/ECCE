# Speeds and diffusion in a molecular dynamics run

Two panels of the Builder analyse the motion of the atoms in a GROMACS dynamics
run: **Speed Distribution** and **Displacement (MSD)**. Open them from the
**Properties** list, in the Energies group beside **Pull Force**. They appear for
a run that carries the analysis: the examples of the **General Chemistry** set
(see [Example calculations](example-calculations.md)) and any run imported with
an `ecce-parse-md.txt` made by the `ecce-mdanalysis` program (below).

## Speed distribution

The panel draws a histogram of the speeds of the atoms, averaged over every
frame of the trajectory, for each kind of atom in the run (atoms of equal mass
are one kind). The dashed curve of the same colour is the Maxwell-Boltzmann
distribution,

f(v) = 4π (m / 2πk<sub>B</sub>T)<sup>3/2</sup> v² exp(−mv² / 2k<sub>B</sub>T),

for the run's reference temperature T (the `ref-t` of its parameter file) and
the atom mass m from its topology. Three vertical lines mark the speeds of that
curve: the most probable speed (√(2k<sub>B</sub>T/m)), the mean speed
(√(8k<sub>B</sub>T/πm)) and the root-mean-square speed (√(3k<sub>B</sub>T/m)).
The text above the plot gives the measured mean and rms speeds beside them.

The speeds need the velocities of the run, saved with `nstvout`; a run without
them has no speed distribution. **Compare...** draws another calculation of the
same folder over this one: helium over argon shows that the lighter atoms are
faster by √(m<sub>Ar</sub>/m<sub>He</sub>), and the curve is lower and wider. **Remove**
takes it off again; **Export .csv...** writes the histograms and the curves.

## Displacement

The panel draws the mean-square displacement (MSD) of each molecule type against
the lag time. A molecule is represented by its centre of mass, followed through
the periodic box without jumps, and the displacement over a lag time is
averaged over all molecules and over the time origins along the trajectory.

At short times the atoms of a dilute gas fly freely and the MSD grows as t²; once
collisions have randomised the direction it grows linearly. The **Log-log**
check box shows both: a slope of 2 for the free flight, a slope of 1 for
diffusion. In the linear part the Einstein relation gives the diffusion
coefficient,

D = slope / 6.

The dashed line is the straight line fitted to the points between the two
vertical lines. **Fit from ... to ... ps** sets that range (the default is the
last three quarters of the lag times; the longest lag times have the fewest
independent origins, so pick the range where the curve is straight); **Fit**
applies it. D is given in 10⁻⁵ cm²/s and in m²/s. **Compare...** draws another
calculation of the folder over this one: liquid argon over argon gas, or decane
over water, with each curve fitted on its own.

## Making the analysis for your own run

The program `ecce-mdanalysis` reads the topology and the `.trr` or `.xtc` file of
a run and writes the file that ECCE imports:

    ecce-mdanalysis --top topol.top --mdp prod.mdp \
        --traj prod.xtc --out ecce-parse-md.txt

Add `--vel prod.trr` for the speed distribution. `--fit-from` and `--fit-to` set
the fit range of the summary it prints; `--lags` sets the number of lag times.
Put `ecce-parse-md.txt` in the folder of the task you import (see
[Importing a calculation](importing-a-calculation.md)). Positions written every
1 ps or more often suit the MSD; use frames closer together than half the box
over the speed of the fastest atoms, since the unwrapping follows each molecule
by its shortest step.
