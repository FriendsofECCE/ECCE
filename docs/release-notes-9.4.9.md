**ECCE 9.4.9**, a patch release of 9.4.8. Please report problems as [issues](https://github.com/FriendsofECCE/ECCE/issues).

**Platforms.** Linux (Debian 13; Ubuntu 24.04, RHEL/Rocky 9 and Fedora 44 packages are built on those systems but not tested). Windows 10 and 11. macOS, Apple silicon and Intel: experimental.

## Changes
- **MO Diagram for a single atom:** the subshells are drawn as a textbook staircase, one column per shell n, at heights in energy order; the drawing fills the panel and wraps to the left when it is too wide. With Energies to scale, the s, p and d levels are grouped, each with its energy in eV, and the core levels below the axis break are in order. Empty subshells are named as the next shell of their type (Cr: 4p, 5s). The pi-system option is not shown for an atom (#817).
- **Atoms with d electrons** (and any calculation whose basis set has g functions) can be shown in the MO Diagram, Density Map and Density Profile; before, the orbital analysis stopped at f functions (#817).
- **Teaching examples (General Chemistry):** calcium (4s²), chromium (3d⁵ 4s¹) and copper (3d¹⁰ 4s¹) added to the atoms (#788).

## Known issues
- **Titanium** is not among the atoms: its restricted open-shell calculation does not reliably reach the 3d² 4s² ground state (#797).
- **HiDPI on Windows:** a few items are still unchecked or off at 150-200 % (#670). **Windows:** uninstalling takes long (#815); the first start takes about a minute on slower computers (#444).
- **GROMACS, CP2K and Quantum ESPRESSO** remain experimental. **The MO diagram** is experimental.
