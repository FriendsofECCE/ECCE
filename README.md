# ECCE

[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.23281771.svg)](https://doi.org/10.5281/zenodo.23281771)

The Extensible Computational Chemistry Environment (ECCE, pronounced
"etch-ā"; *ecce* is Latin for "behold") is a graphical environment for computational chemistry: build a
molecule, set up a calculation, run it on your own computer or a cluster,
and look at the results (orbitals, densities, vibrations, geometry
optimisations), with every calculation kept in one place.

It works with **NWChem, Gaussian, ORCA and MOPAC**; ECCE 9 also includes
ECCE-QM, a small built-in Hartree–Fock/DFT program that needs
nothing else installed. ECCE was developed at Pacific Northwest National
Laboratory, which stopped supporting it around 2017; it is maintained
here since, with PNNL's blessing.

<a href="docs/images/viewer.png"><img src="docs/images/viewer.png" width="420" alt="The viewer, showing a molecular orbital of benzene"></a>

More pictures and the full feature list: [ECCE overview](docs/OVERVIEW.md).

## Get ECCE

Download from the [releases page](https://github.com/FriendsofECCE/ECCE/releases).

* **ECCE 9** (latest
  [9.1.0](https://github.com/FriendsofECCE/ECCE/releases/tag/v9.1.0)),
  for Linux, macOS and Windows: the recommended version. The macOS and
  Windows clients are experimental.
* **ECCE 8.18.x** (latest
  [8.18.10](https://github.com/FriendsofECCE/ECCE/releases/tag/v8.18.10)),
  Linux only: the older line, which still gets bug fixes.

| System | File | Install |
|---|---|---|
| Debian 13, Ubuntu 24.04 | `ecce-client_…deb`, `ecce-server_…deb` | `sudo apt install ./ecce-client_*.deb ./ecce-server_*.deb` |
| RHEL/Rocky 9, Fedora | `ecce-client-…rpm`, `ecce-server-…rpm` | `sudo dnf install ./ecce-*.rpm` (on RHEL/Rocky first `sudo dnf install epel-release`) |
| macOS | `ECCE-…-arm64.dmg` (Apple silicon) or `…-x86_64.dmg` (Intel) | open it and drag ECCE to Applications |
| Windows 10/11 | `ECCE-windows-….msi` | double-click; at the SmartScreen warning choose More info → Run anyway |

Then start `ecce` (on macOS and Windows: ECCE in Applications or the
Start menu). Details for each system: [Installing ECCE](docs/INSTALLING.md).

## Coming in 9.2.0

Done and being tested, not yet released:

* **Quantum ESPRESSO:** molecular dynamics, DFT+U and dispersion
  corrections, and a help page on choosing the cutoff, k-points, smearing
  and box ([#329](https://github.com/FriendsofECCE/ECCE/issues/329)).
* **GROMACS:** `.gro` files open in the Builder with their box as the
  periodic cell, and a periodic-molecules option
  ([#262](https://github.com/FriendsofECCE/ECCE/issues/262)).
* **Builder:** metal–π bonds (ferrocene, benzene and cyclopentadienyl
  complexes) ([#325](https://github.com/FriendsofECCE/ECCE/issues/325)),
  redrawn bond and shape icons, a periodic-table button
  ([#323](https://github.com/FriendsofECCE/ECCE/issues/323)), Ctrl+S
  always saves ([#338](https://github.com/FriendsofECCE/ECCE/issues/338)),
  panes that fold and resize properly
  ([#328](https://github.com/FriendsofECCE/ECCE/issues/328)).
* **Organizer:** the Progress box ends with the final energy
  ([#352](https://github.com/FriendsofECCE/ECCE/issues/352)); New and
  Duplicate in the right-click menu; spaces in names become `_`
  ([#333](https://github.com/FriendsofECCE/ECCE/issues/333)).
* **Register Machines** checks that a program file really is the code
  ([#318](https://github.com/FriendsofECCE/ECCE/issues/318)).
* **Linux under Wayland:** dialogs and the Builder keep repainting, and
  typed text shows in name prompts
  ([#311](https://github.com/FriendsofECCE/ECCE/issues/311)).
* **In progress:** macOS Dock icons and app names, saving in the Builder on
  macOS, and access to Downloads
  ([#344](https://github.com/FriendsofECCE/ECCE/issues/344)).

## First steps

The same pages are in ECCE under **Help**. To see it done first, watch
[Your first calculation with ECCE-QM](https://youtu.be/HQarL7NbFjk)
(3 minutes, nothing else to install) or
[Your first ORCA calculation with ECCE](https://youtu.be/8NzblPZJl5k)
(4 minutes); both have subtitles.

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
