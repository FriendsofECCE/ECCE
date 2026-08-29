# ECCE

The Extensible Computational Chemistry Environment (ECCE, pronounced
"etch-ā") is a graphical user interface, scientific visualization toolkit,
and data management framework for setting up, running, and analyzing
computational chemistry calculations.

PNNL/EMSL stopped supporting ECCE, so we forked the source and maintain it
here.

**Status: heading toward a first alpha/beta release.**

## What's new in this release

ECCE hadn't run on a current Linux system in years — the underlying tools
it was built on are over a decade out of date. This release brings the
same application back to life on modern Debian, with a long list of bugs
fixed along the way. Highlights:

* **It runs on current Linux again.** The whole application has been
  carried forward onto up-to-date system libraries and compilers, so it
  actually builds and runs on a current Debian system rather than
  requiring museum-piece software.
* **One-command install.** A single package installs everything, instead
  of the old multi-step manual setup.
* **Much more stable.** Dozens of crashes and freezes were found and
  fixed — on startup, opening dialogs, building molecules, saving jobs,
  and more.
* **Smooth 3D viewer.** Rotating and zooming a molecule used to stutter,
  jump, or just not update — that's fixed.
* **Molecule builder works properly again.** The periodic table/element
  picker, atom selection, and thumbnail generation all had real bugs that
  are now fixed.
* **Basis sets work correctly.** Selecting a basis set and saving a job
  now reliably produces correct input files, including basis sets that
  previously failed silently.
* **Local help works offline.** No network access or external service
  needed.
* **Jobs run start to finish.** Machine registration, job submission, and
  job monitoring were all broken in various ways and are now fixed —
  calculations run to completion.

The full, detailed history of what was fixed and why lives in `CLAUDE.md`
and `ECCE_modernization_status.md`, if you want the technical story behind
any particular change.

## Installation and getting started

This covers a clean install on Debian 13 ("trixie") through to your first
login. (Other distributions may work but aren't currently tested.)

### 1. Build

```
mkdir -p build-cmake && cd build-cmake
cmake ..
ninja
```

### 2. Package and install

```
cpack -G DEB
sudo apt-get install -y apache2 apache2-utils   # data server dependency
sudo dpkg -i ecce_<version>_amd64.deb
sudo apt-get install -f                         # pulls in any remaining dependencies
```

This installs to `/opt/ecce` and puts the apps on your `PATH` as
`ecce-<app>` — e.g. `ecce-gateway`, `ecce-organizer`, `ecce-builder`,
`ecce-pertable` — runnable by name, no environment setup required.

### 3. Create your account

ECCE needs a client and a server side even when both run on the same
machine. The background services start automatically the first time you
launch an app, but you need to create a login once, up front:

```
ecce-dataserver-start          # if not already running
ecce-dataserver-adduser        # interactive: prompts for name, username, password
```

Use a username matching your Linux username — that's what the login
dialog defaults to.

### 4. Log in

```
ecce-gateway
```

This opens ECCE's main toolbar. Log in with the username/password you just
created. From there you can open the other tools — Organizer, Builder,
Periodic Table, and so on.

### Troubleshooting

See [`GETTING_STARTED.md`](GETTING_STARTED.md) for more detail on each of
these steps, known rough edges, and troubleshooting tips if something
doesn't come up cleanly.

## Registering a compute machine

*(To be added — covers registering the clusters/workstations you'll
submit calculations to.)*

## Branches and releases

`main` is the only active branch — build and file PRs against it. Older
branches (`develop`, `stable`, `master`, `make`) have been consolidated
into `main` and preserved as `archive/*` for history; there's no reason to
branch from or compare against them going forward. Releases are marked
with tags, not separate release branches.

## General features

* Building molecular models.
* A graphical user interface to a broad range of electronic structure
  theory types. Supported codes include NWChem, GAMESS-UK, Gaussian 03,
  Gaussian 09, Gaussian 16, and Amica; other codes can be registered based
  on user requirements.
* A graphical user interface for basis set selection.
* Remote submission of calculations to Unix/Linux workstations, Linux
  clusters, and supercomputers, via PBS, LSF, NQE/NQS, LoadLeveler, and
  Maui Scheduler queue management.
* Three-dimensional visualization and graphical display of molecular data
  and properties, both while jobs are running and after completion.
  Molecular orbitals and vibrational frequencies are among the properties
  displayed.
* Importing results from NWChem and Gaussian calculations run outside of
  the ECCE environment.

## Contributing

Issues and pull requests are welcome — see the
[issue tracker](https://github.com/FriendsofECCE/ECCE/issues). If you hit
a crash or a build failure, a lot of prior investigation may already be
recorded in `CLAUDE.md`; worth a search before filing something new.

## License

See [`LICENSE`](LICENSE).
