**ECCE 9.4.8**, a patch release of 9.4.7. Please report problems as [issues](https://github.com/FriendsofECCE/ECCE/issues).

**Platforms.** Linux (Debian 13; Ubuntu 24.04, RHEL/Rocky 9 and Fedora 44 packages are built on those systems but not tested). Windows 10 and 11. macOS, Apple silicon and Intel: experimental.

## New
- **MO Diagram for a single atom:** the panel shows the atom's subshells with their electrons as arrows (aufbau, Hund's rule), as textbook boxes in energy order, or with the orbital energies to scale. Clicking an orbital shows it in the viewer (#813).

## Fixes
- **ECCE-QM restricted open-shell (ROHF/ROKS):** atoms with a partly filled degenerate shell (O, F, S, Cl, Ti) converge; orbitals are written in energy order (#790).
- **Teaching examples (General Chemistry):** the 13 open-shell atoms are restricted open-shell (ROB3LYP), so each has one orbital list with occupations 0, 1 and 2; empty members of a partly filled shell are labelled with their shell number, e.g. "2p (virtual)" (#788).
- **Workflow example SN2:** the Gaussian 16 IRC step opens with its IRC path, orbitals and progress; an imported Gaussian 16 output whose route has IRC is read as an IRC run (#784).
- **SSH passwords:** a remembered password is offered only to its own host, port and user (#791). A network or connect failure is reported as unreachable, not as a refused password (#777).
- **Viewer:** trackpad zoom stops when the fingers stop; pinch and keyboard zoom (#811).
- **Properties list:** a category keeps its heading when it has a single entry (#804).
- **Run dates:** a job of a few seconds no longer shows a start date after its end.
- **Examples:** the stored copies of all example sets are regenerated with the 9.4.8 parsers.

## Known issues
- **HiDPI on Windows:** a few items are still unchecked or off at 150-200 %: the NWChem code icon, the Dipole table's value row, a scrollbar in the Basis Set Tool, the Feedback row (#670).
- **Windows:** the first start after installing takes about a minute on slower computers, with progress shown (#444).
- **GROMACS, CP2K and Quantum ESPRESSO** remain experimental. **The MO diagram** is experimental.
