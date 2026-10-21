---
type: map
title: "Native ecce-dialog engine (pilot): a .dlg file per Details dialog, same wire protocol as the Python dialog"
area: codereg
paths: ["src/apps/codereg-dialog/ecce-dialog.cpp", "src/wxgui/details", "scripts/codereg/ecceqmtheory.dlg", "scripts/codereg/ecceqmtheory.py", "src/dsm/xml/JCode.C", "tests/dialogs/native_test.py"]
issues: [294]
---
`bin/ecce-dialog <file.dlg> <the usual 17 argv fields>` is a wx-only C++ program (the engine is `src/wxgui/details`, the line format is in `dlg-grammar.md`)
that reads a dialog description and speaks the Python dialogs' protocol: the
same restore file in, the same `name|value|unit|enabled|export|kind` lines and
`#STARTED/#INITIALIZED/#CLOSING` over UDP out. `JCode::getTheoryRunTypeEditorNames`
returns it instead of `python3 <x>.py` only when `ECCE_NATIVE_DIALOGS` is set (not
0), `<x>.dlg` sits next to `<x>.py` and `bin/ecce-dialog` exists; otherwise the
Python dialog runs, so every dialog without a .dlg is unaffected.

ECCE-QM Theory, ORCA Theory and ORCA Runtype have a .dlg (`orcatheory.dlg`,
`orcaruntype.dlg`: ctests `dialogs_native_orcatheory`/`_orcaruntype`; the NEB
pickers are `calcpick`, the product/guess warnings `status`). Keep each in step
with its .py: the
functional list and the dependency (`enable_if="mult>1"`) are written twice, and
`tests/dialogs/native_test.py` (ctest `dialogs_native`) is what notices drift. It
compares the lines both programs send over every context the .edml opens the
dialog in, the 29 hand-picked pilot cases and the pixels of two windows, and
diffs the widget inventories. Two details it pins that are easy to lose: the Python
combo/number widgets send their line again when a restore changes their enabled
state (the native engine repeats that), and the radio pair always exports
(`export=1`), whatever the default.
