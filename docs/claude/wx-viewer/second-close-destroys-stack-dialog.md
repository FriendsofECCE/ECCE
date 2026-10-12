---
type: pitfall
title: "A modal dialog on the stack, opened from a close handler, is destroyed as a child if a second close runs inside its loop (#291)"
area: wx-viewer
paths: [src/apps/organizer/CalcMgr.C, tests/apps/organizer_close_test.py]
issues: [291]
---
A second close request while `confirmAndQuit()`'s dialog is open runs a second
dialog in the first one's event loop. Answering the first calls `Destroy()` on the
frame, and the idle pass inside the nested loop deletes it; its destructor
destroys its children, one of which is the first dialog, a stack object:
`free(): invalid pointer`, abort. `p_quitAsking` vetoes a close while the dialog
is open and `p_quitConfirmed` lets one after the answer through without asking.
The same shape applies to any `ShowModal()` on a stack dialog whose parent can
be closed meanwhile. Reproduce: hook commands `close 2`, `answer ok`;
`tests/apps/organizer_close_test.py`; an ASan build names the frame.
