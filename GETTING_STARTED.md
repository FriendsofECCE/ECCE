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

The site configuration under `/opt/ecce/siteconfig` (the machine list,
queues, `DataServers`, …) is marked as configuration from 8.17.0, so an
upgrade keeps what `sudo ecce -admin` or `ecce-remote-setup` wrote there;
dpkg asks before replacing a file you changed.

### Split packages (client/server)

By default CPack still builds one monolithic `ecce_<version>_amd64.deb`
with everything in it, as above — nothing below changes unless you ask
for it. Configuring with `-DECCE_SPLIT_PACKAGES=ON` instead produces two
packages from the same build:

```
cmake -G Ninja -DECCE_SPLIT_PACKAGES=ON ..
ninja
cpack -G DEB
```

- **`ecce-client`** — the GUI apps, input generators/parsers, codereg
  dialogs, the job-side scripts the Launcher copies to compute hosts
  (`gensub`, `eccejobmonitor`, `*.desc`), the per-session JMSDispatcher
  relay, `siteconfig/`, and `ecce-remote-setup`/`ecce-diagnose`. Depends
  on `python3-wxgtk4.0`, `csh | tcsh`, `perl`, `xterm` and
  `default-jre-headless` (the JMSDispatcher relay runs unconditionally,
  including under `-remote` with no local `ecce-server` at all, so its
  JVM can't be left to arrive only via a Recommends); Recommends
  `ecce-server`, `nwchem` and `openssh-client`; Suggests `imagemagick`
  and `www-browser` (not Depends — a client of someone else's central
  server needs neither `ecce-server` nor `nwchem` locally).
- **`ecce-server`** — the per-user or central WebDAV data server (Apache
  config, structure/basis-set libraries, help content) and the ActiveMQ
  broker's config. Depends on `apache2`, `apache2-utils`, `activemq`.

Install both on one machine for the same all-in-one behaviour as the
monolithic package. For the teaching/central-server deployment (one data
server + broker for a group, students as clients — see CLAUDE.md), install
only `ecce-server` on the server box and only `ecce-client` everywhere
else, then run `ecce-remote-setup <server-host>` on each client and start
sessions with `ecce -remote`.

A client-only install (no `ecce-server` package, `ECCE_REMOTE_SERVER` not
set) does not try to start a local data server or broker — `ecce-gateway-start`
and the `ecce-<app>` wrappers print what's missing on stderr and tell you
to either install `ecce-server` or point at a central one.

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

Your entries are **added to** the site ones, not a replacement — the
machines configured site-wide stay available, and a machine named in both
takes its settings from your file.

**Register the machine first.** A queue entry naming a machine that is not
in your machine registry is a fatal error, not a warning.

### Customising the submit script

The four queue fields above are the only things ECCE reasons about
numerically. Everything else — `--account`, `--qos`, `--constraint`,
`--gres=gpu:N`, `module load …` — goes in the **submit directive block**,
which is a per-queue-manager template:

```
Slurm {
#SBATCH --partition=$queue
#SBATCH --nodes=$nodes
#SBATCH --ntasks=$totalprocs
#SBATCH --time=$wallTime
}
```

The site defaults live in `/opt/ecce/siteconfig/submit.site`. To change them
for one machine, put your own block in `~/.ECCE/CONFIG.<host>` — `gensub`
reads that file **last**, so it wins.

Variables you can use:

`$account` `$code` `$ecceDir` `$host` `$infile` `$inFile` `$memory`
`$memoryMw` `$nodes` `$outfile` `$outFile` `$ppn` `$queue` `$runDir`
`$scratchDir` `$submitFile` `$totalprocs` `$USER` `$wallTime` `$wallHrMin`
`$wallSeconds`

`$wallTime` is `H:M:00` and is not zero-padded; `$wallHrMin` is `H:M`.

Supported queue managers: **Slurm**, PBS (OpenPBS/PBS Pro), LSF, Moab, SGE,
HTCondor, and Shell (run directly, no scheduler).

HTCondor submits a description file, not the script.  Its block in
`submit.site` holds that description as `#CONDOR` lines, and the submit
command cuts them out of the script into `<script>.sub` for `condor_submit`.
Jobs run in the run directory with `should_transfer_files = NO`, so they
need a shared file system (or a one-machine pool).  Pools mount a private
`/tmp` over the real one, so **a run directory under `/tmp` or `/var/tmp` is
refused** with an explanation; use one under your home directory. LoadLeveler, Maui and EASY were
retired in 8.11.0 — see `siteconfig/disabled-queuemanagers-archive.txt`,
which keeps their definitions verbatim if you ever need one back.

### Job scripts are POSIX sh

The script `gensub` writes starts with `#!/bin/sh`, so a compute machine needs
no `csh` or `tcsh` to run ECCE jobs. Scheduler shell-selection directives
(`#PBS -S`, `#BSUB -L`, `#$ -S`) name `/bin/sh` too.

Text you put in `CONFIG.<host>` or `submit.site` that becomes part of that
script (`setup`, `wrapup`, `<Code>Command`, `<Code>_loophole`) must therefore
be sh. `gensub` checks it line by line:

| csh you may have | what `gensub` does |
|---|---|
| `setenv NAME value` (one word, or quoted) | translated to `export NAME=value` |
| `set name = value` (one word, or quoted) | translated to `name=value` |
| `cmd >& file`, `>&! file`, `>>& file`, `\|&` | translated to `> file 2>&1` etc. |
| `exit (n)` | translated to `exit n` |
| anything else csh: `if (...) then`, `foreach`, `end`/`endif`, `source`, `set x = (a b)`, `@ n = ...`, `$?var`, `$status`, `$x:h`, `limit`, `alias`, `switch`, `while (...)` | **refused**: no script is written and the error lists every such line with the sh form to use |

It refuses rather than guesses because a wrong guess would run the wrong job.
Equivalents: `if [ -e f ]; then ... fi`, `for x in a b; do ... done`,
`. /etc/profile.d/modules.sh` (the `sh` flavour of an init file, never the
`csh` one), `x="a b"`, `PATH="dir:$PATH"; export PATH`, `n=$((a * b))`,
`${VAR+set}` for `$?VAR`, `${x%/*}` for `$x:h`, `ulimit` for `limit`.

`ecce-csh2sh` does the conversion for you. It rewrites what has an exact sh
equivalent (everything in the table above that `gensub` translates, plus
`if (-e|-d|-f|-x|... f) then`, `if ($?VAR)`, `if ("$X" == "y")` and numeric
`<`/`>` tests with `&&`, `||`, `!`, `else if`, `else`, `endif`, one-line
`if (...) command`, `foreach v (words) ... end`, `while (...) ... end`,
`$status` and `source`). Anything it is not certain of is left alone and
listed with the sh form to write. `source x.csh` becomes `. x.sh` when `x.sh`
exists on the machine running the command, or always with `--assume-sh-twins`
(then `x.sh` must exist where the job runs; `/init/csh` becomes `/init/sh`).

    ecce-csh2sh --check --user          # ~/.ECCE/CONFIG.*, report only
    ecce-csh2sh --convert --user        # rewrite; original kept as CONFIG.x.csh-backup
    sudo ecce-csh2sh --convert --siteconfig   # submit.site and CONFIG.* in $ECCE_HOME/siteconfig

`--check` exits 1 when anything needs attention. A second `--convert` changes
nothing, and an existing `.csh-backup` is never overwritten (`--force`).
Comments, blank lines, key order and every other key are untouched.

`ecce` runs `--convert --user` once for each version of ECCE and tells you in a
dialog which files it changed and which lines still need you. Installing the
package never edits site files: it prints a notice when `submit.site` or a site
`CONFIG.*` still holds csh, and you run the `sudo` command above.

Migrating a site: run one job per machine after upgrading; a CONFIG that
needs changes fails at launch with the lines named. The shipped
`siteconfig/CONFIG-Examples/` are already converted. `sourceFile` in a CONFIG
file is unaffected: it is read by the login shell ECCE connects through, not by
the job script.

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

### Deployment modes

That is mode 1. A site can instead share the broker, the data server, or
both. Which broker a quit may stop is decided only by what the admin
declared (below), never by guessing who is connected. The data server is
only ever stopped by **Quit and Stop Server**, in every mode.

The broker has no authentication, and the data server speaks plain HTTP
(#138): whoever can reach their ports can use them. Keep them on loopback
or firewall them.

### ActiveMQ on RHEL, Rocky and Fedora

These distributions don't package ActiveMQ, so
a machine that runs a broker (modes 1 and 3) needs it installed by hand:
a JRE (`dnf install java-17-openjdk-headless`), then the ActiveMQ Classic
binary tarball from https://activemq.apache.org unpacked in, e.g.,
`/opt/activemq`. Point ECCE at it with `export
ACTIVEMQ_HOME=/opt/activemq` in the users' environment; for the mode 3
service, add `Environment=ACTIVEMQ_HOME=/opt/activemq` to the unit.
A client of a central server (mode 2) needs none of this.

#### Mode 1: everything local (the default)

Nothing to set up. A user's first session starts their own broker (port
8088, loopback only) and data server. The broker stops when that user's
last session ends, on any display. On a machine with several ECCE users
use mode 3: per-user brokers all want port 8088, so the first user's is
used by everyone else and goes away when that user quits.

#### Mode 2: a central server

One account on the server runs the data server and broker for everyone.
On the server, as that account:

```
ecce-remote-setup --server all   # mark it as the server; listen on every interface
ecce-dataserver-start && ecce-gateway-start
ecce-dataserver-adduser          # once per user
```

For a class, `ecce-dataserver-adduser --from class.csv` creates a batch of
accounts at once from a `username,password,first,last` CSV file (blank
password fields get a random one generated); generated passwords are
written to `class.csv.passwords` for the admin to hand out and then delete.

The listen setting is written once, to `~/.ECCE/dataserver/listen`, and
both services read it: the data server's own `Listen` directive and the
broker's bind address (#138). Leave out `all` if clients reach the
server through ssh tunnels. On each client machine, as root:

```
sudo ecce-remote-setup <server-host>
```

`ecce-remote-setup` also copies the server's registered site machine list
(`sudo ecce -admin` on the server) onto the client, so students don't
register machines by hand; re-run it on the client after the admin
changes that list (#188).

Users then run `ecce -remote`. A client quitting never stops the server's
services, and neither does the server account's own plain quit; its
Quit and Stop Server does. To make the account per-user again, remove
`~/.ECCE/activemq/server`.

Run the server under a dedicated account (e.g. `ecce`), not a teacher's
own login shared with students — `ecce-dataserver-adduser` still creates
one data-server login per student under it. That way only the server
account (or root) can ever reach the pidfiles and stop the services; a
client under `ECCE_REMOTE_SERVER` doesn't even get offered "Quit and Stop
Server" (#190), only a plain Quit.

#### Mode 3: one shared broker on an app server

One broker for every user of the machine, run by systemd under its own
account, instead of one JVM per user. As root:

```
sudo ecce-broker-setup           # declares localhost:8088 in siteconfig/SharedBroker
sudo systemctl link /opt/ecce/server/systemd/ecce-broker.service
sudo systemctl enable --now ecce-broker
```

Sessions then start only their own relay, pointed at that broker. No
quit, not even Quit and Stop Server, stops it; only `systemctl` does.
`ecce-broker-setup host:port` names a broker on another port or machine
(any non-loopback name makes the service listen on every interface).
`sudo ecce-broker-setup --remove` goes back to mode 1.

The data server is separate. Users run `ecce` for a per-user data server,
or `ecce -remote` for a central one set up with `ecce-remote-setup
<data-host>` as in mode 2; the shared broker is used either way. If you
also run a central data server alongside the shared broker, give it its
own dedicated account too, per mode 2 above.

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

### `ecce` command-line options

`ecce --help` lists these:

- **`-admin`** — edit the site-wide machine list (`$ECCE_HOME/siteconfig`)
  directly in Machine Registration; no broker or data server started.
  Needs write access to `siteconfig`, so typically `sudo ecce -admin`.
- **`-machine`** / **`-machines`** — edit your own machine registrations
  (`~/.ECCE`) the same way, without starting a session.
- **`-remote`** — use a central data server/broker
  (`siteconfig/RemoteServer`) instead of starting your own, for the
  two-machine teaching deployment (one server, students connect as
  clients).
- **`-l LOGIN`** — use `LOGIN` as your server login name instead of your
  Unix username, then start normally.
- **`--help`** / **`-h`** — print this list and exit.

### Choosing the editor

Text files (input decks, outputs) open in an external editor. The
simplest way to choose one is **Edit > Preferences > External programs**
in the Organizer, which also sets the terminal used for terminal editors
and the web browser for Help; changes apply at once. ECCE picks the editor
from `ECCE_EDITOR` first, then that preference, then `VISUAL`, then
`EDITOR`, and falls back to `vi` in an `xterm`. The value may carry
arguments (`ECCE_EDITOR="geany -i"`). Set the variable for one run with
`ECCE_EDITOR=geany ecce`, or for good with `export ECCE_EDITOR=geany` in
`~/.profile`. Every environment variable ECCE reads is listed in
[docs/ENVIRONMENT.md](docs/ENVIRONMENT.md) (installed as
`/opt/ecce/doc/ENVIRONMENT.md`).

Terminal editors (`vi`, `vim`, `nvim`, `view`, `nano`, `pico`, `micro`,
`emacs -nw`) are run inside an `xterm`. For `gedit`, `gnome-text-editor`,
`xed`, `geany` and `kate`, ECCE adds the "new instance" flag itself; an
editor that hands the file to an already-running copy and exits would
otherwise end the edit session at once.

### Built-in ssh

ECCE runs commands on this machine directly and on ssh machines over its
own libssh connection, with no shell session or prompts to match. A host
whose ssh config shares connections (`ControlMaster`/`ControlPath`, often
used to answer a second factor once) is reached through the `ssh` command
instead, so it reuses that connection. An ssh host key ECCE has not seen
is asked about in a dialog, and is refused until accepted. The setting is
**Edit > Preferences > External programs > Run commands without a shell
session (built-in ssh)**, on by default; untick it, or set
`ECCE_TRANSPORT=pty`, to go back to the scripted shell session of 8.x. It
applies to connections opened after the change, and a job keeps the
choice it was launched with.

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

#### Reaching the machine through a port-forwarded tunnel

If you forward a local port to the cluster (for example
`ssh -L 2222:login.hpc.example.edu:22 gateway`, after which you authenticate
once), **do not register the machine as `localhost` or `127.0.0.1`**.
ECCE treats those names, and this host's own name, as *this* machine: if the
login name you register is your own local one, the job runs locally on your
workstation instead of through the tunnel (#144). The Launcher and Machine
Registration say which way a loopback name will go, but the safe setup avoids
the question. Give the tunnel its own name in `~/.ssh/config`:

    Host hpc-tunnel
        HostName 127.0.0.1
        Port 2222
        User <your cluster username>

and register the machine as `hpc-tunnel`. An alias is never treated as local,
so it always goes through ssh.

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
