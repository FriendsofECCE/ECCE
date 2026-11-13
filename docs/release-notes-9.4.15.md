**ECCE 9.4.15**, a patch release of 9.4.14. Please report problems as [issues](https://github.com/FriendsofECCE/ECCE/issues).

**Platforms.** Linux (Debian 13; Ubuntu 24.04, RHEL/Rocky 9 and Fedora 44 packages are built on those systems but not tested). Windows 10 and 11. macOS, Apple silicon and Intel: experimental.

## Changes
- **Windows: no more mosquitto.exe.** A local session on Windows now uses a small message broker inside ECCE itself, started by the Organizer and listening only on this computer (127.0.0.1). `mosquitto.exe` is no longer installed, so a virus scanner, Smart App Control or a slow first start of a separate program can no longer stop ECCE from starting with "the message broker did not answer". The broker stays up while an editor or a job monitor is still open and stops about ten seconds after the last one closes. Setting `ECCE_BROKER=mosquitto` brings back the old route for a mosquitto.exe installed separately (#990). Linux, macOS and server installs are unchanged.
- **ORCA import:** the diffuse def2 basis sets (def2-SVPD, def2-TZVPD, def2-TZVPPD, def2-QZVPD, def2-QZVPPD) are recognised by the importer and the input checker (#981). The input checker also knows ma-def2-SVP.
- **GROMACS:** `nstcalcenergy` is chosen again when a user line sets `nstenergy` or `nstlog`, so grompp no longer stops on the mismatch (#762).

## Known issues
- **Titanium** is not among the atoms: its restricted open-shell calculation does not reliably reach the 3d² 4s² ground state (#797).
- **HiDPI on Windows:** a few items are still unchecked or off at 150-200 % (#670). **Windows:** uninstalling takes long (#815); the first start takes about a minute on slower computers (#444).
- **GROMACS, CP2K and Quantum ESPRESSO** remain experimental. **The MO diagram** is experimental.
