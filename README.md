# ECCE

[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.23281771.svg)](https://doi.org/10.5281/zenodo.23281771)

The Extensible Computational Chemistry Environment (ECCE, pronounced
"etch-ā"; *ecce* is Latin for "behold") is a graphical environment for computational chemistry: build a
molecule, set up a calculation, run it on your own computer or a cluster,
and look at the results (orbitals, densities, vibrations, geometry
optimisations), with every calculation kept in one place.

It works with **ORCA, Gaussian, NWChem and MOPAC**, and experimentally with
**GROMACS, CP2K and Quantum ESPRESSO**; ECCE 9 also includes
ECCE-QM, a small built-in Hartree–Fock/DFT program that needs
nothing else installed. ECCE was developed at Pacific Northwest National
Laboratory, which stopped supporting it around 2017; it is maintained
here since, with PNNL's blessing.

<a href="docs/images/organizer.png"><img src="docs/images/organizer.png" width="384" alt="The Organizer: projects and calculations, with a completed ORCA calculation selected"></a>
<a href="docs/images/viewer.png"><img src="docs/images/viewer.png" width="420" alt="The viewer, showing a molecular orbital of benzene"></a>

More pictures and the full feature list: [ECCE overview](docs/OVERVIEW.md).

## Get ECCE

Download from the [releases page](https://github.com/FriendsofECCE/ECCE/releases).

* **ECCE 9** (latest
  [9.4.2](https://github.com/FriendsofECCE/ECCE/releases/tag/v9.4.2)),
  for Linux, macOS and Windows: the recommended version. The macOS and
  Windows clients are experimental.
* **ECCE 8.18.x** (latest
  [8.18.14](https://github.com/FriendsofECCE/ECCE/releases/tag/v8.18.14)),
  Linux only: the older line, which still gets bug fixes.

| System | File | Install |
|---|---|---|
| Debian 13, Ubuntu 24.04 | `ecce-client_…deb`, `ecce-server_…deb` | `sudo apt install ./ecce-client_*.deb ./ecce-server_*.deb` |
| RHEL/Rocky 9, Fedora | `ecce-client-…rpm`, `ecce-server-…rpm` | `sudo dnf install ./ecce-*.rpm` (on RHEL/Rocky first `sudo dnf install epel-release`) |
| macOS | `ECCE-…-arm64.dmg` (Apple silicon) or `…-x86_64.dmg` (Intel) | open it and drag ECCE to Applications; macOS refuses it at first: on macOS 15 and later open it once, then System Settings → Privacy & Security → Open Anyway (macOS 14 and earlier: right-click → Open), or run `xattr -dr com.apple.quarantine /Applications/ECCE.app` |
| Windows 10/11 | `ECCE-windows-….msi` | double-click; at the SmartScreen warning choose More info → Run anyway |

Then start `ecce` (on macOS and Windows: ECCE in Applications or the
Start menu). Details for each system: [Installing ECCE](docs/INSTALLING.md).

## Coming in 9.4.3

- Details dialogs no longer show internal setting names as tooltips.
- Density Map: zooming draws a coarse map at once and the full one when
  the wheel rests, without a dialog.
- Ubuntu 26.04: the package lets ECCE's message broker run under Ubuntu's
  AppArmor profile for mosquitto.
- ORCA: an integration grid choice (DefGrid1/2/3) in Theory Details.
- ORCA: a geometry optimisation of a single atom runs as a single point
  (with frequencies) instead of stopping without an energy.
- Saving in the Calculation Editor asks before regenerating an input file
  that was edited by hand (Keep my input is the default).
- CP2K: an outer SCF loop (&OUTER_SCF) with the OT minimiser, on by default,
  with the inner iteration limit of a new calculation set to 30.
- ORCA and Gaussian 16: a custom solvent given by its dielectric constant and
  refractive index.
- Launch Info in the summary: a Job ID row and a Cost row (cores × wall time
  in CPU hours).
- Tools > Allocations lists each cluster account of your machine profiles and
  how much of it is used.
- Comp. Date is when the job ended, also after a later Reconnect.
- A job the queue system ends (out of memory, wall time reached, node
  failure) is Incomplete instead of System Error, and the reason names the
  cause. A new help page, Run states, says what each state means.
- Run Mgmt > Re-Parse Outputs reads a finished calculation's stored outputs
  again with the current parsers, without logging in to the machine.
- Launch Selected leaves the Organizer usable while a batch is submitted,
  with a Stop button.
- Density Map: a diatomic starts on the molecular plane through its axis.
- General Chemistry: argon at 40, 100, 130 and 200 K (solid, liquid, liquid
  near its critical point, gas) under Gases, liquids and solids.
- The input check counts CP2K atoms with a kind label (such as Nb_e).
- Register Machines is narrower: Vendor, Model and Processor are no longer shown.
- Dipole Moment: where the atomic charges point against the dipole moment
  (carbon monoxide), the arrow stays between the atoms and the text says so.
- Theory Details and Runtype Details dialogs of every code can always be resized; their
  buttons stay in view on a small screen and the settings scroll.

## First steps

The same pages are in ECCE under **Help**. To see it done first, the
[chem4eva YouTube channel](https://www.youtube.com/@chem4eva) has short
videos with subtitles: installing ECCE, a first calculation with ECCE-QM
(nothing else to install) and a first ORCA calculation.

1. [Installation](help/src/installation.md): what you see the first time
   ECCE starts, and where your work is kept.
2. [Your first calculation](help/src/first-calculation.md): build, run and
   look at a calculation.
3. Running codes [on Linux](help/src/running-codes-on-linux.md),
   [on Windows](help/src/running-codes-on-windows.md) or
   [on macOS](help/src/running-codes-on-macos.md): installing NWChem, MOPAC,
   ORCA and the others, and telling ECCE where they are.
4. [Importing a structure](help/src/importing-a-structure.md) or
   [a calculation](help/src/importing-a-calculation.md) that ran elsewhere.

## Getting help and reporting a problem

Questions, and how you use ECCE, are welcome in
[Discussions](https://github.com/FriendsofECCE/ECCE/discussions).
Run the session that goes wrong with `ecce --bug`. When it ends, ECCE
collects its logs into `~/ecce-bug-<time>.zip` (no passwords, but host
names, user names and paths). Attach that to a
[new issue](https://github.com/FriendsofECCE/ECCE/issues/new), with what
you did and what you expected.

## For administrators and developers

* [Getting started](GETTING_STARTED.md): building, packaging, a central
  server for a group or class, TLS, the other deployment modes, upgrading
  from 8.x.
* [Contributing](CONTRIBUTING.md): building from source, branches and
  releases, the roadmap, and how to help.
* [Changes](CHANGELOG.md): what is new in 9.0 and in 8.x, and the release
  history.
* The [wiki](https://github.com/FriendsofECCE/ECCE/wiki): where things are
  in the code, and the current work plan.
* [Accessibility](ACCESSIBILITY.md): what ECCE does for keyboard, screen
  reader and low-vision use, and how to report a barrier.

## Citing ECCE

Please cite the DOI for all versions,
[10.5281/zenodo.23281771](https://doi.org/10.5281/zenodo.23281771); each
release also has its own DOI there. GitHub's "Cite this repository" button
gives the reference in APA and BibTeX form.

## License

ECCE is free software under the Educational Community License 2.0; see
[`LICENSE`](LICENSE). For the use of the name, see [`TRADEMARKS.md`](TRADEMARKS.md).
