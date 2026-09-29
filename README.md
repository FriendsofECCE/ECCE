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

Gaussian 09 still works but is legacy, and is no longer tested against.
Gaussian 03, Gaussian 98, GAMESS-UK and Amica are retired. Quantum
ESPRESSO and GROMACS are in progress — see the roadmap.

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
in every dependency itself (Apache, ActiveMQ, wxPython, a csh, xterm).

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

   On each client machine, `sudo ecce-remote-setup <server-host>`; users
   then run `ecce -remote`. Plain `ecce` still runs a local session, so
   one installation can do both.
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

*In progress: **8.17.0**.* The MO diagram rebuilt on correct ORCA and
Gaussian coefficients and Löwdin composition, with metal–ligand
classification (#163, #170, #162); ORCA Raman activities (#173); a
Machine Registration fix for site machines (#104); and ECCE shipped as
two packages, `ecce-client` and `ecce-server`.

- **v8.16.7** — **Teaching-round fixes, Preferences, and `ecce --bug`**:
  a random basis-set failure after using an editor is fixed; Verify is
  the only gate and its red line survives reopening; View Input is
  read-only for every editor; old Megaword memory settings convert to
  GB; linear molecules no longer send `PG=C*V` to Gaussian. Organizer →
  Edit → Preferences is back, with the editor, terminal and browser;
  `ecce --bug` collects a bug report into one zip.

- **v8.16.6** — **Deployment modes for teaching labs** (#191): a shared
  systemd broker for many-user app servers (mode 3), per-user brokers
  stop again at session end, RHEL can use an upstream ActiveMQ; no Quit
  and Stop Server under `-remote` and real connection errors (#190); job
  monitoring waits for a marker, not its command's echo (#143); optional
  split `ecce-client`/`ecce-server` packages.

- **v8.16.5** — **For new installs and central-server labs**: closing
  the Organizer ends the session (#185); first-run machine registration
  works on new accounts, and `ecce -admin`, `-machine`, `-l` and the
  StartMessage notice are back (#188); Register Machines fits small
  screens (#187); perl and xterm are now declared dependencies.

- **v8.16.4** — **Fixes job monitoring on Ubuntu and with bash** (#143,
  #69): tcsh's line editor (Ubuntu's `csh`) and bash's readline mangled
  the long monitor command's echo, so the job store waited forever and
  jobs stayed "submitted" although they ran. The package now declares
  that it needs a csh.

- **v8.16.3** — **Partial fix for the Wayland auth-dialog freeze**
  (#120): a stale window-activation timestamp made Mutter silently
  decline focus for the dialog under XWayland. A deeper, likely
  Mutter-side issue can still leave it unresponsive to the keyboard
  even with focus confirmed correct — not fully resolved.

- **v8.16.2** — **Fixes a gateway segfault** when the authentication
  dialog is closed via its window's own close button instead of
  Cancel: an unsigned underflow handed wx an invalid range, and a
  missing `return` let a failed startup check run again on already
  torn-down state.

- **v8.16.1** — **`ecce -remote` works again**: the central-server
  client no longer aborts at startup. The data server and broker listen
  on loopback unless configured as a central server. One password prompt
  per session; G16 imports without `pop=full` no longer store a broken
  ORBENG.

- **v8.16.0** — **Launcher and Organizer say why a job failed**, in one
  line. A basis-library fault that silently dropped 6-31G\*'s d shell is
  fixed. MO diagrams label orbitals in the full point group (Oh, D6h,
  σ/π for linear molecules); coordination complexes are not right yet.

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

## Roadmap

No dates — this is a small effort, and the order below reflects what is
being worked on rather than a schedule. Current work is tracked in the
[issue tracker](https://github.com/FriendsofECCE/ECCE/issues).

**Now (8.x)**

* **Qualitative MO correlation diagrams**, including coordination
  complexes. **Released as experimental**: what it can draw depends on
  what the calculation reports.
* **Deeper coverage of the codes already supported** — ORCA, Gaussian 16
  and MOPAC each still have options reachable in the code but not from
  the interface.
* **Client and server packages**: `ecce-client` for the machines people
  sit at, `ecce-server` for the machine that holds the data.

**Next (9.x)** — the plan is on
[#186](https://github.com/FriendsofECCE/ECCE/issues/186):

* **Jobs launched without an interactive shell** (libssh, and a direct
  local spawn), which ends the bash and csh problems for good and
  retires the csh requirement.
* **Native macOS and Windows clients**, once the client no longer needs
  its own data server and broker
  ([#133](https://github.com/FriendsofECCE/ECCE/issues/133)). macOS
  needs someone with a Mac to test it.
* **Finishing Quantum ESPRESSO** and **registering GROMACS**
  ([#106](https://github.com/FriendsofECCE/ECCE/issues/106)).
* **HTCondor**, which needs a different submission model rather than
  another set of submit directives.

**Under consideration**

* **A standalone MO diagram tool**, usable without the rest of ECCE and
  without requiring computational output — for teaching use.
* **A queue editor worth the name.** Queues can be configured inside
  ECCE, but the editor is rudimentary.

## Building from source

CI (`.github/workflows/build.yml`) builds every push on Debian, Ubuntu,
Fedora and Rocky Linux. On Debian 13:

```
sudo apt-get install -y \
  build-essential gfortran cmake ninja-build \
  libwxgtk3.2-dev libxerces-c-dev libgl-dev libglu1-mesa-dev \
  libgtk-3-dev libx11-dev libice-dev libxt-dev libjpeg-dev \
  default-jdk ant activemq git

git clone https://github.com/FriendsofECCE/ECCE.git
cd ECCE
mkdir -p build-cmake && cd build-cmake
cmake -G Ninja ..
ninja
cpack -G DEB
```

That leaves `ecce_<version>_amd64.deb` in `build-cmake/`, to install as
in step 1. The build needs CMake 3.16 and wxWidgets 3.2 at least (this
is a wx3.2-only port). For an RPM, install `rpm` and re-run `cmake .`;
`cpack -G RPM` then builds it. Split client/server packages, installing
without root and running two builds side by side are in
[GETTING_STARTED.md](GETTING_STARTED.md).

## Branches and releases

* **`main`** is the 8.x stable line — build from it and file PRs
  against it. Bug fixes land here first.
* **`release/X.Y.Z`** branches cut a patch release: each starts from the
  previous release's tag and takes the fixes from `main` as
  cherry-picks, so a patch carries fixes and nothing else. Releases are
  tagged `vX.Y.Z`.
* **`v9`** is the experimental line for large, structural work (the
  client/server split, the new job transport, native clients). Changes
  that prove themselves there come back to `main` in small pieces, and
  can ship in an 8.x release when they are opt-in.
* `archive/*` preserves the old `develop`, `stable`, `master` and
  `make` branches for history only.

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
