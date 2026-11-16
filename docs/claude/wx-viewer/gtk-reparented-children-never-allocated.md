---
type: pitfall
title: "wxGTK3: children reparented into a window whose size then never changes stay unallocated (blank, though wx has them laid out)"
area: wx-viewer
paths: ["src/wxgui/jms/ToolTabs.C"]
issues: [983]
---
GTK keeps its own allocation for each widget, separate from wx's
`GetPosition/GetSize`. `Reparent` unparents the widget (GTK resets its
allocation to `-1,-1 1x1`) and puts it into the new parent's pizza with the
wx rectangle; the widget is only allocated when that pizza is size-allocated.
If the new parent already has its final size and keeps it, that never
happens: the children are mapped, visible, wx-sized and laid out, and draw
nothing. `Layout()`, `Refresh()`, `SendSizeEvent()` and even
`gtk_widget_queue_resize` on the pizza do not help: wx skips `DoMoveWindow`
when its own size is unchanged, and the queued resize did not reach the
pizza's allocation. Anything that changes the parent's allocation cures it
(a scrollbar appearing, hiding and showing the notebook page).

Seen as a Calculation Editor tab (#983) that was flat grey on a screen where
its content fits, painted at 1366x768 where the scroller gets a scrollbar.
Diagnose with `tabinfo ...,gtk`: `first=...[alloc=-1,-1+1x1 ... wx=848x47]`.
Fix in `ToolTabs.C` `allocateChildren`: on idle (and after layout/select),
`gtk_widget_size_allocate` each shown direct child of the body whose GTK
allocation is still 1x1, at its wx rectangle (what wxGTK itself does within
a size-allocate).
