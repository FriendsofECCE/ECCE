# Checks before trusting or comparing results

1. Finished: `.ecce-meta` state Complete; for failures read runStatusReason and the end
   of the output and the run log.
2. Optimisation converged by the code's own criteria (the output's convergence table),
   not just stopped; many steps without converging means the setup is wrong (basis,
   coordinates, gradient accuracy, start structure) — find the cause, do not raise limits.
3. Stationary point: 0 imaginary frequencies for a minimum, exactly 1 for a TS, and the
   imaginary mode moves along the intended reaction coordinate.
4. Same level of theory (functional, dispersion, basis, solvent model, grid) before
   comparing energies; electronic vs ZPE-corrected vs Gibbs energies stated.
5. Basis set listed once in the input (no duplicate 'Imported' entries).
6. Geometry: bond lengths, coordination numbers, nothing dissociated or collapsed.
7. Open shell: spin multiplicity as intended, S2 close to the ideal value.
8. MD: equilibration reached (temperature, energy, density plateau) before production
   averages; averages quoted with their fluctuation.
9. State what was checked and what was not; never claim agreement with literature or
   another program without having compared.
