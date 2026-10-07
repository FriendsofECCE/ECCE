---
type: pitfall
title: "GTK :backdrop fades every label to ~2.8:1; ewxApp::applyBackdropStyle() undoes it, and headless runs need ECCE_TEST_BACKDROP"
area: wx-viewer
paths: ["src/wxgui/ewxClasses/ewxApp.C", "tests/look/login_backdrop.py", "tests/look/backdrop.C", "tests/look/organizer_backdrop.py"]
issues: [210]
---
Adwaita draws text in an unfocused toplevel (`:backdrop`) in a 50 % mix of
text and background: a normal label drops from 11.6:1 to 2.8:1, so with
several ECCE windows open the unfocused ones look disabled.
`ewxApp::applyBackdropStyle()` (called from `OnInit`) adds an
application-priority CSS sheet giving `label`, `entry` and `.view` the
theme's normal colour in `:backdrop` and `@insensitive_fg_color` for
`:disabled:backdrop`. GTK3 CSS has no `:not()`, so the disabled case is its
own rule. `ECCE_BACKDROP=theme` switches the fix off.
- Without a window manager (Xvfb) GTK never enters `:backdrop`: every
  window stays active whatever `xdotool windowfocus` does. Tests set the
  flag themselves (`backdrop.C`) or run the app with `ECCE_TEST_BACKDROP=1`,
  which re-asserts it on every top-level window each 200 ms.
- `ctest -R look_backdrop` measures the ink contrast of a window in
  `:backdrop` against an active one; it fails below 85 %, or if a
  disabled label is no dimmer than a normal one.
