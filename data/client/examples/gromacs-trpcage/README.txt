Trp-cage (PDB 1L2Y) through the tasks of a GROMACS study, as the help page
"Your first GROMACS calculation" runs it: Prepare (AMBER99SB-ILDN, TIP3P,
rhombic dodecahedron 1 nm, 0.15 mol/L NaCl), Optimize (200 steps),
Equilibrate (1000 steps) and Dynamics (2000 steps).

The directory is a finished run in the layout File > Import GROMACS Run...
reads: one subdirectory per task, in run order. Importing it makes a GROMACS
MD study with the inputs, outputs and energies of each task; the Organizer
does this by itself on the first start and under File > Open Example
Calculations.

1-prepare/      the recipe, 1l2y.pdb, and the prepared structure and topology
2-optimize/     optimize.mdp, the mdrun log and the final structure
3-equilibrate/  the same for Equilibrate
4-dynamics/     the same, and the trajectory frames
