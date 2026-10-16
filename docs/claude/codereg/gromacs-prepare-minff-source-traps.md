---
type: pitfall
title: "GROMACS Prepare, MINFF source: built on the client and staged as .gmxmin plus includes; a layer split over the z boundary tears when water is added; genion names ions differently from ions.itp"
area: codereg
paths: [scripts/minff_topology.py, scripts/gensub, scripts/parsers/md.gmxprep, scripts/parsers/gromacs.launchpp, src/wxgui/mdtools/GromacsPreparePanel.C, src/comm/commxt/Launch.C, src/dsm/edsiimpl/MdTask.C, tests/minff/prepare_tests.py, tests/launch/gromacs_prepare_test.py]
issues: [930, 241, 262]
---
- **Where it runs.** The editor's Build button runs `$ECCE_HOME/scripts/minff_topology.py`
  on the client (async `wxExecute`, progress dialog) and stores the result as
  the Prepare's auxiliary inputs: `conf.gmxmin` (structure and cell),
  `topol.gmxmin` (topology) and every include `GromacsIncludes::collect`
  finds (`min.itp`, `min.ff__SLASH__forcefield.itp`, ...). The job script
  (`gensub`, SOURCE=minff) restores the folders, copies them to `topol.top` /
  `step1.gro` and skips pdb2gmx. `.gmxmin` is Launch's "source for this
  Prepare only" like `.gmxpdb` (`isGromacsSourcePdb`): it is not carried to
  later tasks, the includes are (a Prepare with neither a Builder fragment nor
  such a file is refused). Rebuilding replaces all of them.
- **Layer split over z.** A layer that straddles the z boundary of its cell
  (Andy's montmorillonite does) is bonded across it; lengthening the cell for
  a water layer then leaves bonds 3.5 nm long and mdrun stops with "no domain
  decomposition ... minimum cell size". `minff_topology.py` therefore moves
  atoms by whole cell vectors so the largest gap in z is at the top
  (`contiguous_z`). The water layer is added to the c component and the tilt is
  kept (`editconf -box` would drop it).
- **Ion atom names.** genion writes the ion's atom as `Na` but min.ff's
  ions.itp names it `Na+` (and `Cl-`): grompp then warns "non-matching atom
  names" and every later task needs `-maxwarn`. The job renames the atoms in
  the .gro to the topology's names (read from `grompp -pp`). Ion molecule
  names are mixed case (`Na`, `Cu2+`), unlike pdb2gmx's `NA`; `md.gmxprep`
  holds the list and derives the charge (`PQ`, passed to `genion -pq`).
- **Cut-off in ions.mdp.** A small mineral cell cannot hold a 1 nm cut-off
  ("cut-off longer than half the shortest box vector"); the MINFF job uses
  0.5 nm for the tpr genion and the charge check read (it never runs).
- **No water, charged mineral.** genion needs solvent, so with a layer of 0 a
  charged mineral stays charged, and PME makes grompp warn; the following tasks
  need `-maxwarn 1`. The summary shows the charge.
- **Periodic molecules.** The Optimize editor takes `periodic-molecules = yes`
  from the cell through `MdTask::getFragmentSummary`; that looked for a
  task's output structure among its *configurations*, never under Outputs, so
  a finished Prepare's `chemsys.pdb` was invisible and the .mdp said false.
  `getTaskFragmentSummary` now looks under Outputs first.
- **Bundling.** `minff_topology.py` looks first for a bundled atomipy
  (`<root>/lib/python/atomipy/`) and min.ff (`<root>/data/minff/min.ff/`),
  `<root>` being `$ECCE_HOME` or the tree the script sits in, then
  `ECCE_ATOMIPY_PYTHON` / `ECCE_MINFF_DIR`, then the pinned download. A
  bundled atomipy still needs numpy, scipy and tqdm from the python that runs
  it.
