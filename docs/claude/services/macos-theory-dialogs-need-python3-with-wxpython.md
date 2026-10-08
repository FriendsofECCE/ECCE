---
type: pitfall
title: "macOS: ECCE.app carries its own Python with wxPython; every python3 ECCE runs must find it first (#133)"
area: services
paths: [src/dsm/xml/JCode.C, src/apps/calced/CalcEd.C, packaging/macos/bundle-python.sh, packaging/macos/launcher.sh, packaging/ecce-home.sh.in, tests/macos/python-check.sh, help/src/running-codes-on-macos.md]
issues: [133]
---
CalcEd starts `python3 <script>.py` for **Theory Details** and **Runtype
Details** (`JCode::getTheoryRunTypeEditorNames`), and `ecce-first-start` is
`#!/usr/bin/env python3`. On a bare Mac `/usr/bin/python3` is the Xcode shim
with no wxPython, so these failed ("Unable to invoke theory details dialog").
`make-app.sh` now puts python-build-standalone CPython 3.13
(`install_only_stripped`, sha256-checked) with the wxPython 4.2.4 wheel in
`Contents/Resources/python` (`bundle-python.sh`; the wheel's tag must be
accepted by that interpreter's pip, so run pip with MACOSX_DEPLOYMENT_TARGET
unset). The launcher puts `Resources/python/bin` first on PATH and sets
`ECCE_PYTHON`; `ecce-home.sh.in` does the same for the wrappers when
`$ECCE_HOME/../python/bin/python3` exists. User Pythons come after.
Keep it that way: a new place that runs `python3` must get PATH from one of
these, not from the caller's. `tests/macos/python-check.sh` (CI) runs the
first-start window and a Theory dialog through `ECCE_APP_RUN=1 ecce CMD...`
(launcher.sh), the app's own environment. ECCE-QM has no Runtype dialog, so
scripts must not wait for `ready` on it.
