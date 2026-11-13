**ECCE 9.4.16**, a patch release of 9.4.15. Please report problems as [issues](https://github.com/FriendsofECCE/ECCE/issues).

**Platforms.** Linux (Debian 13; Ubuntu 24.04, RHEL/Rocky 9 and Fedora 44 packages are built on those systems but not tested). Windows 10 and 11. macOS, Apple silicon and Intel: experimental.

## Changes
- **Windows:** the Organizer no longer freezes while the Tail window follows a running job's output; the window reads only what has arrived (#1006).
- **Gaussian 16 orbitals** are right again for basis sets whose contractions share primitives, such as cc-pVTZ: Gaussian re-contracts such a basis, so its orbitals are now converted from the basis Gaussian used to the one ECCE wrote (#921).
- **Gaussian 16 traces:** an IRC now includes the transition state, so every point of the path has its structure, energy and reaction coordinate. An optimisation no longer shows the converged structure twice, a QST3 transition-state search has its energy trace, and geometry traces are labelled in Å (#946, #947).
- **Show Hydrogen Bonds** uses the criterion of `gmx hbond`: donor-acceptor distance at most 3.5 Å and H-D...A angle at most 30°, with N, O, F and S donors and N, O, F, chloride and bromide acceptors. In a structure with a unit cell the nearest periodic image of the acceptor is used. **Options > Hydrogen Bond Settings...** changes the distance and the angle (#978). Large structures redraw their hydrogen bonds much faster, also at every step of a trace (#982).
- **Builder:** clicking an atom works again while atom labels with numbers are shown (#975). Changing the display style no longer zooms out to the whole system (#977).
- **Organizer project table:** Ctrl+click adds or removes a row and Shift+click extends or shrinks a range again (#925).
- **MOs panel:** the energy-level graphs are filled for calculations without orbital symmetry labels, such as ORCA's (#882). The panel no longer widens the side column (#843).
- **Terminate** no longer leaves a program running after a cancel: the job script waits for the program, and ECCE then stops whatever is still running in the run directory (#855).
- **Job folders** under ECCE's temporary folder are removed when a job fails to start, and old ones are cleared when the Organizer starts (#924).
- **Units** are written Å, °, cm⁻¹ and so on in labels, tooltips and help (#998).
- Multiplicity 9 is called **Nonet** (#853).

## Known issues
- **Titanium** is not among the atoms: its restricted open-shell calculation does not reliably reach the 3d² 4s² ground state (#797).
- **HiDPI on Windows:** a few items are still unchecked or off at 150-200 % (#670). **Windows:** uninstalling takes long (#815); the first start takes about a minute on slower computers (#444).
- **GROMACS, CP2K and Quantum ESPRESSO** remain experimental. **The MO diagram** is experimental.
