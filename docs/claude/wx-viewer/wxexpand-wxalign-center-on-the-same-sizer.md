---
type: map
title: "`wxEXPAND|wxALIGN_CENTER` on the same sizer item"
area: wx-viewer
section: ""
---
**`wxEXPAND|wxALIGN_CENTER` on the same sizer item** — a documented wx
footgun; alignment can suppress expand instead of being ignored.
Combined with a widget that only learns its own size inside its first
`OnPaint()` (e.g. `ElementButton`), this makes content render as
near-invisible near-zero-size widgets. Hit in the periodic table,
Basis Set Tool, and Builder's popup periodic table.
