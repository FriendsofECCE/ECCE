---
type: map
title: "`scripts/codereg`"
area: codereg
section: "Getting a calculation set up (the \"code registration\" system)"
paths: ["*runtype.py", "*theory.py", "globals.py", "scripts/codereg", "templates.py"]
---
**`scripts/codereg`** — one `*theory.py` + `*runtype.py` pair per code
(the "Theory/Runtype Details" dialogs). Python 3 / wxPython Phoenix.
`globals.py`/`templates.py` are the shared base every script imports —
most framework-level bugs live there, not in the per-code scripts.
