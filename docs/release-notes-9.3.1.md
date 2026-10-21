**ECCE 9.3.1.** Please report problems as [issues](https://github.com/FriendsofECCE/ECCE/issues).

**Platforms.** Linux (Debian 13; Ubuntu 24.04, RHEL/Rocky 9 and Fedora packages are built on those systems but not tested). Windows 10 and 11. macOS, Apple silicon and Intel: experimental.

9.3.1 fixes problems found while testing 9.3.0, and adds the features that were finished and tested since.

## Fixes
- **Windows: jobs on this computer failed** with "neither an output file nor a status file". The Launcher's default priority started every job through `nice`, which the Windows bundle did not include. The default is now Normal, and `nice` is bundled for Reduced and Low.
- **Windows: slow everywhere on domain-joined computers.** Each shell command the client starts waited several seconds for a user lookup on the network; it now looks users up locally (#386, #387).
- **Builder: clicking atoms stopped working after Save** (Ctrl+S). Clicks then cleared the selection instead of selecting, until ECCE was restarted (#423).
- **Builder: crystals in 74 space groups could not be built** (those with a subscript in the name, such as P2_1/c).
- **Gaussian 16:** Runtype Details for ROMP2 with Vibration closed as soon as it opened.
- **ORCA:** a relaxed scan is run without symmetry, because ORCA stops when the point group changes along the scan.
- **Copy and select:** the rows of the Energies panel and the labels in the Organizer's detail pane can be selected with the mouse; in a table, the field opened by a double-click copies with Ctrl+C (#408).
- **Organizer:** no empty or "?" warnings, and a calculation's failure reason is no longer left on screen when another calculation is selected.

## New
- **UV-vis spectra from TD-DFT** (Gaussian 16, ORCA; #383): the excited states (energy, wavelength, oscillator strength, label and leading transitions), drawn as sticks and a broadened curve in nm or eV, with a table of states and Export .csv.
- **IRC paths** (Gaussian 16, ORCA; #417): the IRC Path panel plots the energy relative to the transition state against the reaction coordinate; a click shows that structure.
- **Relaxed scans for ORCA** (#393), and for both codes the structure of each scan point. **New calculation from this point** in the Relaxed Scan and IRC Path panels starts a calculation from a chosen point (#410).
- **Compare spectra:** Compare... in the Vibrational Frequencies panel adds another calculation's IR or Raman spectrum as a second curve; both go into the .csv (#410).
- **GROMACS** (#410): restraints per chain (unmerged chains get `POSRES_<chain>`), a solute centre for the box, and centre-of-mass pulling on dynamics tasks with a Pull Force panel (force and distance against time, .csv).
- **Toolbars:** the Manipulator and Measure toolbars use the new icons; the Measure tool is called Dihedral Angle (#210).
- **Help pages** checked against the program (#403).
- Smaller changes: the selected vibration's frequency is shown with its description (#400); a Launcher queue without memory or scratch limits shows no range; the Builder zooms out when an edit puts atoms outside the view (#409); stored structures keep their elements and bonds (#262).

## Previews (off unless switched on)
- **Native Details windows** for ORCA, written in C++ instead of Python: `ECCE_NATIVE_DIALOGS=1` (#411).
- **Transition states from two calculations** (Gaussian 16 QST2/QST3, ORCA NEB-TS) are being redesigned: `ECCE_EXPERIMENTAL_TS=1` (#419).
- **A new periodic table** for the Builder: `ECCE_NEW_PERTABLE=1` (#222).

## Known issues
- **Relaxed Scan panel:** selecting a row in the table does not move the plot or the viewer, and New calculation from this point gives no message when it has made the calculation.
- **GROMACS, CP2K and Quantum ESPRESSO** remain experimental. **The MO diagram** is experimental.
