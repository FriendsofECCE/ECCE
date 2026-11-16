---
type: pitfall
title: "A UTF-8 buffer can borrow its wxString storage"
area: wx-viewer
paths: [src/apps/organizer/TextFileViewer.C, tests/apps/organizer_file_view_test.py]
issues: [1084]
---
In a wxWidgets UTF-8 build, `wxString::ToUTF8()` returns a
`wxScopedCharBuffer` that can borrow the string's internal bytes. Saving
`control->GetValue().ToUTF8()` in `auto` leaves a dangling buffer once the
full expression destroys the temporary wxString. Retain the wxString in a
local variable until the buffer has been consumed, or copy to an owning
string in the same full expression.

The Organizer viewer's `view-text` test hook writes the actual text control,
not the reader's raw buffer. `tests/apps/organizer_file_view_test.py` compares
both a 64 KiB tail and a short replacement file byte for byte; corrupted
prefixes in those snapshots exposed this lifetime error.
