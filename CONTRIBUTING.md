# Contributing to ECCE

What helps most now is **using ECCE and telling us what breaks**: run
your real calculations with it — on a workstation, a cluster, or a
teaching lab's central server — and report anything that fails, looks
wrong or is confusing, with the `ecce --bug` archive attached (see
[Reporting a problem](README.md#getting-help-and-reporting-a-problem)). Issues and pull requests
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

## Building from source

CI (`.github/workflows/build.yml`) builds every push on Debian, Ubuntu,
Fedora and Rocky Linux. On Debian 13:

```
sudo apt-get install -y \
  build-essential gfortran cmake ninja-build \
  libwxgtk3.2-dev libxerces-c-dev libgl-dev libglu1-mesa-dev \
  libgtk-3-dev libx11-dev libice-dev libxt-dev libjpeg-dev \
  libmosquitto-dev mosquitto-dev libaprutil1-dev mosquitto git dpkg-dev file libssh-dev \
  libssl-dev python3 libcoin-dev libegl-dev

git clone https://github.com/FriendsofECCE/ECCE.git
cd ECCE                # main is 9.0 development; for 8.x: git checkout stable-8
mkdir -p build-cmake && cd build-cmake
cmake -G Ninja ..
ninja
cpack -G DEB
```

That leaves `ecce-client_<version>_amd64.deb` and
`ecce-server_<version>_amd64.deb` in `build-cmake/`; install both for a
standalone machine (`-DECCE_SPLIT_PACKAGES=OFF` builds the single
`ecce` package instead). The build needs CMake 3.16, wxWidgets 3.2,
libssh, OpenSSL and Coin3D at least (this is a wx3.2-only port;
`-DECCE_USE_COIN=OFF` builds the vendored Inventor core instead, in a
separate build directory). For RPMs, install `rpm`
and re-run `cmake .`; `cpack -G RPM` then builds them. Installing
without root and running two builds side by side are in
[GETTING_STARTED.md](GETTING_STARTED.md).

## Branches and releases

* **`main`** is the 9.x line — build from it and file PRs against it.
* **`stable-8`** is the latest 8.x release (it moves to each new 8.18.x
  tag). Build from it for the stable version; the packages on the
  Releases page are built from the same tags.
* **`release/X.Y.Z`** branches cut a patch release: each starts from the
  previous release's tag and takes the fixes from `main` as
  cherry-picks, so a patch carries fixes and nothing else. Releases are
  tagged `vX.Y.Z`. 8.x patch releases (8.18.x) are made this way.
* **`wip/*`** branches hold work in progress until it is merged.
* `archive/*` preserves old branches (`develop`, `stable`, `master`,
  `make`, and `v9`, merged into `main` for 9.0) for history only.

## Roadmap

No dates — this is a small effort. The order below is the estimated order
of work, not a schedule. Current work is tracked in the
[issue tracker](https://github.com/FriendsofECCE/ECCE/issues); the 9.x
plan as a whole is on [#186](https://github.com/FriendsofECCE/ECCE/issues/186).

**8.x** receives bug fixes only, as 8.18.x patch releases.

**9.0** (previews: the 9.0.0-alpha releases). Done in the previews: the
client and server packages ([#186](https://github.com/FriendsofECCE/ECCE/issues/186)),
jobs without an interactive shell ([#204](https://github.com/FriendsofECCE/ECCE/issues/204)),
HTCondor ([#105](https://github.com/FriendsofECCE/ECCE/issues/105)),
Mosquitto instead of ActiveMQ with broker authentication
([#213](https://github.com/FriendsofECCE/ECCE/issues/213),
[#194](https://github.com/FriendsofECCE/ECCE/issues/194)), local data mode
([#216](https://github.com/FriendsofECCE/ECCE/issues/216)) and the Coin3D
viewer as the default ([#166](https://github.com/FriendsofECCE/ECCE/issues/166)).
Remaining for 9.0: the vendored Inventor core is still built with
`-DECCE_USE_COIN=OFF` and goes after one release, and issues found in the
previews are fixed.

**After 9.0, in estimated order:**

1. **A complete queue editor** ([#212](https://github.com/FriendsofECCE/ECCE/issues/212)):
   the Job script and Queues tabs of Register Machines exist; discovering
   a cluster's queues, a preview of the job script and a dry-run test do
   not.
2. **A modernised look** ([#210](https://github.com/FriendsofECCE/ECCE/issues/210)):
   theme colours and controls are in the 9.0 previews; current icons and
   consistent plots remain, followed by the viewer's own look.
3. **Native macOS and Windows clients** ([#133](https://github.com/FriendsofECCE/ECCE/issues/133)):
   both build and run as previews; a Windows installer and fixes from
   testing on real machines remain.

**Also planned, not yet placed in the order:**

* **Deeper coverage of the codes already supported** — ORCA, Gaussian 16
  and MOPAC each still have options reachable in the code but not from
  the interface.
* **Finishing Quantum ESPRESSO** and **registering GROMACS** ([#106](https://github.com/FriendsofECCE/ECCE/issues/106)); GROMACS MD studies are in as experimental.
* **Supporting CP2K** ([#130](https://github.com/FriendsofECCE/ECCE/issues/130)).
* **MO correlation diagrams for coordination complexes** ([#162](https://github.com/FriendsofECCE/ECCE/issues/162)); the
  diagrams are released as experimental.

**Under consideration**

* **Jobs monitored by the ECCE server rather than the client** ([#208](https://github.com/FriendsofECCE/ECCE/issues/208)).
* **A standalone MO diagram tool**, usable without the rest of ECCE and
  without requiring computational output — for teaching use.
