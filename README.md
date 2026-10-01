# ECCE

The Extensible Computational Chemistry Environment (ECCE, pronounced
"etch-ā") is a graphical user interface, scientific visualization toolkit,
and data management framework for setting up, running, and analyzing
computational chemistry calculations.

PNNL/EMSL stopped supporting ECCE around 2017, so we forked the source
(with their blessing) and maintain it here.

ECCE compiles and runs on modern Linux systems again — tested primarily
on Debian 13, and also on Ubuntu, Fedora and Rocky Linux. That's why
we're at version 8: reaching it took a real modernization of the build
system and every major dependency, not just a recompile, on top of
bringing the application itself back to life.

## How ECCE is organised

![ECCE has three parts: the client (the windows you use), the ECCE server (a data server that stores your work and a message broker), and the compute machines where calculations run.](docs/images/ecce-architecture.svg)

The **ECCE server** stores your projects and results; it is not where
calculations run. Calculations run on a **compute machine**, a workstation
or an HPC cluster: the client submits each job there over ssh and stores
the results on the ECCE server. On a single computer all of
this is installed and started for you.

## General features

* **Build molecular models**, or import a structure and work from that.
* **Set up calculations through one interface** for **NWChem, Gaussian
  16, ORCA and MOPAC** — built, submitted, monitored and parsed back,
  each actively tested against the real code. Further codes can be
  registered without changing ECCE itself.
* **Check the input before it goes out**: Verify inspects the
  generated input file before it is submitted.
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

## Screenshots

Click any image for the full-size version.

<a href="docs/images/organizer.png"><img src="docs/images/organizer.png" width="300" alt="The Organizer, with a calculation being set up and launched"></a>

*The Organizer is the front door: calculations on the left, a summary of
the selected one in the middle, and the code's own editor and the
launcher opened from it — here an ORCA job on its way to a machine.*

<a href="docs/images/viewer.png"><img src="docs/images/viewer.png" width="300" alt="The viewer, showing a molecular orbital of benzene"></a>

*The viewer: a calculation's orbitals and energies beside the structure
they were computed for — benzene, from ORCA.*

| | | |
|---|---|---|
| <a href="docs/images/orbital-benzene.png"><img src="docs/images/orbital-benzene.png" width="170" alt="Benzene's highest occupied molecular orbital"></a> | <a href="docs/images/esp-benzene.png"><img src="docs/images/esp-benzene.png" width="170" alt="The electrostatic potential on benzene's surface"></a> | <a href="docs/images/vectors-water.png"><img src="docs/images/vectors-water.png" width="170" alt="A vibrational mode of water, drawn as displacement vectors"></a> |
| Benzene's π HOMO (ORCA) | Electrostatic potential on the surface (ORCA) | A vibrational mode of water, as displacement vectors (Gaussian 16) |

<a href="docs/images/mo-diagram-water.png"><img src="docs/images/mo-diagram-water.png" width="380" alt="A qualitative MO correlation diagram for water"></a>

*A qualitative MO correlation diagram — water's orbitals, from Gaussian
16, against the oxygen on one side and the hydrogens' symmetry orbitals
on the other, with the non-bonding lone pair picked out. The same
diagram is drawn from any of the supported codes. Experimental.*

## Installation

Prebuilt packages for Debian/Ubuntu (`.deb`) and RHEL/Rocky/Fedora
(`.rpm`) are on the
[releases page](https://github.com/FriendsofECCE/ECCE/releases). To
build your own instead, see [Building from source](#building-from-source).

### 1. Install the package

On Debian 13 or Ubuntu, from the directory you downloaded it to:

```
sudo apt install ./ecce_<version>_amd64.deb
```

The `./` matters: it tells apt this is a local file, and apt then pulls
in every dependency itself (Apache, ActiveMQ, wxPython, xterm).

On RHEL, Rocky or Fedora, `sudo dnf install ./ecce-<version>.x86_64.rpm`.
These distributions don't package ActiveMQ, so a machine that runs a
broker needs it installed by hand — see
[Deployment modes](GETTING_STARTED.md#deployment-modes).

ECCE installs to `/opt/ecce` and puts `ecce` and its helper commands
(`ecce-dataserver-adduser`, `ecce-remote-setup`, `ecce-diagnose`, …) on
your `PATH`.

**Windows**: use **WSL2** with a Debian or Ubuntu distribution inside
it, and follow these steps as they are. A native Windows client is on
the roadmap ([#133](https://github.com/FriendsofECCE/ECCE/issues/133),
[#18](https://github.com/FriendsofECCE/ECCE/issues/18)).
**macOS** is not supported yet
([#3](https://github.com/FriendsofECCE/ECCE/issues/3)).

### 2. Create your account

ECCE keeps calculations in a data server, which runs as your own user
on a workstation. Create a login on it once:

```
ecce-dataserver-start
ecce-dataserver-adduser        # prompts for name, username and password
```

Use your Linux username — that's what the login dialog defaults to.

### 3. Start ECCE

```
ecce
```

Log in, and the **Organizer** opens: your calculations on the left, the
selected one in the middle, and the Builder, editors, Launcher and
viewer opened from it. The data server and message broker start by
themselves. Closing the Organizer ends the session.

`ecce --help` lists the options; they are described in
[GETTING_STARTED.md](GETTING_STARTED.md#ecce-command-line-options).
Organizer → Edit → Preferences chooses the editor, terminal and web
browser ECCE opens.

### 4. Register a compute machine

A calculation runs on a registered machine — your own workstation is
the simplest. In the Organizer, open **Tools → Register Machines…**:

* **Machine**: the real hostname (run `hostname` to find it); **Name**:
  anything that identifies it to you. Vendor, model and processor don't
  matter. Set the number of processors, and nodes to 1.
* **Each code** needs the full path to its executable, for example
  `/usr/bin/nwchem` (Debian's NWChem), `/opt/gaussian/g16/g16`, or
  `/opt/orca/<version>/orca`. `which` finds the rest, e.g. `which perl`.

SSH has been tested and works for talking to the machine.

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

Queues and submit scripts for a cluster are described in
[GETTING_STARTED.md](GETTING_STARTED.md#describing-the-queues-on-your-own-cluster).

## Deployment modes

ECCE is a client and a server even on one workstation. How those are
shared is up to the site; the three modes are set up step by step in
[GETTING_STARTED.md](GETTING_STARTED.md#deployment-modes).

1. **Everything local** (the default). Each user's session starts their
   own data server and broker. Nothing to configure.
2. **A central server** for a group or a class. One account on the
   server holds everyone's calculations and the shared libraries; users
   elsewhere connect to it. On the server, as that account:

   ```
   ecce-remote-setup --server all     # mark it as the server; listen on every interface
   ecce-dataserver-start && ecce-gateway-start
   ecce-dataserver-adduser            # once per user
   ```

   On each client machine, `sudo ecce-remote-setup <server-host>`, which
   also copies the server's registered machine list; users then run
   `ecce -remote`. Plain `ecce` still runs a local session, so one
   installation can do both.
3. **One shared broker on an app server** with many users logged in, run
   by systemd instead of one broker per user (`sudo ecce-broker-setup`).

**Before putting a server on a network**: the broker has no
authentication, and the data server uses HTTP Basic over plain HTTP, so
passwords cross the network base64-encoded rather than encrypted. That
keeps users' data apart on a trusted network, and is not fine across an
untrusted one — keep the ports on loopback and use ssh tunnels, or
firewall them (#138).

## Reporting a problem

Run the session that goes wrong with `--bug`:

```
ecce --bug
```

It turns diagnostic logging on, and when the session ends it collects
ECCE's logs, the service logs and `ecce-diagnose` output into
`~/ecce-bug-<time>.zip` (a `.tar.gz` if `zip` isn't installed). The
archive holds no passwords, but it does hold host names, user names
and paths. Attach that to a
[new issue](https://github.com/FriendsofECCE/ECCE/issues/new), with what
you did and what you expected. For a job that failed, `ecce-diagnose`
on its own gathers the run directories of your most recent jobs.

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

### Release history

Full notes, and the packages, are on the
[releases page](https://github.com/FriendsofECCE/ECCE/releases); older
releases, and why each fix was made, are in `docs/HISTORY.md`.

<!-- Keep the last four minor releases here, with all their patch
     releases; drop the oldest minor when a new one is added. Everything
     older is on the releases page. -->

- **v8.18.3** — passwords no longer pass through the message broker, and
  ECCE's local message link accepts only its own session (#194); a
  desktop menu entry (#211).
- **v8.18.2** — a job that finishes while its monitor is restarting is no
  longer stored as killed.
- **v8.18.1** — a fresh calculation's output file is no longer sometimes
  named "Outputs", which lost MOPAC's energies and geometries (#207).
- **v8.18.0** — experimental built-in ssh (Edit > Preferences): commands,
  file copies and job monitoring without a shell session, a host-key
  dialog (#204); telnet, Globus and rsh removed; Machine Registration
  crash and Name-field fixes; About ECCE dialog (#168).
- **v8.17.4** — cluster job monitoring: a monitor whose connection drops
  exits, and a login node killing it no longer ends monitoring (#205).
- **v8.17.3** — RHEL 9: the data server starts (#193); Tail, Final Edit and
  Open Shell work with bash (#200). First release with native Ubuntu and
  RHEL packages.
- **v8.17.2** — the data server starts on RHEL (#193).
- **v8.17.1** — an RPM built on RHEL installs there (#199).
- **v8.17.0** — central servers for a class: the server's machine list
  reaches each client, classes are created from a file (#188); final-
  geometry orbitals after an NWChem optimisation (#198); MO diagram on
  correct ORCA and Gaussian coefficients (#163, #170).
- **v8.16.7** — Preferences is back; `ecce --bug` collects a bug report.
- **v8.16.6** — deployment modes for teaching labs, including a shared
  system broker (#191).
- **v8.16.5** — closing the Organizer ends the session (#185); first-run
  machine registration works (#188).
- **v8.16.4** — job monitoring works on Ubuntu and with bash (#143, #69).
- **v8.16.3** — partial fix for the Wayland login-dialog freeze (#120).
- **v8.16.2** — fixes a gateway crash when the login dialog is closed.
- **v8.16.1** — `ecce -remote` works again.
- **v8.16.0** — the Launcher and Organizer say why a job failed.
- **v8.15.0** — Verify: the input file is checked before submission.
- **v8.14.0** — MO correlation diagrams (experimental); RPM packages.

## Roadmap

No dates — this is a small effort. The order below is the estimated order
of work, not a schedule. Current work is tracked in the
[issue tracker](https://github.com/FriendsofECCE/ECCE/issues); the 9.x
plan as a whole is on [#186](https://github.com/FriendsofECCE/ECCE/issues/186).

**8.x** receives bug fixes only, as 8.18.x patch releases.

**9.0** (previews: the 9.0.0-alpha releases):

* **Separate client and server packages**: `ecce-client` for the machines
  people sit at, `ecce-server` for the machine that holds the data
  ([#186](https://github.com/FriendsofECCE/ECCE/issues/186)).
* **Jobs launched without an interactive shell**: commands run directly
  on this machine and over ssh elsewhere (libssh, or the `ssh` command
  for hosts that share connections or are reached through a jump host),
  and job scripts are POSIX sh, so csh is no longer required ([#204](https://github.com/FriendsofECCE/ECCE/issues/204)).
* **HTCondor as a supported queue manager** ([#105](https://github.com/FriendsofECCE/ECCE/issues/105)).
* **Broker authentication**, so that the users of a shared server cannot
  act in each other's name ([#194](https://github.com/FriendsofECCE/ECCE/issues/194)). It is decided first whether ECCE
  moves from ActiveMQ to an MQTT broker (Mosquitto), which would also
  remove Java from the client ([#213](https://github.com/FriendsofECCE/ECCE/issues/213)).

**After 9.0, in estimated order:**

1. **A complete queue editor**: discovering a cluster's queues, job-script
   settings, a preview of the job script and a dry-run test ([#212](https://github.com/FriendsofECCE/ECCE/issues/212)).
2. **A modernised look**: platform-native colours and spacing, current
   icons, consistent plots ([#210](https://github.com/FriendsofECCE/ECCE/issues/210)).
3. **Coin3D in place of the vendored SGI Open Inventor** in the 3D viewer
   ([#166](https://github.com/FriendsofECCE/ECCE/issues/166)), followed by an updated look for the viewer.
4. **Groundwork for native clients**: a session identifier in place of
   `$DISPLAY`, a job store without the X Toolkit, bundled Perl and Python
   ([#186](https://github.com/FriendsofECCE/ECCE/issues/186)).
5. **Native macOS and Windows clients**, macOS first ([#133](https://github.com/FriendsofECCE/ECCE/issues/133)).

**Also planned, not yet placed in the order:**

* **Deeper coverage of the codes already supported** — ORCA, Gaussian 16
  and MOPAC each still have options reachable in the code but not from
  the interface.
* **Finishing Quantum ESPRESSO** and **registering GROMACS** ([#106](https://github.com/FriendsofECCE/ECCE/issues/106)).
* **MO correlation diagrams for coordination complexes** ([#162](https://github.com/FriendsofECCE/ECCE/issues/162)); the
  diagrams are released as experimental.

**Under consideration**

* **Jobs monitored by the ECCE server rather than the client** ([#208](https://github.com/FriendsofECCE/ECCE/issues/208)).
* **A standalone MO diagram tool**, usable without the rest of ECCE and
  without requiring computational output — for teaching use.

## Building from source

CI (`.github/workflows/build.yml`) builds every push on Debian, Ubuntu,
Fedora and Rocky Linux. On Debian 13:

```
sudo apt-get install -y \
  build-essential gfortran cmake ninja-build \
  libwxgtk3.2-dev libxerces-c-dev libgl-dev libglu1-mesa-dev \
  libgtk-3-dev libx11-dev libice-dev libxt-dev libjpeg-dev \
  default-jdk ant activemq git libssh-dev

git clone https://github.com/FriendsofECCE/ECCE.git
cd ECCE
mkdir -p build-cmake && cd build-cmake
cmake -G Ninja ..
ninja
cpack -G DEB
```

That leaves `ecce-client_<version>_amd64.deb` and
`ecce-server_<version>_amd64.deb` in `build-cmake/`; install both for a
standalone machine (`-DECCE_SPLIT_PACKAGES=OFF` builds the single
`ecce` package instead). The build needs CMake 3.16, wxWidgets 3.2 and
libssh at least (this is a wx3.2-only port). For RPMs, install `rpm`
and re-run `cmake .`; `cpack -G RPM` then builds them. Installing
without root and running two builds side by side are in
[GETTING_STARTED.md](GETTING_STARTED.md).

## Branches and releases

* **`main`** is the 9.x line — build from it and file PRs against it.
* **`release/X.Y.Z`** branches cut a patch release: each starts from the
  previous release's tag and takes the fixes from `main` as
  cherry-picks, so a patch carries fixes and nothing else. Releases are
  tagged `vX.Y.Z`. 8.x patch releases (8.18.x) are made this way.
* **`wip/*`** branches hold work in progress until it is merged.
* `archive/*` preserves old branches (`develop`, `stable`, `master`,
  `make`, and `v9`, merged into `main` for 9.0) for history only.

## Contributing

What helps most now is **using ECCE and telling us what breaks**: run
your real calculations with it — on a workstation, a cluster, or a
teaching lab's central server — and report anything that fails, looks
wrong or is confusing, with the `ecce --bug` archive attached (see
[Reporting a problem](#reporting-a-problem)). Issues and pull requests
are welcome on the
[issue tracker](https://github.com/FriendsofECCE/ECCE/issues). `CLAUDE.md` has a
condensed map of the codebase and the bug patterns worth knowing about;
`docs/HISTORY.md` has the full history, dead ends included.

**A note on AI use**: a lot of this modernization effort — including many
commits, and many of the comments you'll see on issues and pull requests —
was done with heavy use of Claude (Anthropic's AI assistant), under the
maintainer's direction and review. We're not hiding this per-comment; this
note is the disclosure. Treat AI-authored analysis the same as you would
any other contributor's: useful, but verify anything you're relying on,
especially root-cause claims in older issue threads.

## License

See [`LICENSE`](LICENSE).
