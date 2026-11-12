---
type: pitfall
title: "The Poses panel's cost is per pose selection, not per open: stepping a pose restyled all 4700 atoms again (#966)"
area: codereg
paths:
  - src/apps/builder/EnsemblePropertyPanel.C
  - src/apps/builder/EnsemblePropertyPanel.H
  - src/viz/sgcommands/DisplayAtomsCmd.C
  - tests/apps/vina_poses_large_test.py
issues: [966, 968]
---
Measured with `ECCE_PANEL_TIMING=1` (every part of the panel prints its
elapsed ms on stderr) on the whole of 1IEP plus imatinib, 4673 atoms, nine
poses, software GL:

    updateSelection   n=9  total 9770 ms   max 1271      (per pose selection)
      processStep     n=8  total 5139 ms   max  698      (the trace frame)
      showPocket      n=9  total 5317 ms   max  689
        styleLigand/stylePocket/showAll    ~170 ms each
    initialize        n=1         283 ms                 (opening the panel)
    load              n=1         264 ms
    makeThumbnail     n=24        304 ms   max   58
    fitPocketCamera   n=1           0.1 ms

So opening the panel is a third of a second, and the per-pose work is what
costs: the thumbnails are cheap (the pocket view draws the ligand only), the
RMSD table runs only for two or more selected rows, and the residue search is
a millisecond. The time is in the scene-graph commands, each of which walks
every atom and bond of the receptor, and in the Geometry Trace step.

`showPocket` now applies the display only when the shown set actually
changed: stepping a pose moves the ligand within the same pocket, so the same
atoms stay shown in the same styles. It also shows the pocket and ligand with
one `DisplayAtomsCmd` instead of two: with `all=false` and `selected=false`
the command shows exactly the selection and hides everything else, so the
earlier "show everything, then hide the rest" pair is one pass.
`setPocketView` and `refresh` clear the cached roles so the next call applies
them in full.

A cell makes this worse, not better: a fragment with a lattice draws its
boundary atoms again as cell images (`SGContainer::updateCellImages`) while
the Periodic Builder is open, and every one of these commands then walks
those too — see `crystal-cell-must-not-reach-a-molecular-calculation-968.md`.

The small fixtures hid all of this; `tests/apps/vina_poses_large_test.py` is
the protein-sized case (1IEP whole, fetched once into `tests/dock/fixtures`)
and fails if the panel's open plus first selection goes over a budget.

**After the two changes** (same case, 4673 atoms, eight pose steps): a
selection costs about 446 ms instead of about 1090, the panel opens in
about 0.9 s and the worst step is 555 ms.  Two separate effects, and the
bigger one is not in this panel: the frame step (`processStep`) fell from
5139 to 756 ms with no change to its code, because the structure is no
longer periodic (#968) — a lattice makes the Builder draw the boundary
atoms again as cell images and makes the trace step do lattice work.  The
`showPocket` saving is the second display pass: showing everything and then
hiding the rest (1899 ms over the run) is one `DisplayAtomsCmd` that shows
exactly the selection (1099 ms).  The role cache skipped nothing in this
run — the ligand moves, so the residues within 6 A genuinely change at
every step — and it earns its keep only on a re-selection of the same pose.
