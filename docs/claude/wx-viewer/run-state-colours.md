---
type: rule
title: "Run-state colours: submitted is Okabe-Ito orange so it stays apart from running's green; checked by tests/look/contrast.py"
area: wx-viewer
paths: ["data/client/config/EcceGlobal", "src/wxgui/wxtools/WxState.C", "include/wxgui/WxState.H", "tests/look/contrast.py", "tests/look/cvd.py", "tests/look/statelegend.py", "src/apps/organizer/CalcMgr.C"]
issues: ["210"]
---
The run states are drawn as ~12 px icons whose shape groups them
(triangle: created/ready; circle: submitted/running; square:
completed/loaded; diamond: unsuccessful/failed/system; coffin: killed), so
within a shape only colour tells them apart. They are icons only, never text.

Submitted and running (both circles) were once teal and green at the same
lightness and looked alike. Running keeps its green (Andy's choice);
submitted is Okabe-Ito orange, #e69f00 in light themes and #f5d999 in dark.
Orange and green merge for deuteranopes and protanopes unless their
lightness differs, which is why the light-theme orange is *not* darkened to
4.5:1 against the background (it is about 2:1, the one exemption).

`tests/look/contrast.py` (ctest `look_contrast`) checks:
- every other colour reaches 4.5:1 against its theme's backgrounds;
- submitted/running differ by CIEDE2000 >= 10 with normal vision and with
  simulated deuteranopia, protanopia and tritanopia (`tests/look/cvd.py`,
  Machado 2009);
- `EcceGlobal` and the fallback tables in `WxState.C` are equal.

Not enforced, known weak: unsuccessful/failed (both diamonds) are 2.3 apart
for deuteranopes in the light theme.

A user value equal to a former default (pre-9.0, or 9.0-alpha light/dark)
is a stale "Reset" copy and is ignored; add the old table to
`isFormerDefault` when defaults change.

`tests/look/statelegend.py OUTDIR [--ecce-home DIR]` renders the real
Organizer legend (`WxState::createLegend`) in light and dark on Xvfb.
