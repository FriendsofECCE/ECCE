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
it was built on were over a decade out of date. This release doesn't add
new end-user features; it's a from-scratch modernization that keeps the
same application running on current software, replacing years-old
bundled dependencies with current, distro-maintained ones:

* **One-command install.** A single package installs everything, instead
  of the old multi-step manual setup.
* **Build system**: CMake/CPack replaces the old `build_ecce`/
  recursive-make workflow.
* **GUI toolkit**: wxWidgets 2.8.12 → 3.2.8, running on GTK3 instead of
  GTK2.
* **XML library**: Xerces-C 2.8.0 → 3.2.4.
* **OpenGL/Mesa**: a 2006-era bundled Mesa 6.5.3 → the system's current
  Mesa (25.0.7).
* **Language runtime**: Python 2 → Python 3 (3.13) for the helper GUI
  scripts.
* **Messaging**: the JMS broker moved from a bundled ActiveMQ 5.1.0
  (2008) to Debian's packaged ActiveMQ 5.17.6.
* **Data server**: moved from a vendored Apache httpd 2.2.25 build to
  Debian's packaged Apache 2.4.68.
* **Target platform**: Debian 13 ("trixie"), instead of a decade-old
  reference distro.

Porting a ~1200-file codebase across two major GUI-toolkit versions and a
completely different build system inevitably introduced its own new bugs
along the way — those were found and fixed too, but as stabilization work
to reach parity with the previous release, not as new value on top of it.

The full, detailed history of what was fixed and why lives in `CLAUDE.md`
and `ECCE_modernization_status.md`, if you want the technical story behind
any particular change.

## Installation and getting started

This covers a clean install on Debian 13 ("trixie") through to your first
login. (Other distributions may work but aren't currently tested.)

### 1. Check out and build

```
git clone https://github.com/FriendsofECCE/ECCE.git
cd ECCE
mkdir -p build-cmake && cd build-cmake
cmake ..
ninja
```

### 2. Package

```
cpack -G DEB
```

This produces `ecce_<version>_amd64.deb` in `build-cmake/`.

### 3. Install

Either install the package you just built, or skip steps 1-2 entirely and
download a prebuilt `.deb` from the
[Releases page](https://github.com/FriendsofECCE/ECCE/releases):

```
sudo apt-get install -y apache2 apache2-utils   # data server dependency
sudo dpkg -i ecce_<version>_amd64.deb
sudo apt-get install -f                         # pulls in any remaining dependencies
```

This installs to `/opt/ecce` and puts the apps on your `PATH` as
`ecce-<app>` — e.g. `ecce-gateway`, `ecce-organizer`, `ecce-builder`,
`ecce-pertable` — runnable by name, no environment setup required.

### 4. Create your account

ECCE needs a client and a server side even when both run on the same
machine. The background services start automatically the first time you
launch an app, but you need to create a login once, up front:

```
ecce-dataserver-start          # if not already running
ecce-dataserver-adduser        # interactive: prompts for name, username, password
```

Use a username matching your Linux username — that's what the login
dialog defaults to.

### 5. Start ECCE

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
  clusters, and supercomputers, via PBS, LSF, Slurm, Moab, SGE,
  LoadLeveler, and Maui Scheduler queue management.
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
