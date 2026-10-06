---
type: rule
title: "Central-server TLS: ports, who is on loopback, and what must stay in step"
area: services
section: "Pitfalls"
paths: ["packaging/dataserver/ecce-remote-setup", "packaging/dataserver/ecce-dataserver-start", "packaging/dataserver/httpd.conf.ecce", "packaging/gateway/ecce-gateway-start", "src/util/jms/MqttLink.C", "tests/tls/server_tls.py"]
issues: [236]
---
**TLS is opt-in per server (`ecce-remote-setup --server --tls`) and per client
(`--tls --pin|--fetch-pin|--system-ca`); nothing detects it and nothing falls back.**
- Server state is `~/.ECCE/tls/{server.pem,server.key,enabled}`, shared by the
  data server and the broker (not under `~/.ECCE/dataserver`, which is
  Apache's ServerRoot). `enabled` is what both start scripts read.
- With TLS on, the plain ports (8096, 8088) are bound to 127.0.0.1/::1 only
  and the TLS ports (8443, 8883) go on the listen setting's addresses *and*
  loopback. A wildcard `Listen 8443` cannot be combined with
  `127.0.0.1:8443` (address in use), so `all` gets the wildcard alone.
- The client learns TLS from the `https://` URL in `RemoteServer/DataServers`;
  `ecce-gateway-start` takes the broker's port (8883) and the
  `tls=1`/`cafile=`/`pinned=1` keys of the broker file from it. The pin is
  `siteconfig/RemoteServer/server.pem`; without it the system CA directory
  and host-name check apply.
- Pinned mode checks the chain against the one certificate and ignores the
  host name (`mosquitto_tls_insecure_set`, curl `--insecure
  --pinnedpubkey`): never use curl `--insecure` without the pin.
- A failed handshake reaches `MqttLink` only as a libmosquitto log line
  (`onLog`), not a CONNACK; the refusal text starts with `TLS:`.
- `ecce-remote-setup --tls` probes the server and writes nothing if the
  certificate is not trusted (curl exit 90/60/35); only an unreachable
  server (7/28) is a warning.
- Mode 3 (`siteconfig/SharedBroker`) has no TLS.
- `ctest -R tls_server` runs the whole thing on loopback; it needs
  mod_ssl and, for the network-address checks, a non-loopback IPv4 address.
