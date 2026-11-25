---
type: rule
title: "SSH folder listing runs on a worker thread; the cache is per thread, so the GUI seeds it; no wxYield while waiting (#1220)"
area: services
paths: ["src/apps/organizer/CalcMgr.C", "src/dsm/edsiimpl/SftpEDSI.C", "include/dsm/SftpEDSI.H", "tests/apps/ssh_listing_async_test.py"]
issues: [1220, 932, 1082, 1272]
---
- `CalcMgr::onSelectionChange` on an `sftp://` node used to run the fill
  script on the GUI thread (window frozen, no progress). Now `startListing`
  runs `SftpEDSI::fillListing` on a detached worker; a 200 ms wxTimer
  (`listingTick`) applies the result, shows the progress panel after 1 s, and
  drops it when the selection or `SftpEDSI::generation()` changed.
- `gCache` and `gBatchDepth` are `thread_local`: a worker's reads never reach
  the GUI thread's Batch. The worker therefore returns only the raw script
  text, and the GUI thread opens a Batch, calls `seedListing`, and runs the
  unchanged selection code, which then reads everything from memory. Anything
  that makes the worker fill a cache itself is lost.
- Never wait with `wxYield` or a pump loop: a timer-driven reload (#889, hook
  timers) frees Resources the callers up the stack still hold (#932).
- The worker has `tInteractive == 0`: no password prompt, existing login only.
  A failed login or script shows its message and leaves the node unloaded.
- While a fill runs, the GUI must not call anything that waits on `gMutex`
  for that host: `OnTreectrlItemExpanding` vetoes the node being listed
  (it expands when the result arrives) and any unloaded `sftp://` node
  (`SftpEDSI::listingsInFlight()`). `folderStamp` only try-locks.
- A cancelled worker still finishes (up to 60 s) and holds the login mutex;
  its text is ignored.
- Only the tree's own selection event (`OnTreectrlSelChanged`, via
  `p_asyncOk`) starts a worker. `refresh()` and the auto-refresh tick raise
  `p_noAsync` and list synchronously: their follow-up code runs against the
  panel they just rebuilt, and a poll must not relabel the node.
- While a listing is pending the desk is hidden, a "Listing NAME..." message
  is up and `updateSelectionMenus` takes its nothing-selected branch.
- A changed server generation causes one restart (`p_restartNext`); a second
  miss is applied on the synchronous path.
- Quit: `gMutex` and `gLogins` are heap objects that are never destroyed, so a
  worker still in `runRemote` at exit touches live memory, and `~CalcMgr`
  waits at most 2 s on `SftpEDSI::waitListings` without pumping events.
