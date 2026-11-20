---
type: pitfall
title: "A new tool tab is painted empty for about a dozen frames before its tool draws; freezing the main window in adopt() does not help (#1210)"
area: wx-viewer
paths: ["src/wxgui/jms/ToolTabs.C", "tests/apps/one_process_test.py"]
issues: [1210, 983]
---
The flash when a Viewer tab opens is not the synchronous page change.
`paintlog` (a wxEventFilter on wxEVT_PAINT, see docs/AGENTS.md) on Xvfb
shows, after the Organizer page, about 14 frames of the main window with
only the new page and its empty body (`wxPanel@858x734`, `wxPanel@858x710`)
before the Viewer's own windows (`SoWxExaminerViewer`, `wxGLCanvas`) paint
for the first time. The page is selected and shown while the tool is still
loading, so the user sees a blank tab, then the tool.

`wxWindowUpdateLocker` on the host frame inside `ToolTabs::adopt()` and
`release()` changed nothing: adopt runs without an event loop pass, so no
paint can happen inside it anyway; the empty frames come from the event
loop turns after it, while the tool fills its page. A fix has to keep the
page hidden or unselected (or the body frozen) until the tool has drawn,
e.g. until `ecce_tool_up` or the tool's first paint, not freeze around
adopt().

Closing a tab: the old page is repainted once or twice, then the Organizer
page; no empty intermediate state was seen. The Organizer page then keeps
repainting a few times a second for a while in some runs (ProjectContextPanel,
WxGridView), independent of the tab code.
