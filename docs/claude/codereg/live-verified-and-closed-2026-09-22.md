---
type: pitfall
title: "The memory field's unit label comes from the dialog, never from stored GUIValues (#77)"
area: codereg
section: "Memory fields: the #77 unit-label fix"
paths: ["scripts/codereg/templates.py"]
issues: [77]
---
Issue #77 (G16 Memory field showing
"Megawords" despite a verified-correct source and running process) is
fixed as of `884593f` — the bug was never `calced`'s C++ side; it was
`BoxSizerFrame.FinalizeSetting()` (`scripts/codereg/templates.py`)
calling `SetUnit(unit)` on GUIValues restore, overwriting the
widget's freshly-correct unit label with whatever was persisted in
the calc's *stored* data (stale for any calc saved before the
GB-everywhere UX change). Fix removes the `SetUnit()` call on
restore — value persists, unit label doesn't. **Live-verified and
closed 2026-09-22** (confirmed more than once on screen). If a wrong
unit label ever shows again, something new is wrong, not a repeat.
