---
type: pitfall
title: "macOS: the Theory/Runtype detail dialogs need a python3 with wxPython on the PATH (#133)"
area: services
paths: [src/dsm/xml/JCode.C, src/apps/calced/CalcEd.C, packaging/macos/make-app.sh, help/src/running-codes-on-macos.md]
issues: [133]
---
CalcEd starts `python3 <script>.py` for **Theory Details** and **Runtype
Details** (`JCode::getTheoryRunTypeEditorNames`). ECCE.app bundles no
Python, and the launcher's PATH is `Resources/bin:libexec:/usr/bin:/bin:...`.
On a bare Mac `/usr/bin/python3` is the Xcode shim (it asks to install the
command line tools, and has no wxPython), so the dialogs never start and a
scripted CalcEd waits at `ready` for ever; a user sees the Details buttons
do nothing. Verified path: Miniforge, `conda install wxpython`, and start
`ecce` with `~/miniforge3/bin` ahead of `/usr/bin` on the PATH. ECCE-QM has
no Runtype dialog, so scripts must not wait for `ready` on it.
Open: ECCE has no setting for "which python3"; a Finder launch cannot find a
conda Python.
