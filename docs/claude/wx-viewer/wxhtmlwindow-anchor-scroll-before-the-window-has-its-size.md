---
type: pitfall
title: "`wxHtmlWindow::LoadPage(\"x.html#anchor\")` scrolls in a layout made before the window has its size"
area: wx-viewer
section: "wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls"
issues: [219]
paths: [src/wxgui/wxdialogs/WxHelpViewer.C]
---
A new help frame loads the page before it is shown. `LoadPage` lays the text
out in a window 20 px high and 0 wide, finds the anchor (y = 4800 for
`#3-build-the-molecule`) and scrolls there. When the frame is shown the window
is 675 px wide, the text reflows (anchor now at y = 795) and the scroll offset
stays, so the view sits 2000 px past the heading. The page and the anchor
reported are right; only the offset is wrong.

`WxHelpViewer::scrollToAnchor()` finds the anchor cell again
(`GetInternalRepresentation()->Find(wxHTML_COND_ISANCHOR, ...)`) and scrolls to
its y. It runs by `CallAfter` after each `load()` and from the html window's
`wxEVT_SIZE`, until the window is shown on screen with a real width.
`tests/apps/help_test.py` reads `viewState()` from the test hook and requires
the heading within 100 px of the top of the view.
