**ECCE 9.3.2.** Please report problems as [issues](https://github.com/FriendsofECCE/ECCE/issues).

**Platforms.** Linux (Debian 13; Ubuntu 24.04, RHEL/Rocky 9 and Fedora packages are built on those systems but not tested). Windows 10 and 11. macOS, Apple silicon and Intel: experimental.

9.3.2 fixes problems found while testing 9.3.1 on Windows and Linux, and changes how bonds are built.

## Changed
- **Bonds have no order in the Builder** (#439). An atom's geometry decides its bonds: trigonal carbons bonded to each other are a double bond, linear ones a triple bond. Every covalent bond is drawn as a single stick; the Build panel's Bond choice offers only Covalent and Metal–π (choosing one enters Bond mode), and the toolbar has no Bond tool. A structure such as ethane with a triple bond can no longer be made. Clean works out the orders it needs from the geometries, notes in the log which bonds it treated as multiple, selects atoms it could not settle, and warns when a bond length ends far from the typical value. Options › Advanced › Show Bond Orders Used by Clean draws them. The help page *Bonds and Clean* has the rule.
- **Example calculations in sets** (#447). The first start imports five calculations (ECCE-QM water and ethanol, MOPAC ammonia, NWChem methane, ORCA benzene); File › Open Example Calculations offers the other sets, starting with Computational (the GROMACS Trp-cage protein). An import shows which calculation it is reading (#444) and takes about half as long (#426).
- **Build panel** one row shorter: ten element buttons, Add H, Del H, Clean, Delete and the periodic table on one row, then Bond and Geometry with their names in full. A new periodic-table icon with all seven periods, in the toolbar and the panel (#438). The toolbar's Geometry tool opens its list below the tool.
  ![Build panel](https://github.com/FriendsofECCE/ECCE/releases/download/v9.3.2/932-build-panel.png)
- **Progress box** (#457): sections SCF, Optimisation and Scan, each with its plot beside it; the Optimisation plot keeps every step of the whole job, so a relaxed scan shows one descent per point, with "Scan: point 4 of 11" (ORCA, Gaussian 16); text and plots wrap on small screens.
  ![Progress box of a relaxed scan](https://github.com/FriendsofECCE/ECCE/releases/download/v9.3.2/932-progress-scan.png)
- **Vibrations** (#443): choosing Animation plays the selected mode at once and Vector stops it; there are no Start and Stop buttons. Space in the 3-D view pauses the animation. The Arrow length slider runs from 0 to 5, the Amplitude slider from 0 to 1.5 (#435).
- **MOPAC:** thermochemistry is on by default for frequency runs; the Settings summary shows only Max. Steps (#441).
- **Launcher:** the local machine offers its real number of cores, and defaults to the physical cores (#436).
- **Calculation Editor:** one Settings layout for every code, with each Details button under its own choice; no Geometry Constraints button (a line warns when the run type ignores constraints). Details buttons are enabled on open (#427). Shorter Quantum ESPRESSO theory labels (#442).

## Fixes
- **Windows: the vibration spectrum was drawn twice and peaks could not be picked** (#445). The arrow-length slider did nothing on Windows (#435).
- **Windows: showing Mulliken charges or MOs of the ammonia example closed the Viewer** (#429).
- **The Launcher no longer freezes** while a launch step waits (#424), nor does MD Prepare.
- **Builder:** a bond made between two molecules was always single; deleting atoms left stubs; Lasso Select did nothing until the mode changed (#432). Box selection from empty space, drawn while dragging (#433). Vibration arrows that end inside an atom are visible (#430). Darker bond and geometry icons, and no black square behind the periodic-table icon (#438, #440).
- **Relaxed Scan and IRC Path panels:** with the Viewer open while the job ran, Play and the step buttons moved the selection but not the structure; the panels now take the structures when they arrive (#460).
- **Atom Table:** coordinates on a symmetry element are exactly zero after Symmetry Find or Sym. Clean, and the table shows six decimals instead of values such as 7.4e-17 (#459).
- **Remove comparison** beside Compare… in the Vibrational Frequencies panel (#425).

## New
- **Multipole moments** (quadrupole, octupole, hexadecapole) drawn as angular surfaces (#428).
- **GROMACS:** Restrain chain boxes on the run page for each chain (#410).

## Previews (off unless switched on)
- **Transition states from two calculations** (#419), with a TS Search button and a Reaction Energies panel: `ECCE_EXPERIMENTAL_TS=1`.
- **Native Details windows** for ORCA: `ECCE_NATIVE_DIALOGS=1` (#411).
- **A new periodic table** for the Builder: `ECCE_NEW_PERTABLE=1` (#222).

## Known issues
- **Windows:** the first start after installing takes about a minute on slower computers, now with progress shown (#444; #448 will shorten it). Repair from the installer pauses for about a minute before its progress page appears (#431).
- **GROMACS, CP2K and Quantum ESPRESSO** remain experimental. **The MO diagram** is experimental.
