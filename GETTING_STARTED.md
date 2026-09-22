# Getting started with ECCE (modernized build)

This covers building, installing, and running this fork of ECCE on Debian
13 ("trixie") — from a clean checkout to a working login. It documents
`main`'s CMake/CPack packaging, not the old `build_ecce`/recursive-make
workflow.

CI also builds this clean on Ubuntu, Fedora, and Rocky Linux (RHEL
family) — the steps below apply there too, adjusted for the distro's
package manager. **On Windows, use WSL2** (not Cygwin, not a native
build) — see the README's "Platform support" section for why.

## 1. Install build dependencies

```
sudo apt-get install -y \
  build-essential gfortran cmake ninja-build \
  libwxgtk3.2-dev libxerces-c-dev libgl-dev libglu1-mesa-dev \
  libgtk-3-dev libx11-dev libice-dev libxt-dev libjpeg-dev \
  default-jdk ant git
```

## 2. Build

```
mkdir -p build-cmake && cd build-cmake
cmake -G Ninja ..
ninja
```

`-G Ninja` matters here: plain `cmake ..` falls back to its default
generator (Unix Makefiles on Debian) instead of Ninja, which still builds
but via `make`, not the `ninja` command used everywhere else in this
document.

This produces the 19 GUI apps (`gateway`, `organizer`, `builder`,
`pertable`, ...) plus the CLI apps, all statically linked against the
in-tree libraries.

## 3. Package and install

```
cd build-cmake
cpack -G DEB
sudo apt-get install -y apache2 apache2-utils   # data server dependency
sudo dpkg -i ecce_<version>_amd64.deb
```

The package installs to `/opt/ecce` and drops thin wrapper scripts named
`ecce-<app>` (e.g. `ecce-organizer`, `ecce-builder`,
`ecce-pertable`) onto `/usr/bin`, plus `ecce` itself, which is the one
you actually start (it launches the gateway, which then spawns the apps --
running `ecce-builder` and friends directly skips that setup). No
`ECCE_HOME` sourcing or environment setup required first.

`apache2`/`apache2-utils` are real runtime dependencies (the data server
below runs as a real Apache instance), not just build-time — `dpkg -i` will
fail to configure without them if `apt-get install` wasn't run first.

### Describing the queues on your own cluster

ECCE needs to know a machine's batch queues — their names, processor and
time limits, and which queue manager (PBS, SLURM, Moab, LoadLeveler…) it
runs. There is no GUI for this yet; it is two files.

They used to be readable only from `$ECCE_HOME/siteconfig`, which on a
packaged install is root-owned, so this needed `sudo`. Your own copies in
`~/.ECCE/` now take precedence:

1. `~/.ECCE/Queues` — the registry. Start from
   `/opt/ecce/siteconfig/Queues`:

   ```
   Queues: mycluster

   mycluster|queueMgrName:   SLURM
   mycluster|prefFile:       mycluster.Q
   ```

   `queueMgrName` must be one listed in `/opt/ecce/siteconfig/QueueManagers`.

2. `~/.ECCE/mycluster.Q` — the queues themselves. Start from
   `/opt/ecce/siteconfig/chinook.Q`:

   ```
   Queues:    normal

   normal|minProcessors:   1
   normal|maxProcessors:   256
   normal|runLimit:        1440
   normal|memLimit:        0
   ```

   `runLimit` is in minutes; `memLimit` 0 means no limit.

Note this is an **override, not a merge** — if you create `~/.ECCE/Queues` it
replaces the site file entirely, so copy across any site machines you still
want. Issue #95 tracks doing this properly.

### Running two instances at once

Everything that makes an instance distinct is an environment variable:

| variable | default | what it moves |
| --- | --- | --- |
| `ECCE_REALUSERHOME` | `$HOME` | the whole `.ECCE` state directory: preferences, the data server's document root, JMS port files |
| `ECCE_DATASERVER_PORT` | `8096` | the data server |
| `ECCE_BROKER_PORT` | `8088` | the ActiveMQ broker |

So a completely separate instance, sharing nothing with your normal one, is:

```
export ECCE_REALUSERHOME=/tmp/ecce-scratch
export ECCE_DATASERVER_PORT=8097 ECCE_BROKER_PORT=8089
ecce-dataserver-start && ecce-gateway-start
ecce
```

The per-user copies of `activemq.xml` and `jndi.properties` are resolved
against `ECCE_BROKER_PORT` at start, so the broker and the dispatcher agree
without editing anything installed. The C++ side needs no configuration at
all — it finds the dispatcher through `$ECCE_REALUSERHOME/.ECCE/<host>_<display>`
rather than reading either file.

One thing this does **not** move: a data server you have already registered
in the GUI keeps whatever URL it was added with. A second instance on a
different port needs its server added at that port.

### Installing without root

`sudo dpkg -i` is the right thing for a real install, but it is a poor fit
for development: every time you want to check a C++ change in the running
app, you need a password. The install location is a build option, so you can
put a second copy somewhere writable and skip that entirely:

```
cmake -B build-user -GNinja \
      -DCMAKE_INSTALL_PREFIX=$HOME/.local/ecce \
      -DECCE_HOME_DIR=$HOME/.local/ecce \
      -DECCE_WRAPPER_DESTINATION=$HOME/.local/bin
cmake --build build-user --target install
```

With `$HOME/.local/bin` on your `PATH`, `ecce-builder` and friends then run
that copy. Both options default to `/opt/ecce` and `/usr/bin`, so the `.deb`
and an ordinary build are unaffected.

`tests/apps` can point at it too, which means the GUI suite no longer needs
root either:

```
ECCE_TEST_HOME=$HOME/.local/ecce ECCE_TEST_WRAPPERS=$HOME/.local/bin \
  tests/apps/run_tests.py
```

Both installs share `~/.ECCE`, so they share the data server, the gateway and
your saved calculations. That is usually what you want, but it does mean the
two cannot run at the same time on one display.

## 4. Start the background services

ECCE has always been a client/server app; this fork packages both server
pieces as **per-user background services** (not system daemons — no root
needed, nothing shared between users):

- **JMS/messaging gateway** (ActiveMQ) — `ecce-gateway-start` /
  `ecce-gateway-stop` / `ecce-gateway-status`
- **Data server** (Apache + mod_dav, WebDAV) — `ecce-dataserver-start` /
  `ecce-dataserver-stop` / `ecce-dataserver-status`

Both live under `~/.ECCE/<service>/`. **You don't normally need to run
these by hand** — every `ecce-<app>` wrapper auto-starts both on launch if
they aren't already running (this also means: if you start `gateway` while
a service is already running, the wrapper's start call is a no-op — safe to
launch multiple apps back to back). Set `ECCE_NO_MESSAGING=1` or
`ECCE_NO_DATASERVER=1` to skip auto-start (e.g. for debugging one app in
isolation).

## 5. Create a data-server account

The data server ships with account auto-creation turned off
(`ECCE_AUTO_ACCOUNTS no`), so create your login manually, once, before first
use:

```
ecce-dataserver-start          # if not already running
ecce-dataserver-adduser        # interactive: prompts for name + username,
                                # then a password via htpasswd
```

Use a `userid` matching your Unix username (`$USER`) — that's what
`gateway`'s login dialog defaults to.

## 6. Log in

Launch the client, e.g.:

```
ecce
```

This is ECCE's main entry point/toolbar. It'll show an "ECCE Authentication"
dialog — log in with the username/password you just created. From the
gateway toolbar you can open the other tools (Organizer, Builder, Periodic
Table, ...).

## 7. Getting help

`ecce-<app>`'s Help menu opens local HTML content shipped in the package
(`/opt/ecce/data/client/WebHelp/`) — no network access or external CGI
service required.

## Known rough edges (this fork, current state)

- **Manual resizing** of some dialogs (e.g. Gateway Preferences) may still
  look slightly off — cosmetic, not a functional blocker.
- Each user's data server is a private, single-user store (matches the
  per-user service design above) — this isn't a shared multi-user server
  the way PNNL's original production deployment was.
- The Perl CGI self-service account flow, and the `SS_COMPRESSION`
  bandwidth filter for trajectory transfers, are intentionally not ported
  (see `CLAUDE.md` for why) — manual `ecce-dataserver-adduser` covers
  account creation, and file transfer just runs uncompressed.

## Troubleshooting

- `ecce-<app>` prints nothing and exits immediately → check
  `ecce-gateway-status` / `ecce-dataserver-status`; if either failed to
  start, run the matching `-start` script directly in a terminal to see its
  error output.
- Login fails with a connection error → confirm `ecce-dataserver-status`
  reports the server as up, and that you created an account with
  `ecce-dataserver-adduser` matching the username you're logging in with.
- A GUI app crashes on some specific action → see `CLAUDE.md` for the
  active-investigation log of fixes already made to this fork (wx3.2/GTK3
  layout issues, missing-icon typos, etc.) before assuming it's a new bug.

### My HPC machine needs two-factor authentication

ECCE submits jobs by running `ssh` to the machine, which cannot answer an
interactive 2FA prompt. Without a way around that you lose the whole
application, including input generation and output analysis, neither of which
needs the remote machine at all.

There is a mode for this (issue #94). Add to the machine's config file —
`~/.ECCE/CONFIG.<machine>`, or `$ECCE_HOME/siteconfig/CONFIG.<machine>` for a
site-wide setting:

    noRemoteAccess: true

ECCE then stops after generating the input deck and submit script locally, and
tells you the directory they are in. Copy that directory to the machine,
submit it yourself, and bring the output back. ECCE will not try to reach the
machine at all — no login check, no file transfer, no job monitoring.

Do not confuse this with the older `userSubmit: true`, which still transfers
files over `ssh` and only leaves the final submit command to you. That one
does not help with 2FA, because the transfer needs the same authentication.

**Status**: the generate-and-stop half is implemented. Reading the output back
in is issue #44, and is not wired up yet — for now the results have to be
brought in by other means.

### The job ran, but with different settings than I chose

This is the failure mode to know about: ECCE builds a code's input file by
substituting `##tag##` slots in a template, and historically a tag that
resolved to nothing just vanished along with its line. The job then ran
normally, with the setting you picked silently absent — no error anywhere.
(Real example: selecting BP86 for an NWChem DFT job ran the default LDA
instead, because the dialog and the input generator spelled the functional
differently.)

Two verbose modes make that visible:

    # what the input generator did with every tag, on stderr
    ECCE_AI_DEBUG=1 ecce

Output is prefixed `[ecce-ai]` and includes the settings dictionary the
generator was given, what each tag resolved to, and — the line to look for —
`!! &DFTXCFun has no case for '...'`, meaning the dialog and the generator
disagree about that value and it will be missing from the input file.

The Theory/Runtype Details dialogs have their own verbose mode, which
`calced` turns on per dialog; its output is prefixed `[ecce-dialog]` and
explains restore decisions, including a stored value skipped because the
field's unit changed since the calculation was saved.

Both write to stderr, so start the app from a terminal (or check the
gateway's log) to see them. If you are reporting a problem, `ECCE_AI_DEBUG=1`
output for the affected job is the single most useful thing to attach.
