---
type: map
title: "One GUI process (#983 stage 1): in-process message routing, OnInit split, globals audit, window lifetime"
area: services
section: "Pitfalls"
paths: ["src/util/jms/MqttLink.C", "include/util/MqttLink.H", "src/util/jms/JMSPublisher.C", "src/wxgui/jms/WxJMSSubscriber.C", "src/wxgui/jms/WxJMSMessageDispatch.C", "src/apps/organizer/CalcMgrApp.C", "src/apps/builder/BuilderApp.C", "src/apps/calced/CalcEdApp.C", "src/dsm/edsiimpl/EDSIFactory.C", "tests/mqtt/mqtt_test.C", "src/util/jms/JMSPublisher.C", "CMakeLists.txt"]
issues: [983, 1007, 990, 213]
---
Organizer, Builder and Calculation Editor become windows of one wx process
(stage 1); the other tools, the job programs and the `ecce-*` commands stay
separate processes on the broker. Callers of `JMSPublisher`/`JMSSubscriber`/
`WxJMSSubscriber` do not change; the routing is inside `MqttLink`.

## Routing (`MqttLink`, switch `ECCE_ONE_PROCESS=1`)

Off (the default): exactly as before. Every message goes to the broker;
a process's own messages come back from it unless every subscriber of the
topic asked for No Local; `MqttEndpoint::receive` drops `senderid == pid`
for `filterSelf` subscriptions.

On (`MqttLink::localDelivery()`):
- `publish()` first hands the message to every endpoint of this process
  that has the topic (`deliverLocal`: same `MqttInbound` the broker path
  builds, same `receive` -> `poster` -> `CallAfter`, so callbacks run on
  the GUI thread, after the publisher's handler returns, in publish
  order), then publishes to the broker for the other processes.
- Every broker subscription is made with No Local, so the broker never
  echoes this process's messages back: each message reaches each
  in-process subscriber exactly once, by the local path. That holds for
  the `ecce/+/ecce_machreg_changed` wildcard subscription too.
- Session and user classes need no check locally: one process is one
  session and one account.
- "Self" for `filterSelf` becomes the *tool*, not the process:
  `senderid == pid` **and** `sendername == endpoint name`. Without that,
  the Organizer (filterSelf on `ecce_url_property`, `ecce_url_created`,
  ...) would drop the Builder's notices.
- Local delivery works without a broker (sessionless, broker down, no
  login yet); `publish()` returns true if any local subscriber took it or
  the broker accepted it.
- `onMessage` takes a snapshot of the endpoints and releases the link lock
  before `receive`: an `arrival` hook publishes, and the local path takes
  the same lock.

Test: `ctest -R mqtt-local` (`mqtt_test local`, no GUI): in-process
delivery, tool-scoped self filter, broker -> in-process, in-process ->
another process (a raw client on the socket), no duplicates (counts after
a sentinel), wildcard, ordering of 200 messages, delivery with the broker
stopped.

## Tool instance ids and notice senders (done)

- **Ids.** With the switch on, every `MqttEndpoint` gets
  `MqttLink::newInstanceId()` = `<pid>.<n>`; `JMSSubscriber`/
  `WxJMSSubscriber::getMyID()` return it and `WxJMSMessageDispatch` stamps
  it on its publisher (`JMSPublisher::setMyID`), so the invoke protocol
  (`getAppMCB`, `pollReplyMCB`, `ecce_invoke_status`, `ecce_poll_reply`)
  addresses one window. `receive` accepts a target equal to its own id or
  the bare pid (a sender without instance ids reaches every tool of the
  process). The self filter matches its own instance id, or the bare pid
  plus the tool name (a plain `JMSPublisher` of the tool). `sscanf("%d")`
  of `<pid>.<n>` still gives the pid where the dispatcher watches a child.
  Off: the id is the pid, everything as before.
- **`EDSIFactory`'s publisher** was named "EDSIFactory" (nobody passed a
  name), so in one process no tool's self filter would drop its own
  `ecce_url_*` notices. It now follows the *acting tool*:
  `JMSPublisher::setActingTool(name, id)` (process default, the Organizer)
  and the scoped `JMSPublisher::ActingTool` (a window around its save).
  Only with the switch on. Not wired into the apps yet: the per-window
  Builder/CalcEd code sets it when they open in-process.
- Caveat: code that compares a sender id with `getMyID()` (e.g.
  `Builder.C` `msg.getSender().getID() == getMyID()`) expects messages
  sent through its own dispatcher; a plain `JMSPublisher` in the same tool
  stamps the bare pid. Check each such site when its tool moves in-process.

Test: `ctest -R mqtt-local` adds instance targets, bare-pid targets, the
instance self filter and EDSIFactory notices as Builder/Organizer; `mqtt`
checks ids and the notice sender are unchanged with the switch off.

## Open in one process (next increments)

- **`ecce_get_app`.** The Organizer intercepts requests for in-process
  tools (`builder`, `calced`) and opens a window instead of starting a
  program; `ecce_poll` reuse works as today (the open window answers it
  through the local path). Other tools still go to the gateway
  (Linux/macOS) or `startToolWin` (Windows).
- **`ECCE_ONE_PROCESS` is inherited by child processes**; the starter
  must unset it.

## Build (done)

`organizer-objs`, `builder-objs` and `calced-objs` are CMake OBJECT
libraries (`ecce_tool_objects`): each tool's directory without its `*App.C`
(`IMPLEMENT_APP`). The five Organizer files the Builder also uses
(`WorkflowSteps.C`, `Recipe.C`, `KeyData.C`, `CompareEnergies*.C`) are
`organizer-shared-objs`, compiled once and linked into both programs. The
programs `organizer`, `builder`, `calced` are `ecce_app(... MAIN <App.C>
OBJECTS ...)`: same names, same install, same link libraries.
- OBJECT, not STATIC: a static archive drops objects nothing references,
  and some are reached only through static registration.
- The programs take `$<TARGET_OBJECTS:...>`, not `target_link_libraries`
  on the object library: its link interface would put the ECCE libraries
  outside the Windows `LINK_GROUP:RESCAN`, which CMake refuses. The object
  library links them PRIVATE only for include paths and definitions.
- Object order in the link changed (the `*App.C` comes first); static
  initialisation order across files is unspecified anyway, but a crash at
  start after this change would point there.
- A one-process program links `organizer-objs builder-objs calced-objs
  organizer-shared-objs` once each; duplicate symbols between the tool
  directories show up only then.

## OnInit split

Once per process (stays in `CalcMgrApp::OnInit`, `ewxApp::OnInit`):
`ewxApp::OnInit` (art provider, locale, prefs), `-pipe` login
(`AuthCache::pipeIn`), the Windows local-data and splash steps,
`EDSIFactory::addAuthEventListener`, `startSubscriber` of the app-level
dispatcher, `notifyReady`.

Per window ("open this tool"): what `BuilderApp::OnInit` and
`CalcEdApp::OnInit` do after argument parsing -- `new Builder(NULL,
standalone)` / `new CalcEd(...)`, `registerTopShell`, `setContext`,
`subscribeMessages`. The tool's subscriptions move from the `wxApp`
subclass (`BuilderApp`/`CalcEdApp` are `WxJMSMessageDispatch`s) to a
non-app dispatcher owned by the frame, as `MDPrep` and `PartialCharge`
already do (`new WxJMSMessageDispatch(name, true)`). `ecce --tool builder`
(and the old `builder` program) run the once-per-process part, then open
that one window, so the `tests/apps` scripts keep a per-tool start.

## Globals audit (before Builder and CalcEd share a process)

`grep -E '^\s*static [^(]*(=|;)'` counts ~160 statics in
`src/apps/builder`, ~45 in `organizer`, 9 in `calced` (file and function
statics). Each gets one of: process-wide by design (caches of read-only
data), per window (move into the frame), or per tool (one per tool type).
Also:
- `exit()`: `BuilderApp::OnInit` (`-help`), and Builder's `_exit` paths
  (the help-capture test hook); none may run with other windows open.
- `ewxConfig::closeConfigs()` in `CalcEd` closes *every* config of the
  process, including the Organizer's; it must close its own only.
- Config names: `INIFILE` per app (`wxbuilder.ini`, `wxcalced.ini`,
  Organizer's); fine in one process as long as the names stay distinct.
- "Already running" checks: the gateway's `ecce_poll`; in one process a
  second Builder for the same calculation must be found by the local poll,
  not by a process scan.
- `wxTheApp` as the tool: `dynamic_cast<Builder*>(wxTheApp->GetTopWindow())`
  (`PullPanel.C`; the top window will be the Organizer), and
  `dynamic_cast<WxJMSSubscriber*>(wxTheApp)->deferMessages` around a
  launch (`LaunchSelected.C`, `ReconnectSelected.C`; it must defer the
  Organizer's dispatcher, not every window's).

## Window lifetime and Quit

- Closing a tool window destroys the frame and its dispatcher (unsubscribe
  -> `MqttLink::deactivate`); the process keeps running while the
  Organizer is open.
- Closing the Organizer (= Quit) asks each open window to close (its own
  save prompt; a refusal cancels Quit), then ends the process, which ends
  the in-Organizer broker (#990/#1007) once no job program or command is
  connected, as today.
- `ecce_quit` from the gateway closes all windows the same way.
- A crash now takes every window; autosave and reopen are stage-2+ work
  (#983 "Risks").

Central-server and shared-broker modes are unaffected: the switch only
adds a local path and No Local, and a client of a central broker keeps one
connection to it for the other processes and other users.
