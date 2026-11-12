# Checks before trusting or comparing results

In this order:

1. **State.** Read `state` in `.ecce-meta`. Complete (Completed) means the code ended normally, not that the result is right. For Unsuccessful, Killed, Failed, Incomplete, Monitor Error or System Error, read `runStatusReason`, the end of the output, `Outputs/slurm.err` and the run log. "Wall time reached" means the job was stopped, not that it converged.
2. **Convergence by the code's own criteria.** Look at the output's convergence table, not only the fact that the job stopped. Many steps without converging means the setup is wrong (basis, coordinates, gradient accuracy, start structure, SCF settings): find the cause, don't raise limits. For CP2K, check every SCF ("SCF run NOT converged" lines) and the final one; for a CELL_OPT, the final pressure and the gradient against its limits.
3. **Stationary point.** A minimum has 0 imaginary frequencies; a transition state exactly 1, moving along the intended coordinate (look at the mode). Leading near-zero frequencies are translations and rotations.
4. **Same level before comparing energies.** Same code, functional, dispersion, basis, solvent model, grid and cutoff. Say which energy is meant: electronic, ZPE-corrected, enthalpy or Gibbs. Never combine energies from different codes in one reaction energy.
5. **Basis set listed once** in the input. A duplicate "Imported" entry is a known fault.
6. **Geometry.** Bond lengths and coordination are sensible, and nothing has dissociated or collapsed. For periodic cells, look at the cell parameters and volume.
7. **Open shells.** The multiplicity is as intended; ⟨S²⟩ is close to S(S+1). Check where the spin sits (Mulliken spin populations, spin density) before calling a reduction metal- or ligand-centred: oxidation states in names are formal.
8. **MD.** Equilibration is reached (temperature, energy, density or volume plateau) before production averages are taken. Quote averages with their fluctuation, and check the time step and constraints in the `.mdp`. In chained studies, check that each task started from the previous task's output.
9. **Cost.** Compare core-hours used (cores × elapsed) with the estimate, and note it.
10. **Report** what was checked and what wasn't. Never claim agreement with literature or another program without having compared. Give units for every value.
