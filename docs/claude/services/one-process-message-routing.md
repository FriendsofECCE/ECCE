---
type: map
title: "One GUI process (#983 stage 1): in-process message routing, OnInit split, globals audit, window lifetime"
area: services
section: "Pitfalls"
paths: ["src/util/jms/MqttLink.C", "src/wxgui/jms/InProcessTools.C", "include/wxgui/InProcessTools.H", "src/apps/builder/BuilderWindow.C", "src/apps/calced/CalcEdWindow.C", "src/apps/calced/CalcEdMessages.C", "tests/apps/one_process_test.py", "include/util/MqttLink.H", "src/util/jms/JMSPublisher.C", "src/wxgui/jms/WxJMSSubscriber.C", "src/wxgui/jms/WxJMSMessageDispatch.C", "src/apps/organizer/CalcMgrApp.C", "src/apps/builder/BuilderApp.C", "src/apps/calced/CalcEdApp.C", "src/dsm/edsiimpl/EDSIFactory.C", "tests/mqtt/mqtt_test.C", "CMakeLists.txt"]
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
  Only with the switch on. Wired into the apps in stage 1c (below).
- Caveat: code that compares a sender id with `getMyID()` (e.g.
  `Builder.C` `msg.getSender().getID() == getMyID()`, fixed in 1c) expects messages
  sent through its own dispatcher; a plain `JMSPublisher` in the same tool
  stamps the bare pid. Check each such site when its tool moves in-process.

Test: `ctest -R mqtt-local` adds instance targets, bare-pid targets, the
instance self filter and notices as Builder/Organizer; `mqtt` checks ids
and the notice sender are unchanged with the switch off. The notices go
through a `JMSPublisher` set up as `EDSIFactory::getPublisher` sets up its
own: `mqtt_test` must not link eccedsi, whose static initialisation fixes
`ECCE_HOME` and the preference path before `main` sets them.

## Tool dispatchers (done)

`BuilderMessages` and `CalcEdMessages` hold the handlers moved unchanged
from `BuilderApp`/`CalcEdApp`, which are no longer dispatchers. The window
owns its dispatcher (`setMessages`) and deletes it in its destructor;
`quitMCB` unsubscribes instead of calling the app's `OnExit`.

## Open in one process (done, stage 1c)

- **Switch scope.** `MqttLink::takeOneProcessSwitch(forThisProcess)` reads
  `ECCE_ONE_PROCESS` once and unsets it: the Organizer takes it for itself,
  the gateway (`GatewayApp::OnInit`) only remembers it. The starter
  (`appExec`, `startToolWin`) sets it for an `Organizer` child only, so
  job programs, `wxExecute`s and other tools never see it.
- **`ecce_get_app`.** `InProcessTools` (eccewxgui) keeps an opener per
  tool (`registerBuilderInProcess`, `registerCalcEdInProcess`, registered
  by `CalcMgrApp::OnInit` with the switch on) and the open windows.
  `CalcMgr::startApp`, and `getAppMCB` where the Organizer is the starter
  (Windows), call `InProcessTools::open`: unless `forcenew`, the window
  whose dispatcher's poll context is the request's `calcurl` is reused,
  else the opener makes a window + dispatcher (subscribed, registered top
  shell for the Builder). The request then goes to that window as its
  `ecce_invoke` over the local path, so the handler runs as in its own
  process (raise, `setContext`, `setPollContext`). Requests from other
  processes to the gateway (Linux/macOS) still start programs; a poll
  from the gateway is answered by an in-process window as before.
- **Acting tool.** `WxJMSSubscriber::deliver` wraps every callback in
  `JMSPublisher::ActingTool(name, id)` of its subscriber; a tracked tool
  window's `wxEVT_ACTIVATE` sets the default to its tool, the Organizer
  frame's sets it back. So a save from the Builder goes out as `Builder
  <pid>.<n>`: the Organizer takes it, the Builder's self filter drops it.
  `Builder::propertyChangeMCB`'s own-sender check accepts its dispatcher
  id or the bare pid with its own name.
- **Closing.** `Builder::quit` with `setInProcess(true)` destroys the
  window instead of `_exit(0)`; its close handler vetoes on Cancel, so
  `InProcessTools::closeAll` (Organizer Quit, after its own question)
  sees a refusal and cancels the Quit. CalcEd already `Destroy()`s.
- **Build.** The `organizer` program always links `builder-objs` and
  `calced-objs` (and the Builder's libraries, GL). Opting in at build time
  would need a second Organizer program; one program keeps one install.

Test: `ctest -R apps_organizer_one_process` (`tests/apps/one_process_test.py`):
two Builders and one editor opened by the Organizer's test hook
(`start TOOL URL`, `inprocess`, `closetools`), invokes traced in the
Organizer's pid (`ECCE_TEST_MESSAGES=1` prints each delivered message),
the Builder's save notice reaching the Organizer, Organizer alive after.

## Globals audit, Builder and Calculation Editor (stage 1c)

Fixed (break a second window or the Organizer):
- `ewxConfig::closeConfigs()` in CalcEd (6 sites) and `~Builder`: now
  `closeConfig(own .ini)` + `flushConfigs()`; no `ewxConfig*` is cached
  across calls anywhere, so the shared dialogs' `closeConfigs()`
  (`SaveExperimentAsDialog`, `ewxGenericFileDialog`) stay safe.
- `Builder::quit` `_exit(0)` (in process: `Destroy()`).
- `~Builder` `wxLog::SetActiveTarget(NULL)` and the constructor's
  `delete wxLog::SetActiveTarget(logWindow)` (deleted the Organizer's log
  target): in one process the Builder leaves the process log alone and
  makes no `ewxLogTextCtrl`; its Log panel then shows only what is
  written to it directly (stage 2: per-window log routing). An
  `ewxLogTextCtrl` pushes itself onto its text control as the event
  handler, so deleting it while the control lives leaves a dangling
  handler: SIGSEGV in `wxTextCtrl::GTKProcessEvent` when the window is
  first shown (realize), heap-dependent, so it can look like a crash
  elsewhere (it was taken for a crash at exit).
- `mainWindowCloseCB`'s function-static re-entry guard -> member.
- 12 timers "living until the process exits" that capture the window
  (test hooks, CalcEd's `ECCE_TEST_CALCED` poller, Builder script):
  `ewxWindowTimer(this)`, deleted on the window's `wxEVT_DESTROY`.
- `PullPanel::openAnalysis` found its Builder as `wxTheApp->GetTopWindow()`:
  now by walking parents.
- `LaunchSelected`/`ReconnectSelected` `deferMessages` on `wxTheApp`: no
  change needed; `wxTheApp` is the Organizer's app, which is the
  Organizer's dispatcher, the only one to defer.

Noted, not fixed (harmless now or test-only):
- Test hooks: `static bool ...Started` flags in `Builder.C` (13: script,
  import, dock, help, saveas, open, scene, metrics, ...) and CalcEd's
  `scriptStarted` run for the first window only; `_exit` in Builder test
  hooks (`Builder.C` import/dock/help/saveas paths, `BuilderPanels.C`
  clip audit) and CalcEd's `quit` test command end the whole process;
  `UvVisPanel`, `MoDiagramPanel`, `NModePanel` shot hooks close
  `wxTheApp->GetTopWindow()` (the Organizer in one process).
- `Builder::quit` unfocuses every `VizPropertyPanel::getPanels()` (process
  set): closing one Builder stops another's animation.
- `Builder.C` `internalSelect` (file static re-entry guard), serial
  counters (`Builder.C`, `PullPanel.C`, `CalculationFactory.C`), CalcEd
  `databuf` pipe buffers, `PropertyPanelFactory` singleton (stateless
  registry), constants in `PlotMetaXY.C`/`TrajectoryPanel.C`: process-wide
  by design.
- Builder static initialisation runs in the Organizer now (objects linked
  in); nothing seen to depend on being the Builder.
- Viewer (`CalculationViewer`, the Builder binary with
  `ECCE_INVOKE_VIEWER`) stays a separate program.

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

## Window lifetime and Quit

- Closing a tool window destroys the frame and its dispatcher (unsubscribe
  -> `MqttLink::deactivate`); the process keeps running while the
  Organizer is open.
- Closing the Organizer (= Quit) asks each open window to close (its own
  save prompt; a refusal cancels Quit), then ends the process, which ends
  the in-Organizer broker (#990/#1007) once no job program or command is
  connected, as today.
- `ecce_quit` from the gateway closes all windows the same way, forced
  (the session is ending): `CalcMgrApp::quitMCB` runs
  `InProcessTools::closeAll(true)` before destroying its frame, because
  each tool's own `ecce_quit` handler may run after it; those handlers
  return if their window is already `IsScheduledForDestruction`.
- A frame's close handler that already called `Destroy()` must not veto:
  `IsBeingDeleted()` stays false while it is only scheduled, so the
  Builder also checks `IsScheduledForDestruction`. The reverse for CalcEd:
  Cancel in its save question vetoes, or `Close()` reports success with
  the window still open.
- Exit crash in wx core's static destructors once the Organizer links the
  Builder: a class-name clash, see
  `docs/claude/wx-viewer/treelistctrl-name-clash.md`.
- The process ends only when no top-level window is left (wx exits the
  main loop with the last one), plus `OnExit`'s 10 s broker linger. A
  hidden parentless window a tool leaves behind keeps it running: the
  `inprocess` hook lists the remaining ones (`tlw=Class/label`).
- Test (`one_process_test.py` second run): Quit with a dirty Builder;
  `answer cancel` to its save question keeps both open, `answer no`
  must end the process. `answer` reports the dialog's title;
  `closetools` runs after the hook returns so a tool's question can be
  answered.
- A crash now takes every window; autosave and reopen are stage-2+ work
  (#983 "Risks").

Central-server and shared-broker modes are unaffected: the switch only
adds a local path and No Local, and a client of a central broker keeps one
connection to it for the other processes and other users.
