---
type: pitfall
title: "A calculation can live in any local folder, not only in a data store"
area: services
section: "Pitfalls"
paths: ["src/apps/builder/Builder.C", "src/wxgui/ewxClasses/ewxFileData.C", "src/wxgui/ewxClasses/ewxFileCtrl.C", "src/wxgui/ewxClasses/ewxGenericFileDialog.C", "src/wxgui/wxdialogs/SaveExperimentAsDialog.C", "src/util/genutil/TempStorage.C", "src/dsm/edsiimpl/FileEDSI.C", "tests/apps/saveas_test.py", "tests/launch/run_tests.py"]
issues: [216]
---
**A calculation can live in any local folder, not only in a data store.**
FileEDSI (#216) serves any `file://` path: a directory's type and
properties are in its own `.ecce-meta`, so Builder > Save As to a
Local Filesystem folder creates a complete calculation there, in
server mode as in local mode. Three places assumed otherwise:

- `Builder::createCalculation()` refused a local URL that was not an
  existing *file*, so the new calculation directory was reported as
  "error parsing ... or it does not exist". It now refuses only a path
  that does not exist.
- The file dialogs list the Local Filesystem with plain wx code, where a
  calculation is a folder to enter. `ewxFileData::isLocalDocument()`
  (a directory with `.ecce-meta` whose resource the server listing
  would show as a file) makes `ewxFileCtrl` list it as a file and
  `ewxGenericFileDialog` open it instead of entering it.
- `TempStorage::getJobRunDirectoryPath()` maps such a calculation to
  its path below `$HOME`, as one in a project maps below the user home.

The write guard of local mode (`FileEDSI::forbidden`) covers only paths
inside the data folder; other folders are the file system's business.
The Organizer shows such a folder only when it is added as a root
(Organizer > Add Server, URL `file:///path`); with "Set Home as Root"
a `file://` root maps to the user's home (server mode) or the data
folder's home (local mode), so the folder is then not shown.

Tests: `tests/apps/saveas_test.py` (save, list, reopen),
`tests/launch/run_tests.py --folder` (a MOPAC job to completed).
