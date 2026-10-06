# Central-server test in rootless podman (#213 stage 5)

`central_server_test.py` runs the central-server deployment (mode 2) on a
container network, which `tests/apps/session_end.py remote` cannot: that one
runs server and client as one Unix user on one machine.

| container | role |
|-----------|------|
| `srv`     | account `ecce`: `ecce-remote-setup --server all`, `ecce-dataserver-start`, `ecce-gateway-start`, `ecce-dataserver-adduser` (data server + Mosquitto with `ecce_users_auth.so` and `mosquitto.acl`) |
| `alice`, `bob` | one Unix account each, own Xvfb; `ecce-remote-setup srv`, then a real `ecce -remote`, login dialog typed into |
| `sysb`    | systemd as PID 1, `ecce-broker.service` (mode 3) enabled with `ecce-broker-setup` |

Checks: login and Organizer per client; her own data created and read back
over WebDAV; WebDAV isolation (the other account, anonymous, wrong
password); broker isolation **by delivery** (mosquitto clients as an
independent oracle, five wildcard subscriptions for bob, controls that the
messages do arrive for their owner, refused logins); alice's quit leaves
bob's session, the server's services and message delivery alone; the same
delivery checks against the systemd broker plus start, reload, kill -9
restart, stop, start.

## Run

    cd build-cmake && cpack -G DEB          # ecce-client and ecce-server
    tests/containers/central_server_test.py [--debs DIR] [--logdir DIR]
                                            [--keep] [--only part,...]

Exit 0 all passed, 1 a check failed, 77 podman or the packages missing. It
is not a ctest (a first run builds the image, several minutes; a run takes
about 5 minutes: 314 s measured). Run it before a release next to `tests/teaching`. The image
is keyed to the packages and the Containerfile; apt needs the network when
it is built. Logs (container logs, broker and data server logs, the
clients' session logs, the journal of `ecce-broker`) go to `--logdir`
(default `tests/containers/logs`).

`sysb` is started with `--cap-add SYS_ADMIN`: without it systemd cannot build
the unit's mount namespace in a container (226/NAMESPACE) or drop to its
User= (217/USER). The unit file itself is used unmodified.

Not covered: two machines on a real network (the container network is
one), TLS (not implemented), `ecce -admin -remote` over ssh.
