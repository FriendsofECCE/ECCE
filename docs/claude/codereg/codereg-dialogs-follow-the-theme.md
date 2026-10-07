---
type: rule
title: "Codereg dialogs use the theme's colours and font like the C++ windows; no fixed colours in globals.py"
area: codereg
paths: ["scripts/codereg/globals.py", "scripts/codereg/templates.py", "tests/look/codereg_shot.py"]
issues: [210]
---
`globals.py` holds no legacy colours any more: inputs and panels are the
theme's (`wx.NullColour`), read-only is `SYS_COLOUR_BTNFACE` (as
`ewxStyledWindow::getReadonlyColor`), and the warning/error flash of the
message box uses the `ewxThemeColours` BAD/UNSURE tints in light and dark.
Fonts are the theme's GUI font at 10 pt plus the Font Size step, read from
`$ECCE_REALUSERHOME/.ECCE/EcceGlobal` (`FontSize`, `UseSystemFont`) since
the dialogs are separate processes; keep that in step with
`ewxStyledWindow::getFontSizeStep`. `tests/look/codereg_shot.py` renders
the real dialogs headless in light/dark (`--flash warning|error`).
