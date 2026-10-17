---
type: rule
title: "The text under the MO diagram has three tiers (#301)"
area: mo-diagram
paths: [src/apps/builder/MoDiagramPanel.C, src/apps/builder/MoDiagramCanvas.H, src/tdat/chemistry/FoDiagram.C]
issues: [301]
---
Tier A is always drawn: the "Experimental ... Click a level" line and one
summary line (construction, unit, `labels in O<sub>h</sub>`, hidden count).
Tier B is orange and present only when something is wrong (incomplete
pairs, electron count, poorly covered fragment levels, a `why` that is a
failure). Tier C is the "About this diagram..." dialog. Add a new sentence
to the tier it belongs in (`NoteTiers` in `MoDiagramPanel.C`,
`FoDiagram::Result::{summary,warnings,about}`), not to a running note.
The dump has `note:` (tiers A and B) and `about:` (tier C); the goldens
in `tests/modiagram/expected` carry both. "Covered", never "captured", in
anything the user reads.
