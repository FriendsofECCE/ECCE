---
type: rule
title: "Run-state colours: submitted yellow, created sky blue, pale fills outlined in light themes; checked by tests/look/contrast.py"
area: wx-viewer
paths: ["data/client/config/EcceGlobal", "src/wxgui/wxtools/WxState.C", "include/wxgui/WxState.H", "tests/look/contrast.py", "tests/look/cvd.py", "tests/look/statelegend.py", "src/apps/organizer/CalcMgr.C"]
issues: ["210", "208"]
---
The run states are drawn as ~12 px icons whose shape groups them
(triangle: created/ready; circle: submitted/running, an open ring for
waiting for login (#208); square:
completed/loaded; diamond: unsuccessful/failed/system; coffin: killed), so
within a shape only colour tells them apart. They are icons only, never text.

Submitted and running (both circles) were once teal and green at the same
lightness and looked alike; created and ready (triangles) were two similar
blues. Running is the light green it had up to 8.x (#00cd00, Andy's
choice) and ready keeps its blue. Orange means failure here, so submitted
is a pale yellow (#f8ef8c light, #f7ee8a dark): Okabe-Ito #f0e442 is only
8.8 from #00cd00 under protanopia, a darker yellow is closer still and created Okabe-Ito sky blue (#56b4e9 light; #b0e2ff dark).

Pale fills cannot reach 4.5:1 on a light background, and darkening yellow
or sky blue to that would bring them to the lightness of green or blue and
merge them for colour-blind users. Instead, in a light theme `WxState`
draws a fill brighter than relative luminance 0.27 with an outline at half
its brightness (`outlineFor`), and the outline must reach 3:1 (WCAG
non-text). The icons are never used as text colours.

`tests/look/contrast.py` (ctest `look_contrast`) checks:
- every non-outlined colour reaches 4.5:1, every outline 3:1;
- submitted/running, created/ready and waiting against submitted and
  running differ by CIEDE2000 >= 10 with
  normal vision and simulated deuteranopia, protanopia and tritanopia
  (`tests/look/cvd.py`, Machado 2009);
- `EcceGlobal` and the fallback tables in `WxState.C` are equal.

Not enforced, known weak: unsuccessful/failed (both diamonds) are 2.3 apart
for deuteranopes in the light theme.

A user value equal to a former default (pre-9.0, or 9.0-alpha light/dark)
is a stale "Reset" copy and is ignored; add the old table to
`isFormerDefault` when defaults change.

`tests/look/statelegend.py OUTDIR [--ecce-home DIR]` renders the real
Organizer legend (`WxState::createLegend`) in light and dark on Xvfb.
