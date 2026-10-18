---
type: pitfall
title: "\"1 1 1\" in one text field reads \"111\" in the GNOME desktop font (Cantarell): give each number its own field"
area: wx-viewer
paths: [src/wxgui/mdtools/GromacsPreparePanel.C, tests/launch/gromacs_prepare_test.py]
issues: []
---
The GROMACS Prepare editor's Repeat (a b c) showed "111" on a fresh task. The
model, the XML, the data server and the field's value were all "1 1 1"
(`fields` printed `repshown=1 1 1`); Cantarell, the GNOME default, draws the
spaces about as wide as the gaps between its digits. Look at the pixels before
chasing the data: a headless run reads the same value in either font, only
`gtk-font-name=Cantarell 11` (a settings.ini under `XDG_CONFIG_HOME`) shows the
glitch. The editor now has one field per axis and keeps the model's "a b c"
string. `tests/launch/gromacs_prepare_test.py --organizer-only` makes a Prepare
with the Organizer's New (local data, and against a data server with
`ECCE_TEST_DAV=1`), opens it for the first time under Cantarell and checks the
three fields.
