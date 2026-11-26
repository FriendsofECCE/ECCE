---
type: pitfall
title: "AddDragger/AddWheel reach every Builder tab, and a foreign dragger indexes the wrong fragment (#1306)"
area: services
paths: ["src/apps/builder/Builder.C", "src/viz/sgcommands/AtomMoveCmd.C", "src/viz/sgcommands/BondRotateCmd.C", "src/viz/sgcommands/SGContainer.C", "include/viz/SGContainer.H", "src/viz/sgcommands/SGFragment.C", "src/tdat/chemistry/Fragment.C"]
issues: [983, 1305, 1306]
---

`EventDispatcher` is process-wide and most of its events carry no sender.
Since one-program mode (#983) every Builder and Viewer tab lives in the
same process, so an event meant for one tool reaches all of them.

`AtomMoveCmd::execute` publishes `AddDragger` with the new `AtomRTDragger`,
`BondRotateCmd::execute` publishes `AddWheel` with the `AtomRotDragger`.
`Builder::eventMCB` attaches `moveCB`/`rotateCB` plus the start and finish
callbacks to whatever dragger arrives, so before #1306 every open Builder
attached its own callbacks to another tab's dragger.

What that costs in the receiving tab is not cosmetic:

* `moveCB` takes the *receiving* Builder's fragment and looks the dragger's
  selected atoms up with `Fragment::atomIndex`, whose comparison is TAtm
  identity (`TAtm::operator==` returns `this == &atom`). A foreign atom is
  therefore never found and `atomIndex` returns `INT_MAX`.
* `SGFragment::getAtomCoordinates` checks the index with `EE_RT_ASSERT(...,
  EE_WARNING, ...)`, which only warns, and then evaluates
  `p_atoms[INT_MAX]` — an out-of-range `std::vector` read.
* `moveEndCB` publishes `GeomChange`, which is what marks a tab modified
  (#1305).

The scoping pattern: the scene graph names itself with
`SGContainer::eventId()` in the event's sender id and the Builder ignores a
dragger from another one. `SyncToggle` (`ViewerEvtHandler::syncToggle`)
scopes the same way through the event object. Any new process-wide event
that carries a pointer into one tab's data needs the same treatment.

Not reproduced in a running ECCE. The Builder's script hooks cannot drive a
drag: both the vendored SGI `SoDragger` and Coin invoke `motionCallbacks`
only from a real `SoLocation2Event` handled while that dragger is the
handle-event action's grabber (`src/inv/interaction/SoDragger.C`), and
`motionCallbacks` is private, so writing the dragger's `translation` field
moves the geometry without running any callback. A reproduction needs a
hook that drives `SoHandleEventAction` with a synthetic location event and
the dragger set as grabber.
