---
type: pitfall
title: "Windows: the session key's host is %COMPUTERNAME%; the DNS host name differs in case and splits the topics"
area: services
paths: ["src/util/genutil/Ecce.C", "packaging/gateway/ecce-session-lib.sh", "packaging/windows/ecce.cmd", "tests/session/session_host_test.C"]
issues: [1287, 233]
---
The session key is `<host>_<id>` (`Ecce::sessionKeyFor`), and it names both
the files in `~/.ECCE` (`session_<host>`, `broker_<key>`) and the MQTT topic
level `ecce/<user>/<key>/...`. `ecce.cmd` sets `HOST=%COMPUTERNAME%` (upper
case); `GetComputerNameEx(ComputerNameDnsHostname)` keeps the configured case
(`5500-myha...` against `5500-MYHA...`). A tool started without `HOST` (an
`ecce-*` command from a Command Prompt) therefore found the pointer and broker
files, since NTFS ignores case, connected to the Organizer's broker with the
file's login, and published under a key the Organizer does not subscribe to:
"ECCE is not running" after the ack wait, no error anywhere. Without `HOST`,
`Ecce::sessionHost` and `ecce_session_host` now use `COMPUTERNAME` on Windows.
Test: `ctest -R session_host` (Windows; `session_host_test.exe` is in the
`windows-test-drivers` artifact).

A `mosquitto_sub` probe of the built-in broker says "unacceptable protocol
version": that is mosquitto_sub's default MQTT 3.1.1 against a 5-only broker
(`-V 5` connects), not a fault.
