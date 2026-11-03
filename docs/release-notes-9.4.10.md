**ECCE 9.4.10**, a patch release of 9.4.9. Please report problems as [issues](https://github.com/FriendsofECCE/ECCE/issues).

**Platforms.** Linux (Debian 13; Ubuntu 24.04, RHEL/Rocky 9 and Fedora 44 packages are built on those systems but not tested). Windows 10 and 11. macOS, Apple silicon and Intel: experimental.

## Changes
- **Output copied back empty:** when the login to the compute host stopped working at the end of a job, ECCE reported the outputs as copied and stored an empty main output. The copy now fails in that case, each file is checked against its size on the host and copied again once if it is short, and a calculation is Complete only when its output has the code's normal-end line (#732).
- **Orbital names with the principal quantum number:** the MOs table, the composition line and the tooltips name atomic orbitals as F 1s, F 2s, H(1) 1s. **Show core orbitals** in the MO Diagram draws the core levels (F 1s) in a band below the axis break, not to scale; it is on by default for molecules of up to 5 atoms, and the choice is remembered (#827).
- **Scans with atoms far apart** (a bond scan starting at 4 Å): the viewer now re-fits when a scan or trace step puts atoms outside the view; before, the atoms only appeared after a click in the viewer (#832).

## Known issues
- **Titanium** is not among the atoms: its restricted open-shell calculation does not reliably reach the 3d² 4s² ground state (#797).
- **HiDPI on Windows:** a few items are still unchecked or off at 150-200 % (#670). **Windows:** uninstalling takes long (#815); the first start takes about a minute on slower computers (#444).
- **GROMACS, CP2K and Quantum ESPRESSO** remain experimental. **The MO diagram** is experimental.
