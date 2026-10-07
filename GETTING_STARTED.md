# Getting started with ECCE (modernized build)

This covers building, installing, and running this fork of ECCE on Debian
13 ("trixie") — from a clean checkout to a working login. It documents
`main` (the 9.0 development line, 9.0.0-alpha previews) and its
CMake/CPack packaging, not the old `build_ecce`/recursive-make workflow.
For 8.x (branch `stable-8`), use that branch's copy of this file; its
package is the single `ecce` package.

CI also builds on Ubuntu, Fedora, and Rocky Linux (RHEL family) — the
steps below apply there too, adjusted for the distro's package manager.
Debian is the tested platform. There are no native Windows or macOS
clients yet (#133, #232); until then ECCE runs on Linux only.

## 1. Install build dependencies

```
sudo apt-get install -y \
  build-essential gfortran cmake ninja-build \
  libwxgtk3.2-dev libxerces-c-dev libgl-dev libglu1-mesa-dev \
  libgtk-3-dev libx11-dev libice-dev libxt-dev libjpeg-dev \
  libmosquitto-dev mosquitto-dev libaprutil1-dev mosquitto git dpkg-dev file libssh-dev libssl-dev \
  python3 libcoin-dev libegl-dev libxc-dev libeigen3-dev
```

`libxc-dev` and `libeigen3-dev` are for ecce-qm, the engine ECCE bundles (code
ECCE-QM); without them the build downloads libxc and Eigen at configure time,
as it always does for libcint. `-DECCE_BUILD_QM=OFF` leaves the engine out.

On Rocky 9 (with EPEL and CRB enabled) and Fedora the Coin3D packages are
`Coin4-devel` (runtime `Coin4`) and `mesa-libEGL-devel`; OpenSSL is
`openssl-devel` (`libssl-dev` on Debian).

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

The 3D viewer is built against the system Coin3D (`libcoin-dev` 4.0.x on
Debian; `ECCE_USE_COIN=ON` is the default, #166). `-DECCE_USE_COIN=OFF` builds
the vendored Open Inventor core in `src/inv` instead; it stays as a fallback
for one release and needs no Coin3D. Use a separate build directory (e.g.
`build-oiv`) for it: switching the option in one tree rebuilds everything.
CI builds both.

Orbital and isosurface lobes are drawn with accurate (depth-peeled)
transparency, which needs an alpha buffer on the canvas; a scene whose frames
take more than 100 ms switches itself to the stippled quick mode, with a note
in the status bar. `ECCE_TRANSPARENCY_FALLBACK_MS=<ms>` moves that threshold
(0 turns the switch off) and `ECCE_QUICK_TRANSPARENCY=1` forces quick mode, as
does the "Quick transparency" preference.

This produces the 19 GUI apps (`gateway`, `organizer`, `builder`,
`pertable`, ...) plus the CLI apps, all statically linked against the
in-tree libraries.

## 3. Package and install

```
cd build-cmake
cpack -G DEB
sudo apt install ./ecce-client_<version>_amd64.deb ./ecce-server_<version>_amd64.deb
```

`cpack` leaves two packages in `build-cmake/` (see "Split packages"
below); install both for a standalone machine. `apt install ./file.deb`
pulls in the dependencies itself.

The packages install to `/opt/ecce` and drop thin wrapper scripts named
`ecce-<app>` (e.g. `ecce-organizer`, `ecce-builder`,
`ecce-pertable`) onto `/usr/bin`, plus `ecce` itself, which is the one
you actually start (it starts the session, which then spawns the apps --
running `ecce-builder` and friends directly skips that setup). No
`ECCE_HOME` sourcing or environment setup required first.

Apache (`apache2`, `apache2-utils`) is a real runtime dependency of
`ecce-server` (the data server below runs as a real Apache instance), not
just build-time. `mosquitto` is the message broker and `libaprutil1` is
used by the central broker's login check (see "Deployment modes").
Neither Java nor ActiveMQ is used.

**Debian's `mosquitto` package also starts its own system service, on
port 1883.** ECCE neither uses nor needs it: ECCE starts its own broker
instances (below). It can be disabled with `sudo systemctl disable --now
mosquitto` without affecting ECCE.

On RHEL, Rocky and Fedora the client RPM requires `mosquitto`, `Coin4` (EPEL 9
has 4.0.10, Fedora 4.0.10 and 4.0.7), `python3-wxpython4`, the wxGTK 3.2 and
xerces-c libraries and `curl`. On RHEL and Rocky all of these except `curl`
are in EPEL, so run `sudo dnf install epel-release` before installing the
RPMs (without it dnf stops with `nothing provides Coin4`, `mosquitto`,
`python3-wxpython4`, ...); the server RPM requires
`httpd`, `httpd-tools`, `mosquitto` and `apr-util`. The Rocky 9 RPMs of
9.0.0-alpha.3 were installed and run on RHEL 9 (local mode with only the
client package, and server mode). The Fedora RPMs are built but not run.

The site configuration under `/opt/ecce/siteconfig` (the machine list,
queues, `DataServers`, …) is marked as configuration, so an
upgrade keeps what `sudo ecce -admin` or `ecce-remote-setup` wrote there;
dpkg asks before replacing a file you changed.

### Split packages (client/server)

`cpack` builds two packages from the same build
(`-DECCE_SPLIT_PACKAGES=OFF` builds the single `ecce` package of 8.x
instead):

- **`ecce-client`** — the GUI apps, input generators/parsers, codereg
  dialogs, the job-side scripts the Launcher copies to compute hosts
  (`gensub`, `eccejobmonitor`, `*.desc`), the scripts that start the
  per-user broker, `siteconfig/`, and `ecce-remote-setup`/`ecce-diagnose`.
  Depends on `python3-wxgtk4.0`, `perl`, `xterm`, `libmosquitto1`, `curl`
  and Coin3D (every ECCE process links `libmosquitto`); Recommends
  `ecce-server`, `mosquitto`
  (the broker program, needed for a local session but not for a client
  of a central server), `nwchem` and `openssh-client`; Suggests
  `www-browser` (not Depends — a client of someone
  else's central server needs neither `ecce-server` nor `nwchem`
  locally).
- **`ecce-server`** — the per-user or central WebDAV data server (Apache
  config, structure/basis-set libraries, help content), the central
  broker's login-check plugin (`server/ecce_users_auth.so`) and access
  rules (`server/mosquitto.acl`), and the shared-broker service unit.
  Depends on `apache2`, `apache2-utils`, `mosquitto`, `libaprutil1` and
  `ecce-client` (same version). The RPMs correspondingly require
  `mosquitto`, `apr-util`, `httpd` and `httpd-tools`.

Install both on one machine for the all-in-one behaviour of the single
package. For the teaching/central-server deployment (one data
server + broker for a group, students as clients), install
only `ecce-server` (which pulls in `ecce-client`) on the server box and
only `ecce-client` everywhere
else, then run `ecce-remote-setup <server-host>` on each client and start
sessions with `ecce -remote`.

A client-only install (no `ecce-server` package, `ECCE_REMOTE_SERVER` not
set) does not try to start a local data server or broker — `ecce-gateway-start`
and the `ecce-<app>` wrappers print what's missing on stderr and tell you
to either install `ecce-server` or point at a central one.

### Describing the queues on your own cluster

ECCE needs to know a machine's batch queues — their names, processor and
time limits, and which queue manager (PBS, Slurm, Moab, SGE, LSF,
HTCondor) it runs. **Tools → Register Machines…**, tab **Queues**, edits
these (wall time in hours, memory and scratch in GB) and writes the two
files below; what it does not do yet is discover a cluster's queues, show
the job script before it is submitted or test it (#212). To edit the
files by hand instead:

Your own copies in `~/.ECCE/` take precedence over the root-owned ones
in `$ECCE_HOME/siteconfig`, so no `sudo` is needed:

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

   `runLimit` is in minutes (the Queues tab shows hours); `memLimit` 0 means
   no limit.

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
reads that file **last**, so it wins. The **Job script** tab of Register
Machines does this without editing the file: the site's text is shown
read-only, **Copy site text to edit** makes your copy, **Available words…**
lists the variables below, and **Advanced: edit file…** opens the file.
Each value is tagged *from site*, *your value* or *not set*, and a changed
field has an undo button. To see where a key's value came from outside the
GUI, run `gensub` with `GENSUB_EXPLAIN=1`: it prints each key's value, the
layer it came from and the values it overrode.

The same rule applies to everything in a machine's CONFIG file, for `gensub`
and for ECCE itself (`shell`, `sourceFile`, `perlPath`, `frontendMachine`,
`noRemoteAccess`, ...): the site file `siteconfig/CONFIG.<host>` is read
first, then `~/.ECCE/CONFIG.<host>`, and for each key the last non-empty value
wins. Keys are case-insensitive and a repeated key keeps its last value. Set
only what you want to change; the rest comes from the site file. To remove a
site value, write `key: -` in your file (an empty value is ignored). A block
(`setup { ... }`) replaces the site block whole. On a `-remote` client the
site file is the copy of the server's, and your file merges over it the same
way.

Jobs on this machine (`localhost`) need a `CONFIG.localhost` that names the
codes. `ecce` copies the template `siteconfig/CONFIG-Examples/CONFIG.localhost`
(bare names such as `nwchem`, `orca`, found through `PATH`) to
`~/.ECCE/CONFIG.localhost` on a start where you have none, and never
overwrites it; edit your copy to pin full paths. It is not copied when an
admin has put a `siteconfig/CONFIG.localhost` there; a file of yours is merged
over the site's.

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
`$ECCE_REALUSERHOME/.ECCE/broker_<host>_<session id>`, written by
`ecce-gateway-start`, so a local and a `-remote` session of one account
do not overwrite each other.

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
two should not run at the same time: each counts only its own programs when
deciding that the shared broker is no longer used.

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
rules are `server/mosquitto.acl`. Unless TLS is switched on (mode 2,
below) the data server speaks plain HTTP and the broker's password is
sent unencrypted too (#138): keep both ports on a trusted network or
firewall them. The accounts keep users apart; they are not a defence
against an untrusted network, and neither is TLS here: it keeps
passwords and data off the wire, and is no security product.
Mode 3 has no TLS yet.

A client stopping never stops a central or shared broker. Which broker a
quit may stop is decided only by what the admin declared, never by
guessing who is connected. The data server is only ever stopped by
**Quit and Stop Server**, in every mode.

#### Mode 1: everything local (the default)

Nothing to set up. A user's first session starts their own broker (a
Unix socket, no port, no password) and data server. The broker stops when
that user's last session ends. Several users on one
machine each get their own broker; mode 3 is only needed to share one.

A single-user install can keep its projects in a folder instead of a data
server; see the next section. Server mode remains the default on Linux;
on macOS, which has no mod_dav for a per-user data server, the data folder
is the default (#133).

#### Local data mode (a data folder instead of a data server)

Set **Edit > Preferences > Data folder** (default `~/.ECCE-local`; it takes
effect at the next start), or `ECCE_LOCAL_DATA=<folder>` in the environment,
which wins over the preference, and no data server is started (#216). The
broker is unchanged. Local mode keeps one data
folder per computer account: users are separated by their operating-system
accounts, and the person's home inside the folder is always `users/local`,
whatever the account is called. On a shared generic lab account everyone
would share that folder; use the central server (mode 2) there.

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
register machines by hand; run `sudo ecce-remote-setup --refresh` on the
client after the admin changes that list (#188). It writes only the data server's address; the
client finds the broker on the same host, port 8088 (set `ECCE_BROKER_PORT`
in the client's environment if the server uses another).

##### TLS for the central server (optional)

Off by default. On the server, as the account that runs it:

```
ecce-remote-setup --server all --tls     # makes a 10-year self-signed certificate
ecce-dataserver-stop; ecce-gateway-stop; ecce-dataserver-start && ecce-gateway-start
```

The data server then serves HTTPS on port 8443
(`ECCE_DATASERVER_TLS_PORT`) and the broker TLS on port 8883
(`ECCE_BROKER_TLS_PORT`). The plain ports (8096, 8088) accept connections
from the server machine itself only, so open 8443 and 8883 in the
firewall, not those. The certificate and key are in `~/.ECCE/tls/`
(`--new-cert` replaces them, `--cert F --key F` installs your own). Give
every client installation `~/.ECCE/tls/server.pem`, and on each client, as
root:

```
sudo ecce-remote-setup <server-host> --tls --pin server.pem
```

That client then accepts only this exact certificate and refuses any
other; it never falls back to plain HTTP or an unencrypted broker, and
reports a certificate that does not match. The installed copy is
`/opt/ecce/siteconfig/RemoteServer/server.pem`; a per-user override does
not exist. `--fetch-pin` takes the certificate from the running server
instead and prints its fingerprint to compare with the one the server's
administrator reads out (`ecce-remote-setup --server` shows it).

A certificate from Let's Encrypt or the university's CA works too, but is
only documented here: pass its full chain and key as `--cert` and `--key`
on the server (renewed certificates need the same command and a restart),
and set the clients up with `--tls --system-ca`, which checks the host
name against the system's CA list and installs no pin. Reach the server by
the name in the certificate.

##### Changing the site settings from a client

An administrator can edit the server's site machine list from a client
with `ecce -admin -remote`. Register Machines then saves over ssh, as the
administrator's own login on the server (`-l LOGIN` for another one, or a
`User` line in `~/.ssh/config`): `ecce-site-admin` on the server writes
`siteconfig` with the same writers as `ecce -admin` there, publishes the
files to clients, and the client fetches its copy again. The data server
password plays no part in this; the ssh login and the right to write the
server's `siteconfig` decide.

Once, on the server, as root, let a group write `siteconfig` (non-interactive
sudo over ssh is not used):

```
sudo groupadd ecceadmin
sudo usermod -aG ecceadmin alice            # each administrator; log in again
sudo chgrp -R ecceadmin /opt/ecce/siteconfig
sudo chmod -R g+w /opt/ecce/siteconfig
sudo chmod g+s /opt/ecce/siteconfig         # new files keep the group
```

The packages never change these permissions. An administrator checks the
setup from the client with `ssh <server-host> ecce-site-admin check`.

If administrators are not the account that runs the data server (`ecce`
above), they also need to write where that account publishes. As root,
add it to the group, then as `ecce` (logged in again):

```
sudo usermod -aG ecceadmin ecce
d=~ecce/.ECCE/dataserver/htdocs/Ecce/system/siteconfig
chgrp ecceadmin "$d" && chmod 2775 "$d"     # as ecce
echo "$d" > /opt/ecce/siteconfig/PublishDir # as an administrator
```

Every directory above `$d` must be searchable (`x`) by the administrators,
e.g. `chmod 711 ~ecce` if the home directory is private.

The administrator's own client refreshes its copy when that user can write
its `siteconfig` (the same group setup on the client); otherwise, and on
every other client, `sudo ecce-remote-setup --refresh` fetches the new list.

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

9.x replaces ActiveMQ and its Java relay with Mosquitto, and the single
`ecce` package with `ecce-client` and `ecce-server`, which replace it on
installation. Before installing, run `ecce-gateway-stop` and
`ecce-dataserver-stop` (or Quit and Stop Server), then install both new
packages; they bring `mosquitto` (and `libaprutil1`/`apr-util` on a
server). `activemq` and a JRE are no longer needed and can be removed.
Job scripts are now POSIX sh: `ecce` converts csh lines in your
`~/.ECCE/CONFIG.*` at the first start and lists what it could not; an
administrator converts the site's files with
`sudo ecce-csh2sh --convert --siteconfig` (see "Job scripts are POSIX sh").

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

Launch the client:

```
ecce
```

It shows an "ECCE Authentication" dialog; log in with the username and
password you just created. The Organizer then opens, and the Builder,
Periodic Table, Machine Register and the other tools are started from it.
(The Gateway window, which used to be the entry point, is hidden; set
`ECCE_GATEWAY_WINDOW=1` to bring it back.) In local data mode there is no
data server to log in to.

### `ecce` command-line options

`ecce --help` lists these:

- **`-admin`** — edit the site-wide machine list (`$ECCE_HOME/siteconfig`)
  directly in Machine Registration; no broker or data server started.
  Needs write access to `siteconfig`, so typically `sudo ecce -admin`.
  With `-remote`, edits the central server's list over ssh instead
  (see "Changing the site settings from a client" under Mode 2).
- **`-machine`** / **`-machines`** — edit your own machine registrations
  (`~/.ECCE`) the same way, without starting a session.
- **`-remote`** — use a central data server/broker
  (`siteconfig/RemoteServer`) instead of starting your own, for the
  two-machine teaching deployment (one server, students connect as
  clients).
- **`-l LOGIN`** — use `LOGIN` as your server login name instead of your
  Unix username, then start normally.
- **`--bug`** — run the session with diagnostic logging and, when it ends,
  collect the logs, the service logs and `ecce-diagnose` output into
  `~/ecce-bug-<time>.zip` (a `.tar.gz` without `zip`) to attach to a
  report. It holds no passwords, but does hold host names, user names and
  paths. Same as `ECCE_BUG=1`.
- **`--version`** / **`-V`** — print the version and exit.
- **`--help`** / **`-h`** — print this list and exit.

### Choosing the editor

Text files (input decks, outputs) open in an external editor. The
simplest way to choose one is **Edit > Preferences > External programs**
in the Organizer, which also sets the terminal used for terminal editors
and the web browser for Help; changes apply at once. ECCE picks the editor
from `ECCE_EDITOR` first, then that preference, then `VISUAL`, then
`EDITOR`, and falls back to `vi` in the terminal. The value may carry
arguments (`ECCE_EDITOR="geany -i"`). Set the variable for one run with
`ECCE_EDITOR=geany ecce`, or for good with `export ECCE_EDITOR=geany` in
`~/.profile`. Every environment variable ECCE reads is listed in
[docs/ENVIRONMENT.md](docs/ENVIRONMENT.md) (installed as
`/opt/ecce/doc/ENVIRONMENT.md`).

Terminal editors (`vi`, `vim`, `nvim`, `view`, `nano`, `pico`, `micro`,
`emacs -nw`) are run inside a terminal: `ECCE_TERMINAL`, else the Terminal
preference, else `xterm` (the same terminal is used for local shells and
tails). For `gedit`, `gnome-text-editor`,
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

- Open problems are on the
  [issue tracker](https://github.com/FriendsofECCE/ECCE/issues); the
  release notes of each 9.0.0-alpha list the known issues of that preview.
- The data server and the broker's password are sent unencrypted (#138):
  keep them on a trusted network (see "Deployment modes").
- The Perl CGI self-service account flow, and the `SS_COMPRESSION`
  bandwidth filter for trajectory transfers, are intentionally not ported
  — manual `ecce-dataserver-adduser` covers account creation, and file
  transfer just runs uncompressed.

## Troubleshooting

- `ecce` prints nothing and exits immediately → check
  `ecce-gateway-status` / `ecce-dataserver-status`; if either failed to
  start, run the matching `-start` script directly in a terminal to see its
  error output.
- Login fails with a connection error → confirm `ecce-dataserver-status`
  reports the server as up, and that you created an account with
  `ecce-dataserver-adduser` matching the username you're logging in with.
- Anything else → run `ecce --bug` and attach the archive to an issue.

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
