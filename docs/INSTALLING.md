# Installing ECCE from the packages

The packages are on the [releases page](https://github.com/FriendsofECCE/ECCE/releases). The help pages inside ECCE ([Installation](../help/src/installation.md)) describe the same steps for users; [GETTING_STARTED.md](../GETTING_STARTED.md) has the full build, packaging and server setup.

Prebuilt packages for Debian/Ubuntu (`.deb`) and RHEL/Rocky/Fedora
(`.rpm`) are on the
[releases page](https://github.com/FriendsofECCE/ECCE/releases), each
built on the system it is for; the Debian packages are the ones that are
tested. To build your own instead, see
[Building from source](../CONTRIBUTING.md#building-from-source).

## 1. Install the packages

9.x comes as two packages: `ecce-client` (the applications) and
`ecce-server` (the data server and broker; it needs the client of the same
version). Install both for a standalone machine. On Debian 13, from the
directory you downloaded them to:

```
sudo apt install ./ecce-client_<version>_amd64.deb ./ecce-server_<version>_amd64.deb
```

The `./` matters: it tells apt these are local files, and apt then pulls
in the dependencies itself. `ecce-client` depends on `perl`, `xterm`,
`curl`, `libmosquitto1`, wxPython (`python3-wxgtk4.0`) and Coin3D;
`ecce-server` on Apache (`apache2`, `apache2-utils`), `mosquitto` and
`libaprutil1`. A machine that only connects to someone else's central
server needs only `ecce-client`; apt recommends `ecce-server` and
`mosquitto` with it, and `--no-install-recommends` leaves them out.

On RHEL, Rocky or Fedora, `sudo dnf install ./ecce-client-<version>.x86_64.rpm
./ecce-server-<version>.x86_64.rpm`; on RHEL and Rocky run `sudo dnf install
epel-release` first, since Mosquitto and Coin3D (`Coin4`) come from EPEL. The
RPMs of 9.0.0-alpha.3 were run on RHEL 9; see
[Deployment modes](../GETTING_STARTED.md#deployment-modes).

ECCE installs to `/opt/ecce` and puts `ecce` and its helper commands
(`ecce-dataserver-adduser`, `ecce-remote-setup`, `ecce-diagnose`, …) on
your `PATH`.

**macOS and Windows** (experimental): native clients are in preview
([#133](https://github.com/FriendsofECCE/ECCE/issues/133)).

* **macOS:** since 9.0.0-alpha.7 each release has an `ECCE.app` in a
  `.dmg` for Apple silicon (macOS 11 or later) and for Intel (macOS 10.15
  or later); CI also builds them as workflow artifacts
  `ECCE-macos-dmg-arm64` and `ECCE-macos-dmg-x86_64`. Drag ECCE to
  Applications. The app is signed ad hoc only, so macOS refuses it at
  first. On macOS 14 and earlier, right-click the app and choose Open. On
  macOS 15 and later, open it once, then choose System Settings > Privacy
  & Security > Open Anyway. Either way, running
  `xattr -dr com.apple.quarantine /Applications/ECCE.app` also works.
* **Windows:** since 9.0.0-alpha.8 each release has a per-user installer,
  `ECCE-windows-<version>.msi` (no administrator rights needed; start
  ECCE from the Start menu), and the same files as a zip (unpack it and
  run `ecce.cmd`). Windows shows a SmartScreen warning at first, because
  the client is not signed: choose More info > Run anyway.

## 2. Choose where your data lives

**In a folder on your computer** — the default on macOS and Windows,
optional on Linux. There is nothing to set up: no account and no login.
On Linux, choose Edit → Preferences → Data folder (default
`~/.ECCE-local`; it takes effect at the next start), or start ECCE with
`ECCE_LOCAL_DATA=<folder>` set. See
[Deployment modes](../GETTING_STARTED.md#deployment-modes).

**In a data server on this computer** — the default on Linux. The data
server runs as your own user. Create a login on it once:

```
ecce-dataserver-start
ecce-dataserver-adduser        # prompts for name, username and password
```

Use your Linux username — that's what the login dialog defaults to.

**On a central server** for a group or a class: the server's
administrator creates your account; see [Deployment modes](#deployment-modes).

## 3. Start ECCE

```
ecce
```

On macOS open ECCE from Applications; on Windows run `ecce.cmd`.
With a data server, log in; the **Organizer** then opens: your calculations on the left, the
selected one in the middle, and the Builder, editors, Launcher and
viewer opened from it. The data server (if used) and the message broker
start by themselves. Closing the Organizer ends the session.

`ecce --help` lists the options; they are described in
[GETTING_STARTED.md](../GETTING_STARTED.md#ecce-command-line-options).
Organizer → Edit → Preferences chooses the editor, terminal and web
browser ECCE opens.

In the Builder, **File → New** starts an empty structure, **Open…**
loads one, **Add Structure from File…** adds a file's structure to the
current one, and **Close** closes it; an **Open structures** panel is
part of the window. **Save As…** can store a calculation in any local
folder as well as in the data server.

## 4. Run on a compute machine

A calculation runs on a registered machine. **`localhost`**, this
computer, is registered already: jobs run directly on it, without ssh and
without a batch system.

* **Codes on your `PATH` work at once.** ECCE starts NWChem, Gaussian,
  ORCA, MOPAC and Quantum ESPRESSO by their command names (`nwchem`,
  `g16`, `orca`, `mopac`, `pw.x`), so a code you can start from a
  terminal by that name needs no setup.
* **A code elsewhere needs its full path.** Open **Tools → Register
  Machines…**, select `localhost`, and on the **Codes** tab enter the
  program, for example `/opt/orca/orca_6_1_1/orca`. Then **Save**.
* **Where this is kept.** Your settings for `localhost` are in
  `~/.ECCE/CONFIG.localhost`. ECCE creates that file at your first start
  and never replaces it afterwards, so your changes stay. On an
  installation where the administrator has set up `localhost` for
  everyone (a `CONFIG.localhost` in the installation's `siteconfig`), ECCE
  starts from that one; what you save in Register Machines is added on
  top of it.

For any other machine, open **Tools → Register Machines…** in the
Organizer. It has five tabs:

* **Machine**: the machine's name as ECCE uses it, its real host name,
  and the numbers of nodes and processors. Vendor, model and processor
  are labels only.
* **Connection**: how ECCE reaches the machine; exceptions to the
  default are under *Advanced*.
* **Codes**: where each code is installed on the machine; a code with
  no path is not offered for it.
* **Job script**: the job script's setup text and submit directives. A
  value is tagged *from site*, *your value* or *not set*. The site's text
  is read-only; **Copy site text to edit** makes your own copy,
  **Available words…** lists the `$variables` a script may use, and
  **Advanced: edit file…** opens the underlying `CONFIG` file. Changed
  fields have an undo button.
* **Queues**: the queue manager and the queues. Wall time is entered in
  hours, memory and scratch in GB.

Changes are kept until you press **Save**.

ECCE's ssh to the machine is built in; no terminal session is involved.
Queues and submit scripts for a cluster are described in
[GETTING_STARTED.md](../GETTING_STARTED.md#describing-the-queues-on-your-own-cluster).

## Deployment modes

ECCE is a client and a server even on one workstation. How those are
shared is up to the site; the three modes are set up step by step in
[GETTING_STARTED.md](../GETTING_STARTED.md#deployment-modes).

1. **Everything local** (the default). Each user's session starts their
   own data server and broker; the broker listens on a Unix socket, not
   on a network port. Nothing to configure. The data server can be
   replaced by a folder (Edit → Preferences → Data folder), which is the
   default on macOS and Windows.
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

**Before putting a server on a network**: a central or shared broker
takes the data server login as its account and lets a user touch only
that user's own topics, but the broker's and the data server's passwords
(HTTP Basic) cross the network unencrypted unless TLS is set up. Since
9.0.0-alpha.7, `ecce-remote-setup --tls` on the server and the clients
encrypts both: the data server on port 8443 (HTTPS) and the broker on
port 8883 (MQTT over TLS), with the plain ports on loopback only
([#236](https://github.com/FriendsofECCE/ECCE/issues/236)). Without TLS,
keep the ports on loopback and use ssh tunnels, or firewall them (#138).
