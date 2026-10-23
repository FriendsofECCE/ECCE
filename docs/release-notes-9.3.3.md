**ECCE 9.3.3.** Please report problems as [issues](https://github.com/FriendsofECCE/ECCE/issues).

**Platforms.** Linux (Debian 13; Ubuntu 24.04, RHEL/Rocky 9 and Fedora packages are built on those systems but not tested). Windows 10 and 11. macOS, Apple silicon and Intel: experimental.

9.3.3 contains bug fixes only.

## Fixes
- **A failed job keeps its output, status and progress** (#470, #467). A job whose output ended inside a block of the code's parse description killed the job monitor, so nothing was stored; the open block is now dropped and the rest is stored.
- **Gaussian 16 with a solvent (PCM/CPCM): properties are stored again for larger molecules** (#491). For a cavity of more than a few spheres Gaussian does not print the line that closed the solvent block, so every later result of the run (energies, orbitals, the geometry trace, frequencies) was lost, during the run and at the end.
- **GROMACS: structures of a triclinic (dodecahedral) box are shown wrapped into the compact cell**, with molecules kept whole (#476).
- **GROMACS Progress box:** plots for energy minimisation and dynamics, read from the log, the table and the energy file (#477).
- **GROMACS Inputs page:** each file name is on its own line under its label and buttons, so long names are no longer cut off.
- **Job start date** is kept when the job monitor restarts, and job times are shown to the second (#463).
- **Builder:** Select Solvent finds water by residue name, Select Ions and Select Molecule work on the selection or the solute, and a style applies to the selection only (#479).
- **GROMACS Run settings:** when "Defined groups" is chosen for the centre-of-mass or temperature-coupling groups and no group is defined, the run uses System and a note in the .mdp header says so, instead of stopping with an error; the page shows a warning line (#482).

## Known issues
- **Windows:** the first start after installing takes about a minute on slower computers, with progress shown (#444). Repair from the installer pauses for about a minute before its progress page appears (#431).
- **GROMACS, CP2K and Quantum ESPRESSO** remain experimental. **The MO diagram** is experimental.
