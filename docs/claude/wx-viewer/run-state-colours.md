---
type: rule
title: "Run-state colours: Okabe-Ito hues; same-shape states differ in lightness, checked by tests/look/contrast.py"
area: wx-viewer
paths: ["data/client/config/EcceGlobal", "src/wxgui/wxtools/WxState.C", "include/wxgui/WxState.H", "tests/look/contrast.py", "tests/look/cvd.py", "tests/look/statelegend.py", "src/apps/organizer/CalcMgr.C"]
issues: ["210"]
---
The run states are drawn as ~12 px icons whose shape groups them
(triangle: created/ready; circle: submitted/running; square:
completed/loaded; diamond: unsuccessful/failed/system; coffin: killed), so
within a shape only colour tells them apart. At that size hue is hard to
judge: submitted (#1f6f8f) and running (#007a00) in 9.0.0-alpha.4 were both
L* 44 and looked alike.

Rules, enforced by `tests/look/contrast.py` (ctest `look_contrast`):
- each colour reaches 4.5:1 against its theme's backgrounds;
- same-shape pairs differ by CIEDE2000 >= 10 with normal vision and with
  simulated deuteranopia, protanopia and tritanopia (`tests/look/cvd.py`,
  Machado 2009), and by >= 10 in L*;
- `EcceGlobal` and the fallback tables in `WxState.C` are equal.

Hues are Okabe-Ito, darkened for light themes and lightened for dark ones
(failed stays red: vermillion pushed toward red). A user value equal to a
former default (pre-9.0, or 9.0-alpha light/dark) is a stale "Reset" copy
and is ignored; add the old table to `isFormerDefault` when defaults change.

`tests/look/statelegend.py OUTDIR [--ecce-home DIR]` renders the real
Organizer legend (`WxState::createLegend`) in light and dark on Xvfb.
