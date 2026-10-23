---
type: rule
title: "`ecce_auth_changed` carries no data server password; a compute password only on the Unix-socket broker"
area: services
section: "Pitfalls"
paths: ["src/tdat/resources/AuthCache.C", "src/wxgui/wxdialogs/WxDavAuth.C"]
issues: [194, 213, 487]
---
**`ecce_auth_changed` crosses the broker, so it carries no password.**
`AuthCache::addAuthentication` saves the credential to the session store
(`authcache_<host>_<id>`, 0600) *before* publishing, and
`AuthCache::msgIn` -- the one receiver all 18 subscribers call -- reads the
password back with `sessionLookup()`. A receiver for which the lookup fails
(url not `sessionWorthy`, store already swept) ignores the message. Do not
add the password back to the message, and do not publish before
`sessionSave()`.

A compute machine's password (`ssh://`, cached by `RCommand::setPassCache`)
is never in the session store, so without the message it reaches no other
process: each Launcher is a new process, gets its cache over `-pipe` from
calced/organizer, and asked at every submit (#487). `addAuthentication` adds
it as `auth` only when `AuthCache::privateBroker()` (the account's own
mosquitto on a Unix socket, local mode), and `msgIn` takes `auth` only for a
non-http url on such a broker. On a TCP broker (central server, shared
broker, the Windows local broker) it is still not sent. Test:
`authcache-relay` (ctest).
