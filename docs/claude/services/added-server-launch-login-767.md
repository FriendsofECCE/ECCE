---
type: pitfall
title: "A job on an added server needs that server's login name in every process, not only the Organizer"
area: services
paths: ["src/util/genutil/ServerList.C", "src/tdat/resources/AuthCache.C", "src/dsm/edsiimpl/EDSIFactory.C", "src/apps/organizer/CalcMgr.C", "tests/launch/server_launch_test.py"]
issues: [767]
---
`ServerList::markAdded/setLogin` are process-local: only the Organizer calls
them. eccejobstore, the Launcher, Builder and Calculation Editor therefore
saw an added server's URL as "the session's server" and used
`Ecce::serverUser()` as the account, which found no password and then fell
into AuthCache's `LAST_URL` rule (newest http credential of any server).

- In a process that never calls `markAdded/clearAdded`, `isAdded()` and
  `loginFor()` answer from the `AddedServers` file (type ecce) plus
  `AuthCache::sessionLoginFor(ref)`: the credential piped in with `-pipe`
  (the whole memcache travels), else the session store. No new file or IPC.
- `AuthCache::getAuthentication` with `LAST_URL` takes, for an added server,
  only that server's own entry for the user; the primary path is unchanged.
- `EDSIFactory::create*` stamp the owner with `ServerList::userFor(url)`.
- `CalcMgr::refuseAddedLaunch` asks for the login (`connectServer`) when the
  session has none and refuses with a message if it is not given; the
  catch-up does the same for waiting jobs of an added server after a restart.
Test: `tests/launch/server_launch_test.py`, `server launchcheck` in
`tests/apps/server_roots_test.py`.
