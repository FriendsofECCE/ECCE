# Package install tests

Both need rootless podman and the network. Neither is a ctest.

## `install_smoke.sh <package> <image>`

One package in a clean container, the data server started as an ordinary
user, WebDAV answering. Quick: it catches missing dependencies and
apache2/httpd differences.

## `install_matrix.py` (#193)

The packages users get (the CI artifacts) on each distro's official base
image, and every deployment mode on top of them.

    gh run download <run-id> -D DIR        # or: install_matrix.py --run <id>
    tests/packaging/install_matrix.py --pkgs DIR [--distro trixie,rocky9]
        [--only install,local,central,tls,broker] [--logdir DIR] [--keep]
        [--jobs N] [--reuse] [--label TEXT] [--overlay FILE:/PATH]

`DIR` holds one directory per artifact (`ecce-debian-trixie-deb`,
`ecce-ubuntu-latest-deb`, `ecce-rockylinux9-rpm`, `ecce-fedora-latest-rpm`).
The distros run one at a time (`--jobs N` for more; the downloads are big),
each with its own container network. A mirror error is retried once.
`--overlay FILE:/PATH` copies a file over the installed one before the modes
run, to try a fix without rebuilding the package (say so in `--label`).

Per distro:

1. **install**: `ecce-client` and `ecce-server` installed from the local
   files by apt or dnf, so dependencies come from the distro's repositories.
   On Rocky a first try without EPEL (its error is kept verbatim), then with
   `epel-release` as GETTING_STARTED says, then with CRB. The container is
   committed; later steps start from it. Which package owns each command,
   the systemd unit and the dependency fields are recorded.
2. Test tools (Xvfb, xdotool, python3-xlib, systemd) are added on top. They
   are not ECCE dependencies.
3. **local**: `ecce-dataserver-start`, `ecce-dataserver-adduser`, `ecce` on
   Xvfb, login, Organizer, quit ends every ECCE process
   (`tests/containers/guest.py` drives the window).
4. **central**: a server container (`ecce-remote-setup --server all`,
   services, an account) and a client (`ecce-remote-setup srv`,
   `ecce -remote`, login, Organizer "on srv", PROPFIND in the server's
   access log, quit).
5. **tls**: the same with `--tls` and `--fetch-pin`.
6. **broker** (mode 3): systemd as PID 1, `ecce-broker-setup`,
   `systemctl link` and `enable --now`, publish as the account, an
   intruder's publish not delivered.

It writes `matrix.md` (distro by mode, each failure with its message, the
install attempts, package facts, timing) and `results.json` to `--logdir`
(default `tests/packaging/logs`), plus container logs. Exit 0 all passed,
1 a check failed, 77 podman or the artifacts missing. A distro takes about
10 minutes on an idle machine (the first install, which downloads and
unpacks nwchem and Coin3D, is most of it; the modes take 2 to 3); `--reuse` skips the install step with the images of an
earlier run.

The checks of `tests/containers/central_server_test.py` (isolation between
accounts, the broker's delivery rules, session end) are deeper but Debian
only; this one asks whether each distro's packages install and the modes
start at all.
