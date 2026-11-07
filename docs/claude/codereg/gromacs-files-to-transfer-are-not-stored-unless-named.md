---
type: pitfall
title: "GROMACS filesToTransfer reach the job store's tmp dir but are stored only when JobStore names them"
area: codereg
section: "GROMACS"
paths: ["src/comm/commxt/Launch.C", "src/comm/commxt/JobStore.C", "scripts/eccejobmonitor", "scripts/ecce_gmxanalysis.py"]
issues: [877]
---
**A file in the monitor's `filesToTransfer` is copied back but not kept.**
The monitor sends it as type `other`; `JobStore::interactFile` stores an
`other` file with the calculation only when it says so (`dbStoreName`), and
until #877 step 2 it never did: `traj.xtc ener.edr topol.tpr` were fetched
into the job store's temp directory and deleted with it, silently. ORCA's
`.gbw` only survives through the separate `RetainOutputFile` /
`RetainAuxiliaryOutput` branch. `isGromacsResult()` now names the GROMACS
files (`traj.xtc traj.trr ener.edr topol.tpr ecce_pp.top`); a new name added
to Launch's list must be added there too, and to the monitor's size-capped
list if it can be large. They land in `<calc>/Outputs/`, which is where
`ecce-gmxanalysis` and the MD Analysis panel look.

Two more traps on the same path:
- `gmx energy` with only `0` on stdin lists the terms and then **exits 1**;
  read the menu without treating that as failure, and pick terms by number,
  since a name is matched as a prefix (`Pres` hits several terms).
- gmx's selection language has no element or molecule-type keyword
  (`element O` fails in 2025.2), so groups are written as an index file from
  `gmx dump -s topol.tpr` (moltype names, residue names, atom names,
  atomnumber, mass). `gmx hbond` without `-t` waits on the terminal for a
  selection: pass `-t` equal to `-r` for "within one group".
