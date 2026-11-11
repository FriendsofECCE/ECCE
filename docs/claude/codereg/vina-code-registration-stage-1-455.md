---
type: map
title: "AutoDock Vina is registered like CREST on the shared docking layer; the structure is the auxiliary input vina.vinafrag (#455)"
area: codereg
section: "Getting a calculation set up (the \"code registration\" system)"
paths: ["data/client/cap/Vina.edml", "scripts/parsers/ai.vina", "scripts/parsers/vina.desc", "scripts/gensub", "scripts/codereg/vinaruntype.py", "scripts/codereg/vinatheory.py", "scripts/parsers/verifyinput", "data/client/config/mimetypes", "src/comm/commxt/Launch.C", "src/apps/builder/EnsemblePropertyPanel.C", "tests/launch/vina_test.py"]
issues: [455]
---
Application type `Vina` (resource `vina_es`, CONFIG key `vina`, gensub sub
`vina`), run type `Docking (Vina)`, theory `Vina` in category `SE` (the
semi-empirical group of the code bar; no basis). Not bundled: the program is
registered per machine like any other.

- **The structure is an auxiliary input, not part of the config.** `ai.vina`
  writes the `.frag` verbatim to `vina.vinafrag`; Vina.edml declares it as
  `<Input type="auxiliary" mimetype="chemical/x-vina-structure">`, so
  DeckWriter stores it beside the deck (its `*<aux name>` file match) and
  Launch copies it to the run directory (`auxInputUrls`). The MIME type is
  found from the extension, so `.vinafrag` is in `data/client/config/mimetypes`.
  The config keeps `#| LIGAND_ATOMS=`, `#| RECEPTOR_ATOMS=` (as ranges,
  `1-2229`) and `#| FLEXIBLE_AMIDES=` (Vina skips `#` lines). gensub copies
  `vina.vinafrag` to `vina.frag`, falls back to a `#| BEGIN-FRAG` block for a
  deck stored before this, and expands the ligand ranges for the frame awk.
  Until 2026-11 the whole structure rode in the config as `#|` lines; a
  protein's receptor list was then a 10 kB line, which Launch's old 255-byte
  line copy cut off (`launch-copies-stored-files-line-by-line.md`).
- **Split is `docking.pl`**, as for xtb: `ES.Runtype.Docking.LigandAtoms` and
  `LigandCharge`. The box (`BoxAuto`, default on) is computed in `ai.vina`
  from the ligand atoms: bounding box + 5 A each side, at least 16 A.
- **Frames are rebuilt in gensub.** `poses.pdbqt` holds only the ligand with
  nonpolar hydrogens merged. The awk script puts receptor atoms at their input
  coordinates, ligand atoms from the pose by PDBQT serial through
  `ligand.map` (ecce-pdbqt `--map`, written only when asked), and places each
  merged hydrogen in the frame of its nearest kept atom and the two kept atoms
  next to it, taken from the original geometry. That is approximate and not
  scored. The frames are `ECCE-DOCK-FRAME`, read by `dock.poses`.
- **POSESCORE is reused**, not a key of its own: the `kindFor` mapping sends
  `Docking (Vina)` to the Poses panel, whose energy column is labelled
  "Vina score (kcal mol-1)" instead of "Interaction energy". `ScanPoint`
  needs nothing: it takes POSESCORE. The panel's Boltzmann "Population"
  column is still computed from the scores, which means nothing for an
  empirical score.
- **ecce-pdbqt path.** gensub bakes `$ECCE_HOME/bin/ecce-pdbqt` of the machine
  that makes the script; `ECCE_PDBQT` in the job's environment overrides it.
  A remote machine without an ECCE installation needs that variable or a
  copy of the program (unlike MLP, the runner is not inlined).
- **No normal-termination line.** Vina 1.2.7 prints none at the default
  verbosity; gensub requires the word `affinity` (the result table header),
  and a failed run exits non-zero anyway.
- **verifyinput** skips its geometry check for Vina (it sees only the
  config) and judges the config keys, a positive box and the two
  `#| ..._ATOMS=` lines instead.
- The viewer's box-picking tool does not exist yet; the box is typed or
  automatic.
- Test: `tests/launch/vina_test.py` (deck checks always; the run needs vina and
  `ecce-pdbqt`, `ECCE_PDBQT` or the build's), progress fixture
  `tests/progress/fixtures/vina`.
