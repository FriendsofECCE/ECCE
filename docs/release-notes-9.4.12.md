**ECCE 9.4.12**, a patch release of 9.4.11. Please report problems as [issues](https://github.com/FriendsofECCE/ECCE/issues).

**Platforms.** Linux (Debian 13; Ubuntu 24.04, RHEL/Rocky 9 and Fedora 44 packages are built on those systems but not tested). Windows 10 and 11. macOS, Apple silicon and Intel: experimental.

## Changes
- **Windows: user names with å, ä, ö and other letters.** ECCE did not start for a Windows user whose profile folder has letters outside A–Z (for example C:\Users\Åsa Öberg): the message broker could not open its files and the Organizer stopped before its window opened. Every ECCE program now uses UTF-8 for file names, so such folders work for starting ECCE and for running jobs (#922).
- **Windows: a failed start is always reported.** If ECCE cannot start, a message says what failed and where the start-up log is (`.ECCE\start.log` in your user folder), instead of the start-up card closing without a word. The card shows the version being started (#922).
- **Folders with a space or å/ä/ö** in the path (home folder, project, calculation name) run local jobs on Linux, macOS and Windows (#922).
- **macOS:** the Intel app opens on macOS 10.15 and later (it asked for macOS 12 before), and a separate Intel app for **macOS 10.13 and 10.14** is available as `ECCE-9.4.12-x86_64-macos10.13.dmg` (experimental; not tested on a real 10.13 Mac) (#923).
- **Update notice:** 9.4.x tells you only about newer 9.4 releases, as 8.x does, so a class on the long-term 9.4 line is not asked to move to 9.5.

## Known issues
- **Titanium** is not among the atoms: its restricted open-shell calculation does not reliably reach the 3d² 4s² ground state (#797).
- **HiDPI on Windows:** a few items are still unchecked or off at 150-200 % (#670). **Windows:** uninstalling takes long (#815); the first start takes about a minute on slower computers (#444).
- **GROMACS, CP2K and Quantum ESPRESSO** remain experimental. **The MO diagram** is experimental.
