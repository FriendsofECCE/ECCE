# Getting started with ECCE (modernized build)

This covers building, installing, and running this fork of ECCE on Debian
13 ("trixie") — from a clean checkout to a working login. It documents
`main`'s CMake/CPack packaging, not the old `build_ecce`/recursive-make
workflow.

## 1. Install build dependencies

```
sudo apt-get install -y \
  build-essential gfortran cmake ninja-build \
  libwxgtk3.2-dev libxerces-c-dev libgl-dev libglu1-mesa-dev \
  libgtk-3-dev libx11-dev libice-dev \
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
`ecce-<app>` (e.g. `ecce-gateway`, `ecce-organizer`, `ecce-builder`,
`ecce-pertable`) onto `/usr/bin`, so they're just runnable by name — no
`ECCE_HOME` sourcing or environment setup required first.

`apache2`/`apache2-utils` are real runtime dependencies (the data server
below runs as a real Apache instance), not just build-time — `dpkg -i` will
fail to configure without them if `apt-get install` wasn't run first.

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
ecce-gateway
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
