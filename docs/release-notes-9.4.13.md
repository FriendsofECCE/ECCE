**ECCE 9.4.13**, a patch release of 9.4.12. Please report problems as [issues](https://github.com/FriendsofECCE/ECCE/issues).

**Platforms.** Linux (Debian 13; Ubuntu 24.04, RHEL/Rocky 9 and Fedora 44 packages are built on those systems but not tested). Windows 10 and 11. macOS, Apple silicon and Intel: experimental.

## Changes
- **Windows: ECCE starts on computers where virus scanning is slow.** The first start of ECCE's message broker can take more than 20 seconds while Windows Defender checks it. ECCE gave up after 20 seconds and tried again in the same folder while the first broker was still starting, so every try failed with "ECCE could not start its message broker" ("Unable to open log file ... for writing"). ECCE now waits up to 60 seconds, gives each try its own folder and stops a try that failed (#958).
- **Long lines in an input file.** When a calculation was launched, an input line of 254 characters or more cut the input file at that line, and the shortened file replaced the stored one. Input files are now copied whole (#957).
- **Windows: Smart App Control.** When Smart App Control (Windows 11) is on, it blocks ECCE because ECCE is not code-signed, with an error that names a DLL file. ECCE now says this in words when it starts, with how to switch Smart App Control off; the problem report (`ecce-report.ps1`) shows whether it is on (#933).

## Known issues
- **Titanium** is not among the atoms: its restricted open-shell calculation does not reliably reach the 3d² 4s² ground state (#797).
- **HiDPI on Windows:** a few items are still unchecked or off at 150-200 % (#670). **Windows:** uninstalling takes long (#815); the first start takes about a minute on slower computers (#444).
- **GROMACS, CP2K and Quantum ESPRESSO** remain experimental. **The MO diagram** is experimental.
