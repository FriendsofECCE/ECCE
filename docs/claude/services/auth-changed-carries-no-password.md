---
type: rule
title: "`ecce_auth_changed` carries url, user and realm, never the password"
area: services
section: "Pitfalls"
paths: ["src/tdat/resources/AuthCache.C", "src/wxgui/wxdialogs/WxDavAuth.C"]
issues: [194, 213]
---
**`ecce_auth_changed` crosses the broker, so it carries no password.**
`AuthCache::addAuthentication` saves the credential to the session store
(`authcache_<host>_<display>`, 0600) *before* publishing, and
`AuthCache::msgIn` -- the one receiver all 18 subscribers call -- reads the
password back with `sessionLookup()`. A receiver for which the lookup fails
(url not `sessionWorthy`, store already swept) ignores the message. Do not
add the password back to the message, and do not publish before
`sessionSave()`.
