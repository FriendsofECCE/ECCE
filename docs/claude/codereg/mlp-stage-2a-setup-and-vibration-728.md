---
type: map
title: "MLP (MACE) stage 2A: Set up builds the environment detached and the model cache sits inside it; Vibration projects rigid motions and lists them first at 0 (#728)"
area: codereg
section: "Getting a calculation set up (the \"code registration\" system)"
paths: ["scripts/ecce-mlp-setup", "scripts/ecce-mlp", "scripts/parsers/mlp.vib", "scripts/parsers/mlp.desc", "src/apps/machregister/WxMachineRegisterTools.C", "src/dsm/xml/CodeFactory.C", "src/apps/calced/CalcEd.C", "tests/mlp"]
issues: [728]
---
- **The code id stays `MLP`; only the shown name is "MLP (MACE)".**
  `CodeFactory::displayName()` maps it for the code-bar tooltip, the editor
  title and the Register Machines list. Stored calculations, CONFIG keys,
  `siteconfig/Machines` and the scripted `select MLP` hook use the id.
  The menu item text is in `ResourceDescriptor*.xml`, the dialog titles in
  `mlptheory.py`/`mlpruntype.py`.
- **Set up... runs `scripts/ecce-mlp-setup` detached on the machine.** The
  script text is sent inline over `SchedulerQuery::Remote` (the path Find and
  Check use), written to `<folder>/ecce-mlp-setup` and started with
  `nohup setsid`; the window polls `setup.log` (lines
  `ECCE-MLP-SETUP step N/5: ...`), `setup.pid` and `setup.status`
  (`done <python>` / `failed <reason>`). A process gone without a status for
  three polls counts as failed. Cancel kills the process group. Scripted runs
  take the folder from `ECCE_TEST_MLP_SETUP_DIR`; `ECCE_MLP_SETUP_MOCK=1`
  makes the script create the venv (`--without-pip`) and skip every download.
- **Model cache = `<venv>/cache/mace`.** mace's `get_cache_dir()` reads
  `XDG_CACHE_HOME` at call time, so the set-up downloads with
  `XDG_CACHE_HOME=<venv>/cache`, and `ecce-mlp` sets it to
  `sys.prefix/cache` when `cache/mace` exists there. A model not in that cache
  is then downloaded into it too (on a node without internet: an error).
- **Vibration is `ecce-mlp`'s own finite-difference Hessian**, not
  `ase.vibrations`: central differences (6N force calls, `delta=`), symmetrised,
  mass-weighted, rigid motions projected out (translations; rotations too
  for a molecule, SVD so a linear molecule loses one). The rigid modes are
  printed first at exactly 0 with zero vectors, the rest ascending, so VIBFREQ
  and VIB are 3N long and row-aligned like ORCA's and xTB's. ase.vibrations
  does not project, so tests compare the 3N-6 (3N-3) non-rigid modes after a
  tight pre-relaxation; they agree to < 0.01 cm-1 (float64) for water and the
  8-atom NaCl cell with MACE medium-mpa-0. No VIBIR (no dipole): NModePanel
  shows no IR pane rather than unit spikes.
- **mlp.vib needs the Begin line** (`ECCE-MLP-VIB <3N> <N>`), so its entry
  uses `Skip=0`.
- **The screening note and the charge/spin warning are in CalcEd's Notes row**
  (`updateNotes()`), not in the Theory/Runtype dialogs; the charge and spin
  handlers call `updateNotes()` so the warning follows a change at once.
