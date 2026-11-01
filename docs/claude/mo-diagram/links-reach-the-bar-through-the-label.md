---
type: pitfall
title: "Correlation lines end at the label column's edge; a leader carries them on to the bar (#745)"
area: mo-diagram
paths: [src/apps/builder/MoDiagramCanvas.H, src/apps/builder/MoDiagramPanel.C, src/apps/builder/MoPanel.C, tests/apps/modiagram_panel_test.py]
issues: [745, 458]
---
Since #458 the model column is "construction label | bar | symmetry label",
and a link meets its level at the label column's outer edge (`CentreLane::
linkL/linkR`), so it never crosses another level's label. With long labels
(HF: "σ(H 1s/F 2p)") that left the H lines well short of the levels. A
horizontal leader on the level's row now runs from that edge to the bar,
behind the label's white box. Builder scene `modiagram <n> links 0` lists each
painted link's distance from its bar (0 = reaches); `modiagram_panel_test.py`
checks HF (fixture `tests/modiagram/fixtures/ecceqm-hf`).

The diagram is always built from the canonical MOs. With "Localised MO"
chosen in the MOs panel (`MoPanel::setLocalised` tells the diagram), its
status line under the controls says so (`modiagram <n> status 0`), and a click
on a level switches the MOs panel back to the canonical set: otherwise the
level's ORBENG index would select the localised orbital of that number.
