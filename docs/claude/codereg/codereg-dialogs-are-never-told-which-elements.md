---
type: pitfall
title: "Codereg dialogs are never told which elements the structure contains"
area: codereg
section: "Pitfalls found more than once (check for siblings)"
paths: ["globals.py"]
---
**Codereg dialogs are never told which elements the structure
contains.** They are standalone processes whose entire input is
`globals.py`'s fixed argv (calc name, category, theory, runtype,
symmetry group, electron/orbital/normal-mode counts). So no
per-element UI is possible in a dialog — anything element-dependent
(QE's pseudopotential-per-species being the live case) has to be
resolved in `ai.<code>`, which does read the `.frag`. Worth knowing
before designing any such dialog.
