# ECCE

The Extensible Computational Chemistry Environment (ECCE, pronounced
"etch-ā") is a graphical user interface, scientific visualization toolkit,
and data management framework for setting up, running, and analyzing
computational chemistry calculations.

PNNL/EMSL stopped supporting ECCE around 2017, so we forked the source
(with their blessing) and maintain it here.

ECCE compiles and runs on modern Linux systems again — tested primarily
on Debian 13, and also on Fedora and Rocky Linux. That's why we're at
version 8: reaching it took a real modernization of the build system and
every major dependency, not just a recompile, on top of bringing the
application itself back to life.

## General features

* **Build molecular models**, or import a structure and work from that.
* **Set up calculations through one interface** for **NWChem, Gaussian
  16, ORCA and MOPAC** — built, submitted, monitored and parsed back,
  each actively tested against the real code. Further codes can be
  registered without changing ECCE itself.
* **Choose basis sets graphically**, with the code's own built-in sets
  used where they match.
* **Submit to workstations, clusters and supercomputers**, through PBS,
  LSF, Slurm, Moab, SGE, LoadLeveler and Maui, or directly via a shell.
* **Watch results arrive while the job is still running** — energies,
  geometry traces and convergence are parsed live, not only at the end.
* **Visualise molecular data in 3-D**: molecular orbitals, electron
  density, electrostatic potential maps, vibrational modes with
  animation, and geometry optimisation traces.
* **Import output from jobs run outside ECCE**, for centres where
  ECCE cannot submit directly.
* **Run one server for a group**, with students or colleagues connecting
  to it as clients.

Gaussian 09 still works but is legacy, and is no longer tested against.
Gaussian 03, Gaussian 98, GAMESS-UK and Amica are retired. Quantum
ESPRESSO and GROMACS are in progress — see the roadmap.

## Screenshots

Click any image for the full-size version.

<a href="docs/images/organizer.png"><img src="docs/images/organizer.png" width="440" alt="The Organizer, with a calculation being set up and launched"></a>
<a href="docs/images/viewer.png"><img src="docs/images/viewer.png" width="440" alt="The viewer, showing a molecular orbital of benzene"></a>

*Left: the Organizer is the front door — calculations on the left, a
summary of the selected one in the middle, and the code's own editor and
the launcher opened from it. Right: the viewer, with a calculation's
orbitals and energies beside the structure they were computed for.*

<a href="docs/images/orbital-benzene.png"><img src="docs/images/orbital-benzene.png" width="250" alt="Benzene's highest occupied molecular orbital"></a>
<a href="docs/images/esp-benzene.png"><img src="docs/images/esp-benzene.png" width="250" alt="The electrostatic potential on benzene's surface"></a>
<a href="docs/images/vectors-water.png"><img src="docs/images/vectors-water.png" width="250" alt="A vibrational mode of water, drawn as displacement vectors"></a>

*Benzene's π HOMO; the electrostatic potential on its surface; a
vibrational mode drawn as displacement vectors.*

<a href="docs/images/mo-diagram-water.png"><img src="docs/images/mo-diagram-water.png" width="560" alt="A qualitative MO correlation diagram for water"></a>

*A qualitative MO correlation diagram — water's orbitals against the
oxygen on one side and the hydrogens' symmetry orbitals on the other,
with the non-bonding lone pair picked out. Experimental.*

## What's new in version 8

ECCE hadn't run on a current Linux system in years — the underlying tools
it was built on were over a decade out of date. Version 8 doesn't add
new end-user features on its own; it's a from-scratch modernization that
keeps the same application running on current software, replacing
years-old bundled dependencies with current, distro-maintained ones:

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

The releases since 8.0.0 continue that stabilization: real calculations
run end to end now (job launch, remote monitoring, basis-set selection,
theory setup), and a long list of crashes and silent failures surfaced by
actually using the application have been root-caused and fixed rather
than papered over.

### Release history

One line each. Full notes, and the packages, are on the
[releases page](https://github.com/FriendsofECCE/ECCE/releases).

- **v8.15.0** — **Verify**: a check on the generated input file before
  it is submitted. The point group you choose now reaches Gaussian.
  Final Edit no longer strips a deck's terminating blank line. Register
  Machines works on a fresh install. MO diagrams gain fragment
  orbitals, chemical groups and a π-only view; still experimental.

- **v8.14.0** — **Qualitative MO correlation diagrams**, experimental.
  "Use symmetry" moves onto the Calculation Editor and defaults on.
  Fixes basis sets being silently corrupted in ORCA and Gaussian decks.
  Gaussian 16 Raman and anharmonic frequencies; RPM packages.

- **v8.13.2** — **Jobs that staged and never ran now run**: the submit
  script was backgrounded with a redirection csh rejects. Affects any
  machine whose `/usr/bin/csh` is tcsh. Present since v8.0.3.

- **v8.13.1** — **Running against a central server works again**:
  `ecce -remote`, and `ecce-remote-setup <host>` to configure a client.
  Fixes helper programs invoked by relative path.

- **v8.13.0** — **Electrostatic potential maps** on the molecular
  surface. 3-D orbitals for MOPAC. Queues configurable inside ECCE.
  **Fixes ORCA p functions declared in the wrong order** — orbitals with
  p character were drawn wrong since v8.0.7, so re-render any you rely
  on — and basis-set values read without their exponent.

- **v8.12.0** — **The Gateway window is gone**: `ecce` opens the Organizer
  directly, which becomes the front door (`ECCE_GATEWAY_WINDOW=1` restores
  it). **ORCA gains implicit solvation** — CPCM and SMD, so every ORCA deck
  before this was gas phase — plus MP2, coupled cluster, double hybrids and
  CHELPG charges. Quantum ESPRESSO gains geometry optimisation. Fixes the
  geometry trace and vibration animation not redrawing, and orphaned
  background services.

- **v8.11.0** — Adds dummy submission for 2FA-blocked clusters, Slurm
  submit directives (previously generating none at all), per-user queue
  configuration, and Machine Browser and Periodic Table on the
  Organizer's Tools menu. Fixes a segfault opening any job with
  vibrational data, and adds dialog and application test suites.
- **v8.10.0** — **ORCA becomes properly usable**: vibrational analysis
  and NMR now work at all, property coverage goes from 9 of 24 parse
  types to all 24, and the dialogs gain RIJCOSX, 15 DFT functionals and
  convergence controls. **Adds MOPAC** as a registered code. Fixes a
  long list of Gaussian and NWChem properties that were silently never
  extracted, and adds a parser regression suite.
- **v8.0.8** — Fixes named-library basis-set import, silently broken for
  every registered code except ORCA; extends ORCA's viewer support and
  fixes its geometry trace never showing more than one step. Also fixes
  a wx3.2-port regression that silently disabled 3-D visualization for
  every code, and converts memory settings to GB throughout.
- **v8.0.7** — Adds ORCA as a registered code (job setup, submission, and
  collection now work end-to-end; property display in the viewer is
  still basic); fixes a version-parsing bug that could silently break
  saving on *any* calculation, not just ORCA's.
- **v8.0.6** — Fixes the data server failing to start for ordinary
  (non-root) users, and a zero-height Gateway window crash on
  multi-monitor setups.
- **v8.0.5** — Fixes "Terminate" not actually killing running jobs,
  several null-pointer crashes (rename/delete/rerun/tree view), and
  per-code memory-unit display in Theory Details; documents two
  unresolved bash-as-local-shell bugs.
- **v8.0.4** — Fixes four Organizer startup crashes, a job-monitoring
  socket crash, and a resource-tree reentrancy crash; adds macOS build
  fixes. Note that building does not work on macOS -- these are preparatory fixes.
- **v8.0.3** — Major reliability pass on remote job submission/
  monitoring over SSH (bash echo/pasting bugs, hung connections, dying
  background jobs), plus more crash fixes and cross-platform
  (macOS/RPM) packaging fixes.
- **v8.0.0-alpha.2** — Second alpha: fixes an atom-mis-selection bug and
  two crash bugs, cleans up default property-panel layout.
- **v8.0.0-alpha.1** — Initial modernization release: ports the
  ~1200-file codebase to current Debian, wxWidgets 3.2/GTK3, CMake,
  Python 3, and current XML/OpenGL/messaging/data-server dependencies.

See the notes on each
[release](https://github.com/FriendsofECCE/ECCE/releases) for specifics.
The full, detailed history of what was fixed and why — including dead
ends, not just the fixes that worked — lives in `docs/HISTORY.md`.
`CLAUDE.md` has the current, condensed map of the codebase and the bug
patterns worth knowing about.

## Roadmap

No dates — this is a small effort, and the order below reflects what is
being worked on rather than a schedule. Current work is tracked in the
[issue tracker](https://github.com/FriendsofECCE/ECCE/issues).

**Now**

* **Qualitative MO correlation diagrams** — central-atom orbitals,
  terminal-atom symmetry orbitals and Mulliken labels, derived from point
  group symmetry rather than drawn by hand. Aimed at teaching as much as
  at analysis. **This will be released as experimental**: what it can
  draw depends on what the calculation reports, and a job run without
  symmetry gives it nothing to work with.
* **Deeper coverage of the codes already supported** — ORCA, Gaussian 16
  and MOPAC each still have options reachable in the code but not from
  the interface.
* **A queue editor worth the name.** Queues can now be configured inside
  ECCE, but the editor is rudimentary.

**Next**

* **Finishing Quantum ESPRESSO**, whose generator and parsers are
  tested against real `pw.x` runs but whose interface coverage is
  incomplete, and **registering GROMACS**, which works but is kept out
  of the menu until the scope question in
  [#106](https://github.com/FriendsofECCE/ECCE/issues/106) is settled.
* **HTCondor**, which needs a different submission model rather than
  another set of submit directives.
* **A tested central-server deployment.** The client/server path is
  supported and documented, but has not been exercised on two machines
  end to end.

**Under consideration**

* **A standalone MO diagram tool**, usable without the rest of ECCE and
  without requiring computational output — for teaching use.
* **macOS and Windows.** What it would take is written up in
  [#133](https://github.com/FriendsofECCE/ECCE/issues/133); nobody is
  working on it.

## Installation and getting started

This covers a clean install on Debian 13 ("trixie") through to your first
login.

### Platform support

CI (see `.github/workflows/build.yml`) builds this on every push against
Debian, Ubuntu, Fedora, and Rocky Linux (a free RHEL rebuild, standing in
for the RHEL family, which needs a subscription for a real CI runner) --
all four currently build clean.

**Windows**: use **WSL2**, not Cygwin and not a native Windows build.
WSL2 runs a real Linux kernel and userland (Ubuntu by default) under
Windows, so the Ubuntu/Debian instructions below apply directly with no
porting needed -- install WSL2, install a Debian or Ubuntu distro inside
it, and follow this guide as-is. Native Windows and Cygwin are both a
much larger undertaking: ECCE's process-launch and credential-handoff
code (`fork()`+`execv()`, named-pipe/FIFO handoff, a remote-shell layer
that assumes a POSIX login shell) is POSIX down to the architecture, not
just the toolkit version -- real porting work, not a packaging exercise.
See [#18](https://github.com/FriendsofECCE/ECCE/issues/18) for the fuller
reasoning.

**macOS**: not supported yet, tracked in
[#3](https://github.com/FriendsofECCE/ECCE/issues/3) -- also in CI (best
effort, allowed to fail) to track progress with a real, current compile
log.

### 1. Install build dependencies

```
sudo apt-get install -y \
  build-essential gfortran cmake ninja-build \
  libwxgtk3.2-dev libxerces-c-dev libgl-dev libglu1-mesa-dev \
  libgtk-3-dev libx11-dev libice-dev libxt-dev libjpeg-dev \
  default-jdk ant activemq git
```

Versions confirmed working, from a real Debian 13 ("trixie") install:
CMake 3.31, wxWidgets 3.2.8, Xerces-C 3.2.4, GTK3 3.24, OpenJDK 21, Ant
1.10. `cmake_minimum_required` in `CMakeLists.txt` sets a hard floor of
CMake 3.16 and wxWidgets 3.2 (older wx won't work — this is a wx3.2-only
port); nothing else pins a specific minimum, but older versions of the
rest haven't been tested.

### 2. Check out and build

```
git clone https://github.com/FriendsofECCE/ECCE.git
cd ECCE
mkdir -p build-cmake && cd build-cmake
cmake -G Ninja ..
ninja
```

The `-G Ninja` matters: without it, `cmake` falls back to its default
generator (Unix Makefiles on Debian), which produces a working build too,
but via `make` instead of the `ninja` command used everywhere else in this
document and in `CLAUDE.md`.

### 3. Package

```
cpack -G DEB
```

This produces `ecce_<version>_amd64.deb` in `build-cmake/`.

To build an RPM as well, install `rpm` first (`sudo apt-get install rpm`
on Debian) and re-run `cmake .` in `build-cmake` so it is detected; then
`cpack` alone produces both, or `cpack -G RPM` just the RPM.

Two things differ on the RPM side. ActiveMQ is not packaged for RHEL,
Rocky or Fedora, so it is a weak dependency rather than a hard one and
has to be installed separately — `ecce-gateway-start` says so if it is
missing. NWChem is likewise weak, since it needs EPEL outside Fedora.

### 4. Install

Either install the package you just built, or skip steps 1-3 entirely and
download a prebuilt `.deb` from the
[Releases page](https://github.com/FriendsofECCE/ECCE/releases) — a
prebuilt package still needs the *runtime* dependencies below, just not
the build-time ones from step 1:

```
sudo apt-get install -y apache2 apache2-utils   # data server dependency
sudo dpkg -i ecce_<version>_amd64.deb
sudo apt-get install -f                         # pulls in any remaining dependencies
```

This installs to `/opt/ecce` and puts the apps on your `PATH` as
`ecce-<app>` — e.g. `ecce-gateway`, `ecce-organizer`, `ecce-builder`,
`ecce-pertable` — runnable by name, no environment setup required.

### 5. Create your account

ECCE needs a client and a server side even when both run on the same
machine. The background services start automatically the first time you
launch an app, but you need to create a login once, up front:

```
ecce-dataserver-start          # if not already running
ecce-dataserver-adduser        # interactive: prompts for name, username, password
```

Use a username matching your Linux username — that's what the login
dialog defaults to.

### 6. Start ECCE

```
ecce
```

This opens ECCE's main toolbar. Log in with the username/password you just
created. From there you can open the other tools — Organizer, Builder,
Periodic Table, and so on.

### Troubleshooting

See [`GETTING_STARTED.md`](GETTING_STARTED.md) for more detail on each of
these steps, known rough edges, and troubleshooting tips if something
doesn't come up cleanly.

## Running against a central server

ECCE can be used two ways. By default each user gets their own data
server and message broker, started automatically — nothing to
configure, and right for a workstation.

The other way is the one ECCE was originally deployed with: **one
server for a whole group**, holding everyone's calculations and the
shared structure and basis-set libraries, with many clients connecting
to it. That suits a teaching machine with a class on it, or several
workstations that should all see the same data.

On each client installation, once, as the owner of the install:

```
ecce-remote-setup chem-server.example.edu
```

That writes `siteconfig/RemoteServer/` and points the message broker at
the same host. Users then run:

```
ecce -remote
```

which uses the central data server and broker and starts neither
locally. Plain `ecce` still runs the per-user servers, so both modes
work from one installation.

Each user needs an account on the central server, created there with
`ecce-dataserver-adduser`.

**Before putting a server on a network**, note that it currently
listens on every interface and authenticates with HTTP Basic over
plain HTTP, so credentials cross the network base64-encoded rather
than encrypted. That is fine inside a trusted network and not fine
across an untrusted one; see issue #138.

## Registering a compute machine

Start `ecce-gateway`, then open Machine Browser. Go to Machine → Register
Machines… and register your new machine.

The simplest case is running everything locally: set Machine to your
machine's real hostname (run `hostname` to find it), and Name to whatever
identifying name you want. Vendor, model, and processor don't matter. Set
the total number of processors to an appropriate value, and nodes to 1.

SSH has been tested and works for communication with the machine.

**Using `localhost` instead of the real hostname**: this also works, but
needs one extra one-time step first. `localhost` resolves to both an
IPv4 and an IPv6 address on most systems, and SSH treats each address as
a separate host identity with its own trusted key — if you've only ever
connected to your machine by its real hostname (or never connected to
`localhost` at all), the very first `localhost` connection SSH tries
might hit an address whose key isn't trusted yet, which fails silently
(no prompt) when ECCE tries it non-interactively. Fix it once, up front,
by running `ssh localhost` in a terminal and accepting the host key
prompt — after that, registering `localhost` in ECCE works fine.

Each computational code needs the full path to its executable. Examples
from testing:

* **Gaussian 16**, installed under `/opt/gaussian/g16`: use
  `/opt/gaussian/g16/g16`.
* **NWChem**, installed from the Debian 13 repos: use `/usr/bin/nwchem`.
* **ORCA**, installed under `/opt/orca/orca_6_1_1_linux_x86-64_shared_openmpi418_nodmrg`: use `/opt/orca/orca_6_1_1_linux_x86-64_shared_openmpi418_nodmrg/orca`.
* **Perl 5**: use `/usr/bin/perl`.

Find the right path for anything else with `which`, e.g. `which perl`.

## Branches and releases

`main` is the only active branch — build and file PRs against it. Older
branches (`develop`, `stable`, `master`, `make`) have been consolidated
into `main` and preserved as `archive/*` for history; there's no reason to
branch from or compare against them going forward. Releases are marked
with tags, not separate release branches.

## Contributing

Issues and pull requests are welcome — see the
[issue tracker](https://github.com/FriendsofECCE/ECCE/issues). If you hit
a crash or a build failure, a lot of prior investigation may already be
recorded in `docs/HISTORY.md`; worth a search before filing something new.

**A note on AI use**: a lot of this modernization effort — including many
commits, and many of the comments you'll see on issues and pull requests —
was done with heavy use of Claude (Anthropic's AI assistant), under the
maintainer's direction and review. We're not hiding this per-comment; this
note is the disclosure. Treat AI-authored analysis the same as you would
any other contributor's: useful, but verify anything you're relying on,
especially root-cause claims in older issue threads.

## License

See [`LICENSE`](LICENSE).
