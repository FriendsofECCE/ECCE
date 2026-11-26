---
type: pitfall
title: "Builder GeomChange/ResidueChange/LatticeChange are process-wide and carry no sender; a Builder ignores them while another Builder is the publisher"
area: wx-viewer
paths: ["src/apps/builder/Builder.C", "tests/apps/builder_dirty_scope_test.py"]
issues: [1307, 1305, 1306]
---
The `EventDispatcher` is one per process, and in one-program mode (#983)
every Builder and Viewer tab listens to it. `GeomChange`, `ResidueChange`
and `LatticeChange` are published as a bare `Event(name)`: no sender, value
or object. `Builder::eventMCB` marks its own calculation modified on them,
so a Viewer tab loading another calculation (its `setContext` publishes
`GeomChange`) made an unedited Builder ask "Save Builder Changes?".

The fix is a mark, `g_publishingBuilder` (anonymous namespace in
`Builder.C`), set by the RAII `PublishingBuilderMark` (saves and restores
the previous value, so nesting works) in `Builder::execute`,
`Builder::setContext`, `Builder::moveEndCB` and `Builder::rotateEndCB`.
In `eventMCB`, if the mark is set and is not `this`, the whole branch
(dirty flag, `updateSave`, `updateForAnyEdit`) is skipped: it all acts on
this Builder's own fragment. Mark and read are GUI-thread only (logged and
asserted).

Still unscoped, with today's behaviour: panels that publish these events
directly (PBC, Slice, Trajectory, Scan, PointCalc) have a null mark, so
every Builder reacts; the PBC, Slice and Symmetry panels and
`ViewerEvtHandler` also hear other tabs' events (they refresh rather than
set a dirty flag; not established by a run). An edit that publishes
nothing is #1305; drag events being process-wide is #1306.

Fragile: a Builder's own load also sets its own dirty flag through
`eventMCB` (its `setContext` is marked as itself); `setContext` then
clears it with `setDirty(false)` after the load. Reordering that, or
dropping the clear, would make every Builder dirty on open.
`tests/apps/builder_dirty_scope_test.py` covers the negative case and two
controls (a real edit with a Viewer open, and a Viewer opened after the
edit) that must still ask.
