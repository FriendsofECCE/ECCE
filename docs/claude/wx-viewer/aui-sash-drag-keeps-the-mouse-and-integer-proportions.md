---
type: pitfall
title: "wxAUI 3.2 sash drag: live resize can end a drag inside a motion and capture the mouse again (window takes no clicks); proportions 1/100000 make the drag move nothing; a Fixed pane has no sash"
area: wx-viewer
paths: ["src/apps/builder/BuilderPanels.C", "src/apps/builder/Builder.C", "src/apps/builder/BuilderPbcTest.C", "tests/apps/builder_sash_test.py"]
issues: [328]
---
From wx 3.2.8 `src/aui/framemanager.cpp`, all three seen in the Builder's
tool column (Open structures, Build, Selection, Atom Table, Symmetry):
- GTK3 always live-resizes. `OnMotion` does `ReleaseMouse();
  DoEndResizeAction(); CaptureMouse();` for every motion. When no resizable
  pane follows the dragged sash (or the proportions sum to 0),
  `DoEndResizeAction` sets `m_action = actionNone` and returns, and the
  capture is taken again. `OnLeftUp` releases only for `actionResize`, so the
  frame keeps the mouse: resize cursor, no clicks, main loop idle (gdb
  shows `ppoll`). A release that never reaches AUI does the same. Seen on
  XWayland after some drags, on native Wayland on every drag.
- A pane sizer belongs to the pane above it, and `OnLeftDown` ignores the
  sash of a `Fixed()` pane: Open structures was fixed, so the sash above
  Build could not be grabbed.
- AUI turns the drag into integer `dock_proportion`s
  (`new_pixsize * total / dock_pixels`) and borrows only from the next
  resizable pane. The column used 1 (stay at the floor) and 100000 (take
  the rest; AUI's default for 0), so a dragged share rounded to nothing and
  space moved between the wrong panes.
- `fitToolColumn()` scales the panes' floors down until they fill a short
  column: then every pane is at its minimum and no sash can move at all.

`Builder::guardSashDrag()` binds on the manager (dynamic binds run before
AUI's own table): on a press it sets each resizable column pane's
proportion to its whole height (content, caption, border); a release, or a
motion without the button, that finds the frame still holding the capture
ends it (`ReleaseMouse` and a `wxMouseCaptureLostEvent` to the manager).
`ECCE_DEBUG_SASH=1` reports each forced release.
`sashdrag <tool> <dy> [lost]` (ECCE_VIEWER_SCENE) drags through the frame's
event chain; `builder_sash_test.py` runs at 1920x1200 so the column has
room above its floors.
