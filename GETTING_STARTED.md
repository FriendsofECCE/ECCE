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
  libmosquitto-dev mosquitto-dev libaprutil1-dev mosquitto git libssh-dev
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
sudo apt-get install -y apache2 apache2-utils mosquitto libaprutil1   # runtime dependencies
sudo dpkg -i ecce_<version>_amd64.deb
```

The package installs to `/opt/ecce` and drops thin wrapper scripts named
`ecce-<app>` (e.g. `ecce-organizer`, `ecce-builder`,
`ecce-pertable`) onto `/usr/bin`, plus `ecce` itself, which is the one
you actually start (it launches the gateway, which then spawns the apps --
running `ecce-builder` and friends directly skips that setup). No
`ECCE_HOME` sourcing or environment setup required first.

`apache2`/`apache2-utils` are real runtime dependencies (the data server
below runs as a real Apache instance), not just build-time. `mosquitto`
is the message broker and `libaprutil1` is used by the central broker's
login check (see "Deployment modes"). `dpkg -i` will fail to configure
without them if `apt-get install` wasn't run first. Neither Java nor
ActiveMQ is used. `sudo apt install ./ecce_<version>_amd64.deb` pulls
in the dependencies itself.

**Debian's `mosquitto` package also starts its own system service, on
port 1883.** ECCE neither uses nor needs it: ECCE starts its own broker
instances (below). It can be disabled with `sudo systemctl disable --now
mosquitto` without affecting ECCE.

On RHEL, Rocky and Fedora the RPM requires `mosquitto` (in EPEL on RHEL
and Rocky: `sudo dnf install epel-release`) and `apr-util`; the server
RPM also requires `httpd` and `httpd-tools`.

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
  (`gensub`, `eccejobmonitor`, `*.desc`), the scripts that start the
  per-user broker, `siteconfig/`, and `ecce-remote-setup`/`ecce-diagnose`.
  Depends on `python3-wxgtk4.0`, `perl`, `xterm` and `libmosquitto1`
  (every ECCE process links it); Recommends `ecce-server`, `mosquitto`
  (the broker program, needed for a local session but not for a client
  of a central server), `nwchem` and `openssh-client`; Suggests
  `imagemagick` and `www-browser` (not Depends — a client of someone
  else's central server needs neither `ecce-server` nor `nwchem`
  locally).
- **`ecce-server`** — the per-user or central WebDAV data server (Apache
  config, structure/basis-set libraries, help content), the central
  broker's login-check plugin (`server/ecce_users_auth.so`) and access
  rules (`server/mosquitto.acl`), and the shared-broker service unit.
  Depends on `apache2`, `apache2-utils`, `mosquitto`, `libaprutil1` and
  `ecce-client` (same version). The RPMs correspondingly require
  `mosquitto`, `apr-util`, `httpd` and `httpd-tools`.

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
file is unaffected: ECCE runs it once per connection, in the machine's own
shell (`shell:`), not in the job script.

### Running two instances at once

Everything that makes an instance distinct is an environment variable:

| variable | default | what it moves |
| --- | --- | --- |
| `ECCE_REALUSERHOME` | `$HOME` | the whole `.ECCE` state directory: preferences, the data server's document root, the per-session broker files |
| `ECCE_DATASERVER_PORT` | `8096` | the data server |
| `ECCE_BROKER_PORT` | `8088` | the central broker's TCP port (`-remote` clients, and a server account's broker); a per-user broker has no port |

So a completely separate instance, sharing nothing with your normal one, is:

```
export ECCE_REALUSERHOME=/tmp/ecce-scratch
export ECCE_DATASERVER_PORT=8097 ECCE_BROKER_PORT=8089
ecce-dataserver-start && ecce-gateway-start
ecce
```

A per-user broker listens on a Unix socket in `$ECCE_REALUSERHOME/.ECCE`
(mode 0700, no TCP port), so instances do not collide on it;
`ECCE_BROKER_PORT` only matters to a central server's broker and to
`-remote` clients. The processes find their broker through
`$ECCE_REALUSERHOME/.ECCE/broker_<host>_<display>`, written by
`ecce-gateway-start`, so a local and a `-remote` session of one account
on different displays do not overwrite each other.

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
needed, nothing shared between users), apart from the optional shared
broker of mode 3:

- **Message broker** (Mosquitto, MQTT 5) — `ecce-gateway-start` /
  `ecce-gateway-stop` / `ecce-gateway-status`
- **Data server** (Apache + mod_dav, WebDAV) — `ecce-dataserver-start` /
  `ecce-dataserver-stop` / `ecce-dataserver-status`

Both keep their files under `~/.ECCE/` (the broker's socket, configuration, log and pid file directly in it, the data server in `dataserver/`). **You don't normally need to run
these by hand** — every `ecce-<app>` wrapper auto-starts both on launch if
they aren't already running (this also means: if you start `gateway` while
a service is already running, the wrapper's start call is a no-op — safe to
launch multiple apps back to back). Set `ECCE_NO_MESSAGING=1` or
`ECCE_NO_DATASERVER=1` to skip auto-start (e.g. for debugging one app in
isolation).

### Deployment modes

There are three. Which one a session uses is decided by the site's
configuration, in this order: `siteconfig/SharedBroker` (mode 3), then
`ecce -remote` (mode 2), otherwise mode 1. In every mode the broker is
Mosquitto, never the system's own `mosquitto.service` (Debian runs that
one on port 1883; ECCE does not use it and it can be disabled).

| | broker | where it listens | who may connect |
| --- | --- | --- | --- |
| 1. local | one per user, started by `ecce-gateway-start` | a Unix socket in `~/.ECCE` (0700), no TCP port | the user's own processes |
| 2. central server | the server account's broker | TCP, port 8088 (`ECCE_BROKER_PORT`) | accounts of the data server's `users` file |
| 3. shared broker | one system service, `ecce-broker.service` | TCP, port 8088 unless declared otherwise | accounts in `siteconfig/SharedBroker.passwd` |

On a TCP broker (modes 2 and 3) no anonymous client is accepted, and an
account may publish and subscribe only below `ecce/<its name>/` (plus
hearing that a machine registration changed). One user therefore cannot
act in another's name, for example by cancelling that user's jobs. The
rules are `server/mosquitto.acl`. The data server still speaks plain
HTTP and the broker's password is sent unencrypted too (#138): keep both
ports on a trusted network or firewall them. The accounts keep users
apart; they are not a defence against an untrusted network.

A client stopping never stops a central or shared broker. Which broker a
quit may stop is decided only by what the admin declared, never by
guessing who is connected. The data server is only ever stopped by
**Quit and Stop Server**, in every mode.

#### Mode 1: everything local (the default)

Nothing to set up. A user's first session starts their own broker (a
Unix socket, no port, no password) and data server. The broker stops when
that user's last session ends, on any display. Several users on one
machine each get their own broker; mode 3 is only needed to share one.

Instead of a data server, a single-user install can keep its projects in
a folder (local data mode, #216): set **Edit > Preferences > Data folder**
(default `~/.ECCE-local`), or `ECCE_LOCAL_DATA=<folder>` in the
environment, which wins over the preference. No data server is started.
Server mode remains the default. The broker is unchanged.

#### Mode 2: a central server

One account on the server runs the data server and broker for everyone.
On the server, as that account:

```
ecce-remote-setup --server all   # mark it as the server; listen on every interface
ecce-dataserver-start && ecce-gateway-start
ecce-dataserver-adduser          # once per user
```

`--server` creates `~/.ECCE/mosquitto.server`. With that mark,
`ecce-gateway-start` gives the account's broker, besides its own socket,
a TCP listener on port 8088 (`ECCE_BROKER_PORT`) at the addresses of the
listen setting, and restarts a broker that is already running without
it. The broker checks each login against the data server's own `users`
file (`~/.ECCE/dataserver/users`) through the plugin
`/opt/ecce/server/ecce_users_auth.so`, rereading the file when it
changes. A user's data server login is therefore also their broker login,
and `ecce-dataserver-adduser` needs no separate step; an account or
password change takes effect without restarting anything.

For a class, `ecce-dataserver-adduser --from class.csv` creates a batch of
accounts at once from a `username,password,first,last` CSV file (blank
password fields get a random one generated); generated passwords are
written to `class.csv.passwords` for the admin to hand out and then delete.

The listen setting is written once, to `~/.ECCE/dataserver/listen`, and
both services read it: the data server's own `Listen` directive and the
broker's bind addresses (#138). Leave out `all` if clients reach the
server through ssh tunnels. On each client machine, as root:

```
sudo ecce-remote-setup <server-host>
```

`ecce-remote-setup` also copies the server's registered site machine list
(`sudo ecce -admin` on the server) onto the client, so students don't
register machines by hand; re-run it on the client after the admin
changes that list (#188). It writes only the data server's address; the
client finds the broker on the same host, port 8088 (set `ECCE_BROKER_PORT`
in the client's environment if the server uses another).

Users then run `ecce -remote`. A client quitting never stops the server's
services, and neither does the server account's own plain quit; its
Quit and Stop Server does. To make the account per-user again, remove
`~/.ECCE/mosquitto.server`.

Run the server under a dedicated account (e.g. `ecce`), not a teacher's
own login shared with students — `ecce-dataserver-adduser` still creates
one data-server login per student under it. That way only the server
account (or root) can ever reach the pidfiles and stop the services; a
client under `ECCE_REMOTE_SERVER` doesn't even get offered "Quit and Stop
Server" (#190), only a plain Quit.

Open to clients on the server: the data server port (8096 by default)
and the broker port (8088). The data server and broker must both run for
`ecce -remote` to start; if either is down, `ecce-gateway-start` names
the host and port that does not answer.

#### Mode 3: one shared broker on an app server

One broker for every user of the machine, run by systemd under its own
account (`ecce-broker`, state in `/var/lib/ecce-broker`) instead of one
broker per user. As root:

```
sudo ecce-broker-setup           # declares localhost:8088 in siteconfig/SharedBroker
sudo ecce-broker-setup --user alice   # one account per user; asks for the password
sudo systemctl link /opt/ecce/server/systemd/ecce-broker.service
sudo systemctl enable --now ecce-broker
```

The per-user data servers keep their user files in home directories the
broker cannot read, so the shared broker has its own account list,
`siteconfig/SharedBroker.passwd` (hashed, mode 0600). `--user NAME`
adds an account, or changes its password; use the same name and password
the user has on their data server, because that is what their session
presents. `--remove-user NAME` deletes one. After a change run `sudo
systemctl reload ecce-broker` (`ecce-broker-setup` does it itself when
the service is running and it is run as root). The list is not synced
with the data server: when a user's data server password changes,
change it here too.

Sessions then start no broker of their own and connect to this one. No
quit, not even Quit and Stop Server, stops it; only `systemctl` does.
`ecce-broker-setup host:port` names a broker on another port or machine
(any non-loopback name makes the service listen on every interface).
`sudo ecce-broker-setup --remove` goes back to mode 1.

The data server is separate. Users run `ecce` for a per-user data server,
or `ecce -remote` for a central one set up with `ecce-remote-setup
<data-host>` as in mode 2; the shared broker is used either way. If you
also run a central data server alongside the shared broker, give it its
own dedicated account too, per mode 2 above.

### When the broker refuses a login

On a central or shared broker the Gateway shows a dialog, once per
session, titled "Message broker refused the login": the data server
accepted the user's login, but the broker did not know the account or its
password. Until it is fixed, jobs do not report back and ECCE's windows do
not update each other. The administrator checks, by mode:

- **Mode 2**: the account must exist in the server account's
  `~/.ECCE/dataserver/users` with the password the user types, and the
  server's broker must be the one with the TCP listener: on the server,
  `ls ~/.ECCE/mosquitto.server`, `ecce-gateway-status`, and
  `~/.ECCE/mosquitto.log`. Re-run `ecce-remote-setup --server` and
  `ecce-gateway-start` if the mark is missing.
- **Mode 3**: `sudo ecce-broker-setup --user NAME` with the user's data
  server password, then `sudo systemctl reload ecce-broker`;
  `systemctl status ecce-broker` for the service.

`ecce-diagnose` (also run by `ecce --bug`) lists what is listening on
ports 8096 and 8088 and which `mosquitto` processes the user runs; it
does not test a login.

### Upgrading from 8.x

9.x replaces ActiveMQ and its Java relay with Mosquitto. Install
`mosquitto` (and `libaprutil1`/`apr-util` on a server) with the new
packages; `activemq` and a JRE are no longer needed and can be removed.

**Single-user install.** Nothing to configure: the next session starts a
per-user Mosquitto in place of ActiveMQ. Calculations, preferences and
the data server's contents are untouched.

**Central server.**

- Existing data server accounts keep working as broker accounts. Nothing
  is re-created and no passwords change.
- On the server account, restart the services (the 8.x central-server
  mark, `~/.ECCE/activemq/server`, is carried over to
  `~/.ECCE/mosquitto.server` at the first start): `ecce-dataserver-stop; ecce-gateway-stop; ecce-dataserver-start
  && ecce-gateway-start`. Stop any ActiveMQ still running there. The existing `dataserver/listen` setting is kept.
- The firewall is unchanged: the broker is still on port 8088, the data
  server on 8096. Only the protocol on 8088 differs (MQTT, no longer
  OpenWire).
- **Clients must be 9.x as well.** An 8.x client cannot talk to a 9.x
  broker, nor a 9.x client to an 8.x server. Upgrade the server and every
  client together, then re-run `sudo ecce-remote-setup <server-host>` on
  each client only if the server's machine list changed.

**Shared system broker (mode 3).** Add an account per user with
`sudo ecce-broker-setup --user NAME` (the 8.x broker had no accounts),
then `sudo systemctl restart ecce-broker`. `siteconfig/SharedBroker` is
unchanged. The unit file is replaced by the new package; if the service
was linked from `/opt/ecce/server/systemd/ecce-broker.service`, the link
still points at it.

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

### How ECCE connects to machines

ECCE runs commands on this machine directly and on ssh machines over its
own libssh connection, with no shell session or prompts to match. There is
no setting for this: the scripted shell session of 8.x is gone. A host
whose ssh config shares connections (`ControlMaster`/`ControlPath`, often
used to answer a second factor once) is reached through the `ssh` command
instead, so it reuses that connection. When that connection is not open,
ECCE opens it: the password or verification code (and an unknown host key)
are asked in ECCE dialogs, so no terminal is needed. Without a display, or
if the dialog is cancelled, ECCE says that no shared connection is open and
to run `ssh <host>` once in a terminal. A host reached through `ProxyJump` or
`ProxyCommand` in `~/.ssh/config` goes the same way, since libssh cannot
answer a login prompt of the proxy's own ssh; if the config shares no
connection for it, ECCE adds its own (kept ten minutes under `~/.ECCE/cm`),
so you are asked once. An ssh host key ECCE has not seen
is asked about in a dialog, and is refused until accepted.

Because no interactive shell is started, `~/.bashrc` and `~/.cshrc` on a
compute machine are no longer read for ECCE's commands. Whatever a code
needs on its `PATH` or in its environment goes in the machine's **source
file** (`sourceFile` in its `CONFIG` file, see above); ECCE runs it once
per connection in the shell it is written for, and applies the variables it sets to every command
(aliases and shell functions it defines are not carried over). Machines
registered with `sshpass` or `ssh/ftp` are plain ssh now.

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
