---
type: pitfall
title: "One process: the event dispatcher is process-wide; a closed tool's listeners stayed on it and the next Builder's first event called a deleted panel (#1088)"
area: services
paths: ["src/util/events/EventDispatcher.C", "src/util/events/Listener.C", "src/wxviz/viztools/SelectionPanel.C", "src/viz/propsgcommands/ShowFragCmd.C", "tests/apps/one_process_test.py"]
issues: [1088, 983]
---
- `ecce::EventDispatcher::getDispatcher()` is a singleton. With separate
  processes each tool had its own; with tools as tabs (`ECCE_ONE_PROCESS=1`)
  every Builder, panel and the Basis Set Tool share one list of `Listener*`.
- `Listener::~Listener` did not unsubscribe, and several listeners never do
  it themselves (`SelectionPanel`, the Builder's selection tools, has no
  destructor). A closed Builder left dangling pointers; the next Builder's
  `ShowFragCmd::execute` publishes `StepChange` during `Builder::Create`
  and the Organizer died in `EventDispatcher::publish`.
- Fixed in the base class: `~Listener` unsubscribes. `publish` walks a copy
  and skips listeners that left meanwhile, so subscribing (the vector
  reallocates) or destroying listeners inside `eventMCB` is safe. The old
  loop stopped delivering when the list shrank; now the rest still get it.
- Any other process-wide singleton holding pointers to tool objects has
  the same failure mode in one-process mode. Test: `runTabs1088`
  (`ECCE_1088_ONLY=1`) opens and closes Builder and Basis Set Tool tabs
  twice, then a Builder beside an editor tab.
