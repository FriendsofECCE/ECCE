---
type: pitfall
title: "Calculation Editor: a state message must not undo MODIFIED, and a READONLY editor must reopen on a box merge (#964)"
area: codereg
paths: [src/apps/calced/CalcEd.C]
issues: [964]
---
Docking Box Around Selection stores the box, puts the calculation back to
CREATED and publishes `ecce_url_details`; `CalcEd::detailsMCB` merges the box
and calls `enableSave()`. Save stayed grey for two reasons, both ordering:

- `CalcEd::stateMCB` set `EDIT` on every state message at or below READY, which
  clears MODIFIED. The state message (sent first) can be handled after the
  details one. It now moves READONLY to EDIT only, so unsaved edits stay.
- A completed calculation opens the editor READONLY, and `detailsMCB` returned
  before merging when it was. It now lifts READONLY itself when the stored
  state is at or below READY, then merges and enables Save.

Other state, runtype and details messages reach the editor the same way:
anything that calls `setEditStatus(EDIT)` outside Save can silently discard
the Save button.
