---
type: map
title: "GROMACS Prepare, imported force fields: a per-user library on the client, staged with the Prepare as <name>.ff__SLASH__<file>.gmxff inputs"
area: codereg
paths: [data/client/config/mimetypes, data/client/cap/GROMACS.edml, packaging/dataserver/httpd.conf.ecce, scripts/parsers/gromacs.launchpp, src/wxgui/mdtools/GromacsForceFieldLibrary.C, include/wxgui/GromacsForceFieldLibrary.H, src/wxgui/mdtools/GromacsPreparePanel.C, scripts/gensub, src/comm/commxt/Launch.C, tests/launch/gromacs_prepare_test.py]
issues: [930]
---
- **Library.** `<ECCE user dir>/forcefields/<name>.ff/` (`Ecce::realUserPrefPath()`,
  so `~/.ECCE/forcefields` under `ECCE_REALUSERHOME`), on the client only.
  `GromacsForceFieldLibrary` does import, listing and lookup with wx file
  APIs and `wxZipInputStream` (no shell, so it is the same on Windows and
  macOS). Nothing in a job script refers to this path.
- **Import** (Force field box, "Import..."): a `<name>.ff` folder or a zip
  holding exactly one. Refused: a name not ending `.ff` or with characters
  `md.gmxprep` would not take, no `forcefield.itp`, no `.rtp` (pdb2gmx needs
  one; such a folder could only serve through an attached topology), a zip
  entry with an absolute path or `..` (checked for every entry before any is
  written), more than one `.ff` in a zip, over 300 MB. The copy is made in a
  scratch folder inside the library and renamed in only when it validated. A
  name already in the library or among the built-in ids asks replace/cancel;
  an imported folder of a built-in's name hides that built-in in the choice
  (pdb2gmx takes `./name.ff` before the library).
- **Choice and model.** Built-ins, then imported ones labelled with the first
  line of `forcefield.doc` plus "(imported)", then Other (GMXLIB). The model
  stores the folder name in `<ForceField>` and the recipe as `FF=` exactly as
  for a built-in; the panel recognises an imported one by finding that name in
  the library. Its water models come from the folder's `watermodels.dat`.
- **Staging.** Choosing an imported force field (and reopening a task that
  uses one) stores its files as inputs of the Prepare under flat names
  `<name>.ff__SLASH__<file>.gmxff`, like min.ff's includes (`GromacsIncludes::encodeName`).
  The extra `.gmxff` (type `chemical/x-gromacs-forcefield`, in `data/client/config/mimetypes`,
  `GROMACS.edml` and the data server's `httpd.conf.ecce`) is needed because the server types
  a file by extension and a lookup by type finds nothing for `.rtp`, `.atp`, `.hdb` ...; a
  central server must run the matching `ecce-server` package. The job strips it.
  `Launch::auxInputUrls` hands a study's auxiliary inputs on to every later
  GROMACS task, so the Optimize and MD tasks get the folder with no extra
  path. `gensub` restores the folders before pdb2gmx (protein source) and
  before grompp (every other task); `gromacs.launchpp` does the same for its
  client-side grompp check. Because it is all task input files, it works the
  same against a central data server. Switching the choice removes the old
  staged folder; a library entry that has since gone leaves the staged copy in
  place and the editor warns.
- **Test hooks.** `set ffimport <path>` (`set ffclash replace|cancel` answers a
  name clash), `ffchoices`; `tests/launch/gromacs_prepare_test.py
  --ffimport-only` imports a renamed copy of a built-in as a folder and as a
  zip, runs Prepare and Optimize with it, and covers the refusals.
