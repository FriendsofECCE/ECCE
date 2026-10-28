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
| **v9.4.2** — A run that stops without converging ends Unsuccessful (ORCA, MOPAC, CP2K, Quantum ESPRESSO, ECCE-QM; #614); Return in a message box presses the highlighted button on Linux (#634); a Slurm job cancelled on the machine ends Killed (#636); Compare Energies with sums of calculations, E+ZPE, H and G (#612); allocation profiles per compute machine (#541, #613); request lines per queue (#624); Register Machines Browse on another computer; Launch Selected: Launch on another machine and Reconnect Selected (#601); Progress box: queued jobs (#606), the number of real and imaginary modes (#611), CP2K 2023 optimisations; Density Profile around an atom with progress and Cancel (#629); one login per compute machine after a dropped connection (#599, #631) | **v8.18.14** — A failed job keeps its output and status (#470). |
| **v9.4.1** — macOS: the Viewer no longer hangs on opening (#605); vibrational spectra no longer keep a Freq. factor from an earlier calculation (#604); Tools > Compare Energies, with the ionisation-energy and electron-affinity examples rebuilt around it (#573); ECCE-QM orbitals of every scan and IRC point (#549) and CPCM solvent for every run type but Raman (#317); ORCA dispersion setting (#593); frequency stage in the Progress box (#600); spin density as tables (#608); software OpenGL detected and handled (#566); MD example fixes (#560); dates from the data server read in any language setting (#609) | **v8.18.13** — Builder clicks select atoms again after File > Save (#423). |
| **v9.4.0** — Orbitals: Density Map and Density Profile panels, the MO Diagram docked beside them, localised orbitals (closed and open shell) and the spin density (#215, #532, #533); ECCE-QM transition-state search, IRC, relaxed scans, analytic RHF Hessian, CPCM solvent for energies and improved virtual orbitals (#452, #450, #394); transition states from two calculations always offered, IRC Path panel, playback Delay and Loop, initial Hessian options for Gaussian 16 and ORCA, ORCA OptTS (#419, #535, #537, #557); TD-DFT options and UV-vis HOMO→LUMO labels (#383); vibrational mode composition (#486); example sets by topic — Getting Started, Organic Chemistry, Spectroscopy, General Chemistry with 94 ECCE-QM calculations and MD of gases, liquids and solids with speed and displacement panels (#447, #448, #485, #559); a notes panel in the Calculation Editor (#461); Launch Selected (#492); one login per compute machine on Linux and macOS (#487); faster first start on Windows (#522) | **v8.18.12** — the Viewer no longer crashes opening the MOs of a calculation with many orbitals (parsed results longer than 2048 bytes per line were cut when stored). |
| **v9.3.3** — Bug fixes: a failed job keeps its output, status and progress (#470, #467); GROMACS dodecahedral boxes shown wrapped into the compact cell (#476), Progress plots for minimisation and dynamics (#477), long file names on the Inputs page, defined groups with none defined fall back to System (#482); job start date kept on a monitor restart, times to the second (#463); Builder Select Solvent/Ions/Molecule and styles apply to the selection (#479); Gaussian 16 with PCM/CPCM keeps its properties for larger molecules (#491); XML parse errors show line and column again | **v8.18.11** — NWChem MP2 and coupled-cluster inputs are generated again; constraints survive a fixed atom; frequency-only runs draw vibrations on NWChem's geometry; ORCA and NWChem imports recognised better; no Organizer crash on a second close; no green tint on atoms. |
| **v9.3.2** — No bond orders in the Builder: geometry decides, all covalent bonds drawn single, Clean reports the orders it assumed (#439); example calculations in sets, five at first start with progress shown (#447, #444, #426); Build panel one row shorter with a new periodic-table icon; Progress box plots every step of a scan with 'point k of n' (#457); Relaxed Scan/IRC playback with the Viewer open during the run (#460); exact zeros after symmetrisation (#459); vibration animation without Start/Stop, shorter slider ranges (#443, #435); Windows: spectrum drawn twice and peaks not pickable (#445), ammonia MOs/charges closed the Viewer (#429), installer pages and progress texts (#431); Launcher and MD Prepare no longer freeze (#424); Builder box selection and edit fixes (#432, #433); multipole surfaces (#428); MOPAC thermochemistry on by default (#441) | **v8.18.10** — After Symmetry > Find, the Calculation Editor no longer shows too many atoms and electrons (water as H4O). |
| **v9.3.1** — Windows jobs on this computer failed (no `nice` in the bundle) and the client was slow on domain-joined computers (#386, #387); Builder clicks stopped selecting after Save (#423); 74 space groups with subscripts could not be built; UV-vis spectra from TD-DFT (#383); IRC paths (#417); ORCA relaxed scans and per-point structures with New calculation from this point (#393, #410); Compare spectra; GROMACS per-chain restraints, box centre and centre-of-mass pulling (#410); new Manipulator and Measure icons (#210) | **v8.18.9** — A compute machine's password was sent to the data server, and jobs on machines with password login were not monitored. |
| **v9.3.0** — GROMACS from start to finish: a Prepare task (proteins through `pdb2gmx`, MINFF and ClayFF minerals with the bundled unit cells and atomipy, small molecules), a launch check that warns instead of blocking, one MD Summary panel, trajectories the Builder animates, File › Import GROMACS Run… and a protein example (#262, #334, #336, #363, #374, #375, #397, #398); CP2K as a new code (#329); Quantum ESPRESSO Verify and cluster pseudopotentials; Gaussian 16 relaxed scans (#393, #396); ORCA Duplicate with Final Orbitals (#75); Vibrational Frequencies with mode symmetry, descriptions and CSV export (#367, #392); Register Machines with a Modules line and per-user run directory and account; Gaussian 09 retired from the New Calculation menu; double-click opens a calculation's editor; MO diagram rebuilt around a model-first fragment-orbital drawing, still experimental (#301); lattices from non-right-angled cell parameters were transposed (#398) | **v8.18.8** — ORCA with an explicitly written Dunning or ANO basis set lost functions and gave wrong energies (#239). |
| **v9.2.2** — Viewer: the molecule no longer moves off the screen when the MOs are shown (NVIDIA under XWayland; the depth-peeled transparency copied from a multisampled image); the view no longer zooms in and out while an orbital is computed; the property column keeps its width when MOs is chosen; the Log stays folded and is three lines high when opened | **v8.18.7** — MD Prepare's Orient panel and toolbox labels no longer show garbage. |
| **v9.2.1** — Viewer no longer crashes opening the MOs of a larger ECCE-QM calculation (above about 120 basis functions; ORCA above about 245 MOs): parsed results longer than 2048 bytes per line were cut when stored; the Calculation Editor names def2 basis sets correctly for ECCE-QM (it said Custom) | **v8.18.6** — File dialog filter and typed paths work; import reports an unreadable path instead of crashing; CAR files keep their first atom; Register Machines runs no shell commands on what you type. |
| **v9.2.0** — Quantum ESPRESSO molecular dynamics, DFT+U (HUBBARD card or the pre-7.1 keywords) and dispersion corrections (#329), and a help page on choosing the cutoff, k-points, smearing and box; GROMACS `.gro` files open in the Builder with their box as the cell, and a periodic-molecules option (#262); metal–π bonds for sandwich and half-sandwich complexes (#325); redrawn bond and shape icons and a periodic-table button (#323); Register Machines checks that a program file is the code (#318); the Progress box ends with the final energy (#352); New and Duplicate in a calculation's right-click menu, spaces in names become `_` (#333); Ctrl+S always saves (#338); Builder panes fold and resize properly (#328); Wayland: dialogs keep repainting and typed text shows (#311); macOS: each tool is its own application with the ECCE icon and name, the gateway is out of the Dock, thumbnails save, panes fold, Downloads access (#344, #354); Preferences › Colours: flat or shaded run-state icons | **v8.18.5** — Viewer redraws after every change (#99); ESP surfaces about 30 times faster (#229); Symmetry panel follows the point group; ecce-remote-setup needs curl. |
