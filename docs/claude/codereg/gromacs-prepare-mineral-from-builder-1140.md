---
type: pitfall
title: "GROMACS Prepare, Mineral from the Builder: the whole structure is typed by `minff_topology.py system`, reordered MIN, ions, SOL; the job skips the water layer and genion when the charge is 0"
area: codereg
paths: [scripts/minff_topology.py, scripts/gensub, src/wxgui/mdtools/GromacsPreparePanel.C, src/apps/gmxprepare/GmxPrepare.C, src/tdat/chemistry/Fragment.C, src/apps/builder/Builder.C, tests/minff/prepare_tests.py, tests/fragreaders/fragreaders.C]
issues: [1140]
---
- The editor writes the task's fragment with `Fragment::dumpGRO` and runs
  `minff_topology.py system` on it. The split is by residue: a residue named
  SOL/HOH/WAT/... with one O and two H is water, a one-atom residue whose
  element min.ff's `ions.itp` has is an ion, everything else is the mineral,
  which atomipy types as one molecule MIN (all layers of a stack together).
- The written `conf.gro` is **reordered**: MIN, then ions grouped by kind,
  then SOL last, because `genion` (run when the charge is not 0) replaces
  waters and appends its ions after the solvent; the `[ molecules ]` list
  follows that order. The structure's atom order is therefore not kept.
- Ion molecules are min.ff's names (`Na`, `Ca`, `Mg`), the atom name is the
  itp's (`Na+`, `Ca2+`), so grompp has nothing to warn about. A 4-site water
  model gets its virtual site placed from the `[ virtual_sites3 ]` coefficients.
- `dumpGRO` gives atoms without a residue one (water -> HOH with OW/HW1/HW2,
  a lone non-H/O atom -> residue named by its element); the rest stays UNK 1.
  A structure read with residues (the Layer Builder from a .gro) needs none of this.
- gensub: SOURCE=minff with SOL in step1.gro sets `MINFF_SYSTEM=1`: DIST is
  ignored (and said so), solvate is skipped, genion runs only for a non-zero
  charge from the ions.tpr grompp, or a salt concentration.
- ClayFF has no "from the Builder" (the choice is hidden with ClayFF).
