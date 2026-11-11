---
type: map
title: The Vina search box in the viewer and the Poses panel for docking
area: codereg
paths:
  - src/apps/builder/BuilderVinaBox.C
  - src/apps/builder/Builder.H
  - src/apps/builder/BuilderApp.C
  - src/apps/builder/EnsemblePropertyPanel.C
  - src/apps/calced/CalcEd.C
  - scripts/codereg/vinaruntype.py
  - scripts/parsers/ai.vina
  - tests/apps/vina_viewer_test.py
issues: [455]
---

Stage 2 of #455: picking and drawing the docking box, and the pose panel for
an empirical score.

**The box lives in the run type details, not in a tool of its own.**
`ES.Runtype.Vina.BoxAuto|BoxMargin|BoxCenter{X,Y,Z}|BoxSize{X,Y,Z}` are what
`ai.vina` writes into the config, so the Builder reads and writes exactly
those keys through `ICalculation::guiparams()`. Tools > *Docking Box Around
Selection* sets centre and size from the selected atoms plus the margin,
turns `BoxAuto` off, and puts the calculation back to CREATED so Launch
cannot reuse the old deck.

- *Both sides must round the same way.* `ai.vina`'s `round1` is
  `sprintf "%.1f"`; `BuilderVinaBox.C` rounds with `snprintf` for the same
  reason, so the drawn box is the box Vina searches. The automatic box is
  recomputed in the Builder with the same rule (bounding box + 2*margin, at
  least 16 A) rather than read back from the deck, which does not exist until
  the editor saves.
- *The editor and the Builder are separate processes.* The Builder publishes
  `ecce_url_details` after storing the box, and `CalcEd::detailsMCB` (empty
  until now) merges back only the `ES.Runtype.Vina.Box*` keys, so unsaved
  edits of other details survive. In the other direction the editor publishes
  `ecce_vina_box` on every detail the run type dialog sends, which is how the
  box follows typed numbers before any save; `BuilderApp::vinaBoxMCB` draws
  from the message, not from the server.
- *The wireframe is a property switch*, `SGContainer::getPropertySwitch
  ("VinaSearchBox")`, as the superposition overlay is: it is not part of the
  fragment, so nothing in the Builder tries to edit, save or measure it.

**The Poses panel.** `EnsemblePropertyPanel` is shared with CREST, the MLP
sites and the protonation ensembles, so the Vina changes are conditional:

- `isVina()` looks at the *code* (`TaskJob::application()`) as well as the run
  type, because a calculation can reach the panel without its run type
  (import, a script). The column label is "Vina score (kcal mol-1)".
- `p_population` drops the Boltzmann column for Vina only (table, plot label,
  CSV, PNG and the sort-column mapping all key off it). CREST and the sites
  keep it.
- *Pocket view*: the receptor is drawn as sticks only for whole residues with
  an atom within 6 A of the pose's ligand, the ligand as balls and sticks and
  the rest hidden (`CSStyleCmd` on a selection, then `DisplayAtomsCmd`).
  Without it a 2300-atom receptor hides the pose completely. The ligand comes
  from `ES.Runtype.Docking.LigandAtoms`, or, when that is empty, from the
  smallest connected component, which is what `docking.pl` splits on.
- The plot under the RMSD table has a 220 px minimum height: in a short pane
  the two tables above squeezed it until the axis labels were clipped.

**Hooks** (`ECCE_BUILDER_SCRIPT`, `BuilderVinaBox.C::vinaScriptCommand`):
`selectatoms <1-5,9>`, `vinabox [margin]`, `vinaboxinfo`, `poses
select|info|pocket|png`. The Calculation Editor's `ECCE_TEST_CALCED` gained
`detail <key> <value>` and `details <prefix>`, which is how a calculation can
be made entirely through the editor in a test instead of through the harness
shortcut. `tests/apps/vina_viewer_test.py` does exactly that with 1IEP and
imatinib, then runs it and checks the panel and the box.

**Packaging.** `autodock-vina` is a Recommends of `ecce-client` (and of the
monolithic package); `/usr/bin/vina` is found by `CodePrograms::findOnPath`
at first-start registration, as for the other codes. Windows and macOS have
no Vina package: see the bundling note in
`pdbqt-preparation-455.md`.

**Thumbnails.** In the pocket view a pose's thumbnail is its ligand only
(the whole receptor gave every pose the same grey ball). They are drawn on
the viewer's background; `thumbnailBatch` keeps its timer running at 500 ms
and redraws them when the background changes or the pocket view is toggled,
because a script or user setting the background after the panel opened left
black thumbnails on a white viewer. Switching the pocket view on frames the camera on the shown atoms
(ligand + pocket residues) and the drawn search box
(`EnsemblePropertyPanel::fitPocketCamera`), once per switch-on, so stepping
through poses keeps the user's zoom; switching it off calls `viewAll()`.
`viewAll()` alone is no use here: hidden atoms count in its bounding box.
The fit projects the box corners on the camera axes rather than using
`SoCamera::viewAll` on a proxy: the bounding sphere left the pocket at a third
of the narrow viewer. *ADJUST_CAMERA puts the height angle on the shorter side
of the viewport*: in a tall viewer (aspect < 1) it spans the width, so the
half extent is `max(hx, hy*aspect)`, not `max(hy, hx/aspect)` (that zoomed out
by 1/aspect). Hook: `poses info`/`poses pocket on` report `fit=<diagonal A>`
of the framed box (0 when off); the test checks 0 < fit < 60 A.

**Fewer poses than NumPoses is Vina, not the parser.** Vina 1.2.7 prints all
`num_modes` in its log table but writes to `--out` only the modes within
`energy_range` of the best. 1IEP + imatinib, seed 7, exhaustiveness 4: the log
lists -13.47, -10.52, -9.83, -9.70; `poses.pdbqt` holds the first two with
`energy_range = 3`, all four with 10 (checked by hand on the prepared files).

**Pocket view: bonds go with their atoms.** `showPocket` restyles the
ligand (Ball and Stick) and the pocket residues (Stick) and hides the rest
with `CSStyleCmd`/`DisplayAtomsCmd` on the selection, and both commands change
only the bonds in `m_bondHighLight`. A bond is drawn by the ChemDisplay whose
style name it carries (`SGContainer::touchChemDisplay`), and that display draws
it only with its atoms; with atoms alone selected every bond kept the old
style, so the pocket drew as bare-atom crosses and the ligand as balls without
sticks on any receptor (frames and the stored bonds were never the problem;
CREST/MLP ensembles do not use the pocket view). Each role now selects its
bonds too: a bond takes the role of its two atoms, and one between a shown
and a hidden residue is hidden. Hook: `poses info` reports `bonds=<ligand>/<pocket>`
(1IEP + imatinib: 41/261; it was 0/0), checked in `vina_viewer_test`.
