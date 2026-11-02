---
type: pitfall
title: "AuthCache offers an ssh password to its own host[:port] and user only, under every policy"
area: services
paths: ["src/tdat/resources/AuthCache.C", "src/comm/commxt/JobStore.C", "src/comm/rcommand/RCommand.C", "tests/mqtt/authcache_host_test.C"]
issues: [791]
---
The job store sets `LAST_URL`, which once returned the newest cached
non-http credential for any `ssh://` lookup that missed, and the default
`ANY_USER` returned another user's password for the same host. Either sent a
password to an sshd (or account) it was not typed for, and a refusal was
then reported as a wrong password for that host.

- Both widenings now apply to `http(s)://` keys only (data-server realms).
  An ssh lookup is the exact key `ssh://host[:port]/` + user, or nothing,
  so the login asks (one dialog per host, #684).
- No legitimate path needed the widening: launch, job store and catch-up all
  key the machine by `RefMachine::fullname()`, the front end by its own
  `frontendMachine()` name, SSH folders by `passHost` (host:port, #789).
- Test: `ctest -R authcache-host`.
