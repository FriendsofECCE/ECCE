**ECCE 9.4.14**, a patch release of 9.4.13. Please report problems as [issues](https://github.com/FriendsofECCE/ECCE/issues).

**Platforms.** Linux (Debian 13; Ubuntu 24.04, RHEL/Rocky 9 and Fedora 44 packages are built on those systems but not tested). Windows 10 and 11. macOS, Apple silicon and Intel: experimental.

## Changes
- **Windows: the Smart App Control warning is shown.** 9.4.13 was meant to say in words when Smart App Control (Windows 11) blocks ECCE, but the check never ran: ECCE's start script found the wrong `find` program. With Smart App Control on, ECCE waited 150 seconds and then reported that its message broker did not answer. ECCE now shows the warning, with how to switch Smart App Control off (#973).
- **Clipping plane.** Tools > Clipping plane in the Builder removes everything on one side of a plane: normal to x, y or z, a Miller plane (h k l) of the unit cell, or the plane of three selected atoms, with an offset slider and Flip side. It cuts atoms, bonds, surfaces and the unit cell (#879).
- **Replicate.** After Replicate, the atoms on the outer faces of the replicated block were missing, so a 2×2×2 block looked incomplete. They are now drawn, as for a single cell (#974).

## Known issues
- **Titanium** is not among the atoms: its restricted open-shell calculation does not reliably reach the 3d² 4s² ground state (#797).
- **HiDPI on Windows:** a few items are still unchecked or off at 150-200 % (#670). **Windows:** uninstalling takes long (#815); the first start takes about a minute on slower computers (#444).
- **GROMACS, CP2K and Quantum ESPRESSO** remain experimental. **The MO diagram** is experimental.
