**ECCE 9.4.6**, a patch release of 9.4.5. Please report problems as [issues](https://github.com/FriendsofECCE/ECCE/issues).

**Platforms.** Linux (Debian 13; Ubuntu 24.04, RHEL/Rocky 9 and Fedora 44 packages are built on those systems but not tested). Windows 10 and 11. macOS, Apple silicon and Intel: experimental.

## Fixes
- **Windows at 150 % and 200 % display scaling** (#670): toolbar and tree icons, the Viewer's corner buttons, the Structure/Properties buttons, thumbnails, window sizes and splitter positions are drawn at the display's scale instead of at 100 %. The icons are drawn from vector images; the start window is sharp.
- **Organizer:** no longer closes ("failed to relocate label window") when a calculation is set up again for a code with a basis set, for example an xTB calculation for ORCA (#718).
- **Organizer:** right-click Reconnect Selected is enabled for an incomplete job, as Run Mgmt > Reconnect already was (#689). A calculation whose job is submitted, running or waiting, or a folder holding one, can no longer be renamed or moved (#700).
- **Launch Info:** a Wall time row: the time left of the Launcher's limit while running, the requested time while queued, the time used once ended (#711).
- **CP2K:** a run is judged by its final SCF; earlier SCF failures in an optimisation or MD run are counted in the log instead of marking the run Incomplete (#691).
- **Progress box plots** label the last cycle or step on the x axis (#713).
- **Density Map:** zooming out goes up to four times the whole plane, so the diffuse orbitals of an atom fit; the plane tooltip of a single atom says to use the screen plane (#714). **Density Profile:** zooming keeps the bottom of a linear y axis in place (#715).

## Known issues
- **HiDPI on Windows:** a few items are still unchecked or off at 150-200 %: the NWChem code icon, the Dipole table's value row, a scrollbar in the Basis Set Tool, the Feedback row (#670).
- **Windows:** the first start after installing takes about a minute on slower computers, with progress shown (#444).
- **GROMACS, CP2K and Quantum ESPRESSO** remain experimental. **The MO diagram** is experimental.
