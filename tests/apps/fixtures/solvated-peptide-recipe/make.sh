#!/bin/sh
set -e
# Regenerates ../solvated-peptide.gro (GROMACS 2025.2, OPLS-AA, TIP3P):
# a 4-residue peptide, 48 waters, one NA and one CL.  Run in an empty
# directory.
here=$(dirname "$0")
python3 "$here/mkpep.py"
gmx_mpi pdb2gmx -f pep.pdb -o pep.gro -p topol.top -ff oplsaa -water tip3p -ignh > l0 2>&1
printf 'integrator=steep\nnsteps=200\ncutoff-scheme=Verlet\nrlist=0.6\nrcoulomb=0.6\nrvdw=0.6\ncoulombtype=cutoff\n' > em.mdp
gmx_mpi editconf -f pep.gro -o box.gro -box 1.9 1.4 1.4 -c > l1 2>&1
gmx_mpi solvate -cp box.gro -cs spc216.gro -maxsol 50 -p topol.top -o solv.gro > l2 2>&1
gmx_mpi grompp -f em.mdp -c solv.gro -p topol.top -o ions.tpr -maxwarn 5 > l3 2>&1
echo SOL | gmx_mpi genion -s ions.tpr -o ionized.gro -p topol.top -np 1 -nn 1 > l4 2>&1
gmx_mpi grompp -f em.mdp -c ionized.gro -p topol.top -o em.tpr -maxwarn 5 > l5 2>&1
gmx_mpi mdrun -ntomp 1 -deffnm em > l6 2>&1
cp em.gro "$here/../solvated-peptide.gro"
