---
type: pitfall
title: "Without Print[P_OrbEn] 2, ORCA labels only the occupied orbitals and ten virtuals, so ORBSYM is short and every reader drops it"
area: codereg
paths: ["scripts/parsers/orca.orbsym", "scripts/parsers/orca.desc", "src/apps/builder/MoDiagramPanel.C", "src/apps/builder/MoPanel.C", "src/apps/builder/DensityProfilePanel.C"]
issues: [981]
---
ORCA's ORBITAL ENERGIES block, the only place it prints irreps (with
`UseSym`), stops after the first ten virtual orbitals unless `%output
Print[P_OrbEn] 2 end` is given ("*Only the first 10 virtual orbitals were
printed."). The MO coefficient block (`Print[P_MOs] 1`) carries no irreps.
Jobs ECCE writes include `Print[P_OrbEn] 2`; outputs run outside ECCE and
imported often do not.

`orca.orbsym` then writes an ORBSYM shorter than ORBENG (16 of 39 for
H2O def2-SVPD). Its gap check only catches an unmatched row in the middle.
Every reader (`MoPanel::fillTable`, `MoDiagramPanel`, `DensityProfilePanel`)
ignores an ORBSYM of the wrong length, because energies and labels are
indexed by the same row. So the calculation has no usable labels: no Sym
column, and no MO correlation diagram. The short vector is kept on purpose
as the sign that the job did use symmetry: the MO diagram tells "only some
orbitals are labelled" (add `Print[P_OrbEn] 2`, rerun) apart from "run
without symmetry". Nothing can recover the missing labels from such an
output. `tests/apps/calcimport_test.py` imports one
(`orca/h2o_svpd_sym.out`, `mo 1 orbsym 1`).
