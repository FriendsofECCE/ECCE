# Changes

Full notes, and the packages, are on the [releases page](https://github.com/FriendsofECCE/ECCE/releases).

## What's new in 9.0

9.0.0-alpha.1 to alpha.10 and 9.0.0-beta.1 are previews. 9.x does not interoperate with 8.x;
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

| 9.x (previews) | 8.x (stable) |
|---|---|
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
