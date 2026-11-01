**ECCE 9.4.7**, a patch release of 9.4.6. Please report problems as [issues](https://github.com/FriendsofECCE/ECCE/issues).

**Platforms.** Linux (Debian 13; Ubuntu 24.04, RHEL/Rocky 9 and Fedora 44 packages are built on those systems but not tested). Windows 10 and 11. macOS, Apple silicon and Intel: experimental.

## Fixes
- **Input generation:** the generated input replaces the template file reliably, also on Windows; a failure to write it is reported instead of leaving the template in place.
- **Organizer:** no longer stops at start with "input vector length does not match rows*columns" for a stored table of the wrong shape; it warns once instead (#719). A calculation that no longer exists says so, and a password is blamed only for a refused login (#683). A running study task's right-click menu offers Terminate and Shell in Run Directory (#731).
- **ORCA:** a boxed "ERROR !!!" is named by the sentence under it, e.g. that the optimisation reached the maximum number of cycles (#755).
- **Progress box:** a converged optimisation reads "Optimisation converged in N steps" also when the code's converged line is missing from the stored output (#722). GROMACS density is plotted against time in ps (#738), each point once (#756).
- **GROMACS study chains:** a task starts from its provider's final structure, and waits until the provider is complete (#735). A running GROMACS job's Geometry Trace can be played: frames are written every 30 s (#752).
- **Job monitor:** the output of a job ended by the scheduler is copied after it has settled (#732).
- **Calculation summary:** the "Remaining on submission" row is removed (#737); the Launcher button sits level with the Machine row and Progress blocks keep a fixed order per code (#753).
- **Density Map and radial plots:** units on the axes at any size, the zero-density note in its own strip, no spurious node lines from basis-set tails, and the spherical average starts at the nucleus (#744).
- **MO diagram:** with localised MOs selected the status line says the diagram shows canonical MOs; correlation lines reach their levels past long labels (#745).
- **Dipole moment of an ion:** ECCE-QM reports it about the centre of mass, and the origin is shown (#746).
- **Teaching examples (General Chemistry):** electron affinities of Be and N computed at UPBE0/aug-cc-pVDZ, so both anions come out unbound as observed (#747).
- **Builder:** a click on a measured distance, angle or torsion's value opens the prompt for a new value, as a click on its line does (#716).

## Known issues
- **HiDPI on Windows:** a few items are still unchecked or off at 150-200 %: the NWChem code icon, the Dipole table's value row, a scrollbar in the Basis Set Tool, the Feedback row (#670).
- **Windows:** the first start after installing takes about a minute on slower computers, with progress shown (#444).
- **GROMACS, CP2K and Quantum ESPRESSO** remain experimental. **The MO diagram** is experimental.
