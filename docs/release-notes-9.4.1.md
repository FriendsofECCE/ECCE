**ECCE 9.4.1.** Please report problems as [issues](https://github.com/FriendsofECCE/ECCE/issues).

**Platforms.** Linux (Debian 13; Ubuntu 24.04, RHEL/Rocky 9 and Fedora packages are built on those systems but not tested). Windows 10 and 11. macOS, Apple silicon and Intel: experimental.

## Fixes
- **macOS: the Viewer could hang on opening** (pinwheel, no window), for every calculation (#605). Two causes, both fixed:
  - After an upgrade, a notice about resetting the window layout opened out of sight and the Viewer waited for it. The layout is still reset; the notice is gone.
  - On some Macs the login information handed from the Organizer to a new Viewer never arrived, and the Viewer waited for it without limit. It now waits at most 10 seconds.
- **Vibrations spectrum: bands at the wrong wavenumber** (#604). The Freq. factor was remembered for every later calculation, so one entry of 0.5 put every band at half its frequency. The factor now starts at 1 each time, the line under the spectrum gives the scaled value next to the frequency when the factor is not 1, and a decimal comma is accepted (0,96).
- **Spin density surface** starts at a level (about 0.003) where the spin of a single unpaired electron shows around the atom; before, it started at 0.05 and nothing was visible for many open-shell molecules.
- **Dates from the data server are read in any language setting** (macOS with a non-English region could fail to read them) (#609).
- **XYZ trajectories with forces:** the force columns are read as forces (they were stored as velocities) (#607).
- **Atom orbital names:** an open shell's split 2p set is no longer named 2p and 3p (#572).
- **Atom numbers count from 1** everywhere they are shown, including the Builder's status line (#582).
- **Progress box plots:** cycle and step axes start at 1 (#596).

## New
- **Tools > Compare Energies** (Organizer): the final energies of two calculations and their difference in Hartree, kJ/mol, kcal/mol and eV, with a note when the code, method or basis set differ (#573).
- **General Chemistry, Ionisation energies and electron affinities:** each cation and anion now has its neutral atom beside it, so ionisation energies and electron affinities follow from Compare Energies (#573).
- **ECCE-QM scans and IRC:** the MOs, MO Diagram, Density Map and Density Profile panels show the orbitals of the point selected in the Relaxed Scan or IRC Path panel (new calculations; #549).
- **ECCE-QM implicit solvent (CPCM)** now works for geometry optimisations, frequencies, transition-state searches, IRC and scans, not only energies (#317).
- **Dipole Moment:** the arrow runs from the negative to the positive charge pole, and the value (μ = … D) is shown in the viewer's upper-right corner (#578, #597).
- **ORCA: Dispersion** (None, D3BJ, D3(0), D4) is its own choice in Theory Details. A calculation that has the dispersion in its functional name (for example PBE0 D3BJ) keeps it when it is opened and saved again (#593).
- **Progress box: frequencies** are shown as soon as that stage starts, with the analytic Hessian's progress or the numerical point (ORCA, ECCE-QM, Gaussian 16, NWChem) (#600).
- **Relaxed Scan panel:** Graph or Table, as in the IRC Path panel (#584).
- **Spin density** is shown as two tables, the singly occupied orbitals and the atomic spin populations, under a note that wraps to the panel's width (#608).
- **Vibrations:** the selected mode is shown in a strip under the spectrum, with its composition.
- **Density Profile** follows atoms picked in the viewer.
- **MO Diagram:** weak correlation lines stay visible (#574).
- **Software OpenGL** (virtual machines, remote desktops) is detected; the viewer then draws its text in a way that works there and starts in the quick transparency mode. Edit > Preferences > Advanced > OpenGL drawing overrides the choice; Help > About and Help > Report a Problem name the renderer (#566).
- **MD examples (General Chemistry):** argon and helium keep their element; a click in a plot shows the nearest stored structure, with a message when it is not at that time; Compare finds runs in topic folders; molecules in rectangular boxes are kept whole (#560).
- **Example sets:** opening a set that is already imported adds the topics and calculations it is missing, without touching existing ones (#571).

## Known issues
- **Windows:** the first start after installing takes about a minute on slower computers, with progress shown (#444).
- **Trajectory playback** of large systems (about 10 000 atoms) is slow (#570).
- **GROMACS, CP2K and Quantum ESPRESSO** remain experimental. **The MO diagram** is experimental.
