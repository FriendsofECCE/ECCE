# Changes

Full notes, and the packages, are on the [releases page](https://github.com/FriendsofECCE/ECCE/releases).

## What's new in 9.0

9.0.0 is the first release of the 9 series (after the previews 9.0.0-alpha.1 to alpha.10,
beta.1 and beta.2); 8.18.x continues with bug fixes only. 9.x does not interoperate with 8.x;
see [Upgrading from 8.x](GETTING_STARTED.md#upgrading-from-8x).

* **Two packages**, `ecce-client` and `ecce-server`
  ([#186](https://github.com/FriendsofECCE/ECCE/issues/186)).
* **Mosquitto replaces ActiveMQ** and the Java relay; no Java runtime is
  needed ([#213](https://github.com/FriendsofECCE/ECCE/issues/213)). On
  one machine each user's broker listens on a private socket in
  `~/.ECCE`. A central server's broker listens on TCP port 8088, accepts
  only the data server's accounts, and lets each user read and send only
  their own messages ([#194](https://github.com/FriendsofECCE/ECCE/issues/194)).
* **TLS for a central server**: HTTPS for the data server and MQTT over
  TLS for the broker, set up with `ecce-remote-setup --tls`
  ([#236](https://github.com/FriendsofECCE/ECCE/issues/236)).
* **Native macOS and Windows clients**, as previews: a `.dmg` for Apple
  silicon and Intel Macs, and a Windows build from CI
  ([#133](https://github.com/FriendsofECCE/ECCE/issues/133)).
* **The 3D viewer is built on Coin3D** from the distribution by default
  ([#166](https://github.com/FriendsofECCE/ECCE/issues/166)): orbital
  surfaces are drawn with accurate transparency (a quick mode is in
  Edit → Preferences), and the Builder has Reset View. The vendored Open
  Inventor core stays available with `-DECCE_USE_COIN=OFF` for one
  release.
* **Local data mode**: a client can keep its projects in a folder instead
  of a data server ([#216](https://github.com/FriendsofECCE/ECCE/issues/216)).
* **Jobs run without an interactive shell**: commands run directly on
  this machine and over ssh elsewhere; job scripts are POSIX sh, and
  `ecce-csh2sh` converts csh in site and user configuration
  ([#204](https://github.com/FriendsofECCE/ECCE/issues/204)). A job is
  shown as killed only when you cancel it from ECCE.
* **HTCondor** as a queue manager ([#105](https://github.com/FriendsofECCE/ECCE/issues/105)).
* **Machine configuration**: a machine's site and user `CONFIG` files are
  merged key by key (`key: -` removes a site value), and
  `GENSUB_EXPLAIN=1` prints where each value came from. A
  `CONFIG.localhost` template is copied to `~/.ECCE` at the first start
  ([#230](https://github.com/FriendsofECCE/ECCE/issues/230)).
* **Register Machines** is a tabbed editor (Machine, Connection, Codes,
  Job script, Queues) that shows where each value comes from and has undo.
* **Builder**: File → New, Open…, Add Structure from File… and Close, and
  an Open structures panel; Save As can write a calculation to any local
  folder.
* **Look** ([#210](https://github.com/FriendsofECCE/ECCE/issues/210)):
  colours and controls follow the GTK theme, light or dark; editors have
  a text Save button; the default text size is 10 point.
* **Images without ImageMagick**: image conversion uses wxWidgets
  ([#231](https://github.com/FriendsofECCE/ECCE/issues/231)).
* **No X Toolkit**: the job store no longer uses Xt, and X11, Xt and EGL
  are linked only where the viewer uses them, on Linux
  ([#232](https://github.com/FriendsofECCE/ECCE/issues/232)).

## What's new in version 8

ECCE hadn't run on a current Linux system in years. Version 8 began as a
from-scratch modernization of everything underneath the application:

* **One-package install**, instead of the old multi-step manual setup.
* **CMake/CPack** instead of the old `build_ecce`/recursive-make build.
* **wxWidgets 3.2 on GTK3**, from 2.8 on GTK2; **Python 3** for the
  helper GUIs; **Xerces-C 3**; the system's current **Mesa**.
* **Distribution-maintained servers**: Debian's ActiveMQ and Apache 2.4,
  instead of 2008-era bundled builds.

Since then the 8.x releases have added what the old ECCE never had:

* **ORCA and MOPAC** as registered codes, with ORCA's solvation, coupled
  cluster, NMR and vibrational analysis, and Gaussian 16 Raman and
  anharmonic frequencies.
* **Verify**, a check of the generated input before it is submitted,
  and a one-line reason in the Launcher and Organizer when a job fails.
* **Electrostatic potential maps** on the molecular surface, and 3-D
  orbitals for MOPAC.
* **Qualitative MO correlation diagrams**, labelled in the molecule's
  point group — experimental.
* **Deployment modes for teaching labs**: a central server with
  `ecce -remote`, or a shared broker for a many-user machine.
* **Preferences** for the editor, terminal and browser, and
  `ecce --bug` for reporting problems.

## Release history

Full notes, and the packages, are on the
[releases page](https://github.com/FriendsofECCE/ECCE/releases); older
releases, and why each fix was made, are in `docs/HISTORY.md`.

<!-- At most ten entries per column, newest first; older releases are on
     the releases page. -->

| 9.x | 8.x (bug fixes) |
|---|---|
| **v9.3.2** — No bond orders in the Builder: geometry decides, all covalent bonds drawn single, Clean reports the orders it assumed (#439); example calculations in sets, five at first start with progress shown (#447, #444, #426); Build panel one row shorter with a new periodic-table icon; Progress box plots every step of a scan with 'point k of n' (#457); Relaxed Scan/IRC playback with the Viewer open during the run (#460); exact zeros after symmetrisation (#459); vibration animation without Start/Stop, shorter slider ranges (#443, #435); Windows: spectrum drawn twice and peaks not pickable (#445), ammonia MOs/charges closed the Viewer (#429), installer pages and progress texts (#431); Launcher and MD Prepare no longer freeze (#424); Builder box selection and edit fixes (#432, #433); multipole surfaces (#428); MOPAC thermochemistry on by default (#441) | |
| **v9.3.1** — Windows jobs on this computer failed (no `nice` in the bundle) and the client was slow on domain-joined computers (#386, #387); Builder clicks stopped selecting after Save (#423); 74 space groups with subscripts could not be built; UV-vis spectra from TD-DFT (#383); IRC paths (#417); ORCA relaxed scans and per-point structures with New calculation from this point (#393, #410); Compare spectra; GROMACS per-chain restraints, box centre and centre-of-mass pulling (#410); new Manipulator and Measure icons (#210) || **v8.18.13** — Builder clicks select atoms again after File > Save (#423). |
| **v9.3.0** — GROMACS from start to finish: a Prepare task (proteins through `pdb2gmx`, MINFF and ClayFF minerals with the bundled unit cells and atomipy, small molecules), a launch check that warns instead of blocking, one MD Summary panel, trajectories the Builder animates, File › Import GROMACS Run… and a protein example (#262, #334, #336, #363, #374, #375, #397, #398); CP2K as a new code (#329); Quantum ESPRESSO Verify and cluster pseudopotentials; Gaussian 16 relaxed scans (#393, #396); ORCA Duplicate with Final Orbitals (#75); Vibrational Frequencies with mode symmetry, descriptions and CSV export (#367, #392); Register Machines with a Modules line and per-user run directory and account; Gaussian 09 retired from the New Calculation menu; double-click opens a calculation's editor; MO diagram rebuilt around a model-first fragment-orbital drawing, still experimental (#301); lattices from non-right-angled cell parameters were transposed (#398) | |
| **v9.2.2** — Viewer: the molecule no longer moves off the screen when the MOs are shown (NVIDIA under XWayland; the depth-peeled transparency copied from a multisampled image); the view no longer zooms in and out while an orbital is computed; the property column keeps its width when MOs is chosen; the Log stays folded and is three lines high when opened | |
| **v9.2.1** — Viewer no longer crashes opening the MOs of a larger ECCE-QM calculation (above about 120 basis functions; ORCA above about 245 MOs): parsed results longer than 2048 bytes per line were cut when stored; the Calculation Editor names def2 basis sets correctly for ECCE-QM (it said Custom) | **v8.18.12** — the Viewer no longer crashes opening the MOs of a calculation with many orbitals (parsed results longer than 2048 bytes per line were cut when stored). |
| **v9.2.0** — Quantum ESPRESSO molecular dynamics, DFT+U (HUBBARD card or the pre-7.1 keywords) and dispersion corrections (#329), and a help page on choosing the cutoff, k-points, smearing and box; GROMACS `.gro` files open in the Builder with their box as the cell, and a periodic-molecules option (#262); metal–π bonds for sandwich and half-sandwich complexes (#325); redrawn bond and shape icons and a periodic-table button (#323); Register Machines checks that a program file is the code (#318); the Progress box ends with the final energy (#352); New and Duplicate in a calculation's right-click menu, spaces in names become `_` (#333); Ctrl+S always saves (#338); Builder panes fold and resize properly (#328); Wayland: dialogs keep repainting and typed text shows (#311); macOS: each tool is its own application with the ECCE icon and name, the gateway is out of the Dock, thumbnails save, panes fold, Downloads access (#344, #354); Preferences › Colours: flat or shaded run-state icons | |
| **v9.1.0** — ECCE-QM computes static polarisabilities and Raman activities with depolarisation ratios (#248; within 0.1 % of ORCA), with an ethanol Raman example; ECCE-QM basis sets 6-31G, 6-31+G*, 6-311G*, aug-cc-pVDZ and cc-pVTZ, and frequency points computed side by side; the Builder opens Quantum ESPRESSO calculations with the Periodic Builder (#300); Builder tool panes resizable (#293); ORCA imports with NumFreq read as frequency runs; macOS says when the wrong package for the processor is installed (#310); fixes from the 9.0.1 test round: the Builder ready to place atoms in an empty structure, vibration arrows in wireframe (#312), ORCA jobs refuse the GNOME screen reader, Verify, Raman kept on a code change, scroll bars shown, typing on Wayland (#311); a start-up splash on every platform; message boxes in the desktop's style; the current logo in the login window; Help › Report a Problem (diagnostics as a zip file, also on Windows); thumbnails for the example calculations; the macOS application icon (#316); Verify marks the block lines of ECCE-QM decks | **v8.18.11** — NWChem MP2 and coupled-cluster inputs are generated again; constraints survive a fixed atom; frequency-only runs draw vibrations on NWChem's geometry; ORCA and NWChem imports recognised better; no Organizer crash on a second close; no green tint on atoms. |
| **v9.0.1** — Bug fixes and small additions: NWChem MP2/CC inputs generate again, open-shell CC through TCE (#297); geometry constraints defined in the Builder (Tools › Geometry Constraints) at any run type, with an atom-order check; NWChem no longer drops bond/angle/torsion constraints silently next to fixed atoms (#306); dummy atoms attach at an open site, or are placed from a selection (Tools › Add Dummy Atom), and ECCE-QM optimises with dummy atoms and constraints (#309); Ctrl+click selection; Forgot password in the login window on this computer's data server (#203); example calculations imported at the first start (#302); MOPAC FORCE progress (#303); no black areas behind the Code Change question (#308); the element selector opens below its button (#304); Ctrl+X/C/V/Z in the Builder's 3-D view; fewer menu redraws on Windows (#296) | |
| **v9.0.0** — First release of the 9 series. Partial charges from every scheme for ORCA, Gaussian 16 (with AIM and NBO 6/7), ECCE-QM (Löwdin, CHELPG, Merz–Kollman), NWChem and MOPAC (ESP); ECCE-QM 2–3.4× faster for DFT; example calculations (File › Open Example Calculations); a Ligands folder in the Structure Library; Progress plots kept after a job and built for older calculations; the per-user data server offers to create the account; Ctrl+click selection; dummy atoms (#298, NWChem and ORCA; ORCA geometry constraints now written); Preferences › Advanced with testing switches (#307); Builder Save enabled after changes and Ctrl+S in the 3-D view; Geometry Constraints dialog shows all columns; imports on Windows (the host is registered as localhost); fixes for an exit crash (#291), Local data delete (#288), window hopping (#275), the MO panel with ESP (#285), C2/N2 s–p mixing in the MO diagram (#287); Windows icons, Launcher units and redraw; help for running codes on Linux | |
| **v9.0.0-beta.2** — ECCE-QM geometry optimisation, frequencies, IR and thermochemistry; job progress in the Organizer for ECCE-QM, ORCA, Gaussian, NWChem, MOPAC, QE and GROMACS, and "running?" when a job is out of contact; Energies grouped, thermochemistry and solvation for G16/NWChem; Jmol element colours (Preferences › Colours); sharper atoms, per-pixel lighting, new icons; ESP colour bar, polarizability ellipsoid; MO diagram fixes (TASOs when the code symmetrised, ECCE-QM diatomics, more character tables, composition on hover); Windows: Builder element buttons, focus, redraw and a faster installer | |
| **v9.0.0-beta.1** — "Store data on this computer" works on Linux; "Connect to a server" prefilled, with this computer's own data server and a broker check; data folder controls greyed out when unused; help pages Running codes on Windows, Importing a structure, Importing a calculation | **v8.18.10** — After Symmetry > Find, the Calculation Editor no longer shows too many atoms and electrons (water as H4O). |
| **v9.0.0-alpha.10** — The start question at every start; "Use this computer instead" goes on in local mode; Windows: no Organizer flicker or focus loss, Calculation Editor opens once, Launcher and #247 layouts, Hill formulas; MPI warning that never blocks a launch; ORCA on one core no longer needs MPI; Symmetry pane no longer over the viewer; Add Hydrogens at typical bond lengths; GROMACS position restraints (#253) | **v8.18.9** — A compute machine's password was sent to the data server, and jobs on machines with password login were not monitored. |
| **v9.0.0-alpha.9** — Windows: user names with a space (#247) and saving to a data server fixed; TLS server chosen at first start; Symmetry > Find no longer shows extra atoms in the Calculation Editor; first start asked on Linux; "Use this computer instead" in the login window; MO diagram built only when opened | **v8.18.8** — ORCA with an explicitly written Dunning or ANO basis set lost functions and gave wrong energies (#239). |
| **v9.0.0-alpha.8** — Windows installer (MSI) and fixes from the first Windows tests; ECCE-QM, a built-in Hartree–Fock/DFT engine; first start asks where data is kept (#240); Python included in the macOS app; Teaching > Diatomics structures; property displays follow the open panel; GROMACS MD studies (experimental) | **v8.18.7** — MD Prepare's Orient panel and toolbox labels no longer show garbage. |
| **v9.0.0-alpha.7** — Builder no longer crashes opening structure files (#238); viewer works on FastX with the Fedora/RHEL packages (#237); ORCA explicit basis sets fixed (#239); MO composition (#161); TLS for the central server (#236); windows fit small screens (#189); first macOS packages (#133) | **v8.18.6** — File dialog filter and typed paths work; import reports an unreadable path instead of crashing; CAR files keep their first atom; Register Machines runs no shell commands on what you type. |
| **v9.0.0-alpha.6** — Builder panels in one column with a choice of layout and a collapse button; jobs followed after logout (#208); spectrum viewer (#214); Register Machines as a tabbed editor with queue discovery and job-script preview (#234, #212); help window (#219); session id (#233); `ecce --local`. | **v8.18.5** — Viewer redraws after every change (#99); ESP surfaces about 30 times faster (#229); Symmetry panel follows the point group; ecce-remote-setup needs curl. |
| **v9.0.0-alpha.4** — 3D viewer on Coin3D (#166): accurate transparency, immediate redraws, Reset View, atom labels again; ESP surfaces about 30 times faster (#229); a CONFIG.localhost for new users (#230). | **v8.18.4** — Viewer property panes fold to their caption bar (#196); a new desktop icon. |
| **v9.0.0-alpha.3** — Mosquitto replaces ActiveMQ and Java (#213, #194); optional local data mode (#216); colours and controls follow the GTK theme (#210). | **v8.18.3** — Passwords no longer pass through the message broker; the local message link accepts only its own session (#194); a desktop menu entry (#211). |
| **v9.0.0-alpha.2** — The scripted shell session is removed: commands run directly or over ssh; csh no longer required; ssh/ftp and sshpass become plain ssh. | **v8.18.2** — A job that finishes while its monitor restarts is no longer stored as killed. |
| **v9.0.0-alpha.1** — Built-in ssh by default, shared ssh connections and a host-key dialog (#204); two packages, ecce-client and ecce-server; job scripts in POSIX sh. | **v8.18.1** — A fresh calculation's output file is no longer named "Outputs", which lost MOPAC's energies and geometries (#207). |
