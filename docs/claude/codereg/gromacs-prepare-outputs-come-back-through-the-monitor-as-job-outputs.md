---
type: map
title: "GROMACS Prepare: how its outputs reach the next task (.gmxtop, .gro, posre.itp, topol_*.itp), and the places that treat a Prepare as NWChem's"
area: codereg
paths: [scripts/eccejobmonitor, src/comm/commxt/Launch.C, src/dsm/edsiimpl/MdTask.C, src/dsm/edsiimpl/TaskJob.C, include/dsm/GromacsPrepareModel.H, src/dsm/xml/GromacsPrepareModelXMLizer.C, scripts/parsers/md.gmxprep, tests/launch/gromacs_prepare_test.py]
issues: [930]
---
A GROMACS Prepare (`gromacs_md_prepare`, content type `ecceMdPrepare`,
application `GROMACS`) is stored and run through the paths NWChem's Prepare
and GROMACS's other tasks already use; what had to be told apart:

- **Outputs by name, from Launch's `##` arguments.** `MdTask::getTopologyName`
  is `<sys>.gmxtop` for a GROMACS Prepare (`""` for its other tasks), so
  `jobOutputFile5` brings the topology back; `getRestartName` is
  `<sys>_<calc>.gro` (`jobOutputFile4`), `chemsys.pdb` is the fragment
  (`jobOutputFile3`).
- **Includes by extension.** `posre.itp` and one `topol_<mol>_chain_X.itp`
  per chain have names only the run knows, and `filesToTransfer` does not
  glob (and its files are never stored). `eccejobmonitor` therefore sends
  every `*.itp` of a run with `mdPrepareTask yes` as a job output (like
  `*.rst`), which `eccejobstore` stores under the include type
  (`chemical/x-gromacs-include`), where `Launch::auxInputUrls` and the
  provider search of the later tasks find them. A later task needs no
  attachment at all: it finds topology, structure and includes on the Prepare.
- **A Prepare's own old outputs are not its inputs.** `Launch` searches the
  task itself first for a GROMACS task's topology and structure; for a
  Prepare that would stage its previous run's `topol.top`/`conf.gro` on a
  rerun, so it is skipped for `CT_MDPREPARE`.
- **The source structure** is the task's `.gmxpdb` input (an auxiliary input;
  `.pdb` would be the task's fragment and `TaskJob::getDataFiles` matches by
  type). `Launch` writes `<sys>.pdb` only when there is a fragment, and only
  then ships it; a Prepare with neither fragment nor `.gmxpdb` is refused.
  `auxInputUrls` does not carry an earlier task's `.gmxpdb` to later tasks.
- **Recipe.** `TaskJob::getSerializer` gives a GROMACS Prepare
  `GromacsPrepareModelXMLizer` (not NWChem's `PrepareModelXMLizer`);
  `md.gmxprep` reads that XML. The editor must run `md.gmxprep`, not
  `md.gmxtask`, and store the result as `gromacs.gmxprep`: GROMACS.edml
  declares two primary inputs and `MDEdBase::generateInputFile` takes the
  first (`gromacs.mdp`). Once only `gromacs.gmxprep` is stored on a task,
  `##input##` resolves to it.
- **A failing job ends `unsuccessful`**, not `failed` (the gmx tool's
  `Fatal error` paragraph is the reason). `genion` accepts any ion name
  (a `QQ` ion is made, uncharged), so a bad name does not fail the job; a
  salt concentration beyond the water in the box does ("Not enough solvent
  for adding ions").
- **GROMOS.** The tasks after a Prepare use `grompp -maxwarn 1` when the
  topology's force-field `#include` is `gromos*.ff/...` (`gensub`,
  `gromacs.launchpp`); any other warning still stops the job.

Check: `tests/launch/gromacs_prepare_test.py` (`ctest -R launch_gromacs_prepare`)
runs Trp-cage through Prepare and Optimize (nothing attached to the Optimize)
and the Organizer hook `new <study> gromacs_md_prepare`.
