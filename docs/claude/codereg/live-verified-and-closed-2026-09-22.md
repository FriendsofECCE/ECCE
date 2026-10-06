---
type: pitfall
title: "The memory field's unit label comes from the dialog, never from stored GUIValues (#77)"
area: codereg
section: "Memory fields: the #77 unit-label fix"
paths: ["scripts/codereg/templates.py"]
issues: [77]
---
**A restored memory field keeps the dialog's unit label.**
`BoxSizerFrame.FinalizeSetting()` (`scripts/codereg/templates.py`) restores
the stored *value* from GUIValues but deliberately does not call
`SetUnit(unit)`: the stored unit is whatever was current when the
calculation was saved, so restoring it would put a stale label (e.g.
"Megawords") on a field that is now entered in GB. The bug was never in
calced's C++ side; a wrong unit label points here first.
