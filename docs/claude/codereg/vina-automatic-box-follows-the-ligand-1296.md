---
type: pitfall
title: The automatic Vina box follows the ligand, so a ligand from another file gets a box in empty space
area: codereg
paths: [scripts/parsers/ai.vina, scripts/parsers/vina.launchpp, tests/launch/vina_test.py]
issues: [1296]
---
With `ES.Runtype.Vina.BoxAuto` on, `ai.vina` `SearchBox()` takes the ligand's
bounding box plus the margin. A calculation holds one structure and the
receptor/ligand split is by atom index, so for a ligand drawn in the Builder or
merged with File > Add Structure from File the box sits where that molecule's
coordinates are. Vina searches empty space and returns normal-looking poses and
scores; nothing else reports it.

`vina.launchpp` (runs on every Vina launch) reads `center_*`, `size_*` and
`#| RECEPTOR_ATOMS=` from the staged deck (name from `inputFile` in the `-p`
file) and the coordinates from `vina.vinafrag`, and counts receptor atoms in
the box. Under 10 it prints `ECCE-LAUNCH-WARNING` and the text, exit 0 (see
`launch-check-reports-through-a-marker-not-an-exit-status.md`). The test is
geometric, not provenance: provenance is not stored, and a cognate ligand
displaced from its pocket is caught as well. The deck does not record
`BoxAuto`, so the text says "unless it was set by hand". Anything missing or
unparseable exits 0 silently.
