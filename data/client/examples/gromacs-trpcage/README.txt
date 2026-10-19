Trp-cage (PDB 1L2Y) through the tasks of a GROMACS study, as the help page
"Your first GROMACS calculation" runs it: Prepare (AMBER99SB-ILDN, TIP3P,
rhombic dodecahedron 1 nm, 0.15 mol/L NaCl), Optimize (200 steps),
Equilibrate (1000 steps) and Dynamics (2000 steps).

1l2y.pdb               the structure
prepare.recipe         what the Prepare task runs
optimize.mdp, equilibrate.mdp, dynamics.mdp   the parameters of each task
*.txt                  one energy per saved step, from gmx energy
