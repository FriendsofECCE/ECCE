**ECCE 9.4.4**, a patch release of 9.4.3. Please report problems as [issues](https://github.com/FriendsofECCE/ECCE/issues).

**Platforms.** Linux (Debian 13; Ubuntu 24.04, RHEL/Rocky 9 and Fedora 44 packages are built on those systems but not tested). Windows 10 and 11. macOS, Apple silicon and Intel: experimental.

## Fixes
- **Windows: the local data folder chosen in Preferences is used** (#680). The Windows launcher always used `ecce-local` in your user folder and ignored the preference; it now reads it, as on Linux and macOS, and offers to move your calculations to the new folder at the next start. Without the preference, Windows keeps using `ecce-local`.
- **Windows: atom labels are drawn again** (#672). The labels use the font bundled with ECCE on every system.
- **Viewer:** a long text in the top-right corner (such as the dipole moment) moves one line down instead of running into the text on the left.
- **Viewer, Calculation Summary:** Launch Info in one column, so dates, Cost and Allocation are no longer cut off at the panel's edge. The Organizer's summary keeps its two columns.
- **Spin density tables:** every cell is centred vertically, also beside a wrapped cell.
- **CP2K:** a converged geometry optimisation says "Optimisation converged" in the Progress box instead of "stopped". Run Mgmt > Re-Parse Outputs corrects a calculation that has already finished.
- **Properties menu:** UV/Vis Spectrum (was UV-vis Spectrum).

## Known issues
- **Density Map:** the full map of a large calculation still takes a while after zooming.
- **Windows:** the first start after installing takes about a minute on slower computers, with progress shown (#444).
- **GROMACS, CP2K and Quantum ESPRESSO** remain experimental. **The MO diagram** is experimental.
