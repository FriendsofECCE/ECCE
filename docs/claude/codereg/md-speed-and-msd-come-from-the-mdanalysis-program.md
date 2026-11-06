---
type: map
title: "Speed Distribution and Displacement panels read MDSPEED/MDMSD, written by the mdanalysis program into ecce-parse-md.txt, in the job script and on import"
area: codereg
paths: [scripts/gensub, tests/launch/gromacs_md_analysis_test.py, include/util/MdAnalysis.H, include/util/MdTrajectory.H, include/util/MdTopology.H, src/util/mdanalysis, src/apps/mdanalysis/mdanalysis.C, src/apps/builder/MdAnalysisPanels.C, scripts/parsers/gromacs.md, scripts/parsers/gromacs.desc, src/apps/organizer/GromacsImport.C, tests/mdanalysis, tests/apps/md_analysis_panels_test.py, data/client/examples/genchem]
issues: [877]
---
- ECCE had no trajectory reader (GEOMTRACE is a text file of <= 100 frames
  that gensub cuts with gmx). `MdTrajectoryReader` reads .trr (float or
  double) and .xtc (the compressed coder, ported to C++) natively; the
  result was checked frame by frame through D against `gmx msd`
  (`tests/mdanalysis/gmx_oracle.py`, set `ECCE_MD_RESULTS`).
- The analysis is not done in the panel: `ecce-mdanalysis` (bin/) writes
  `ecce-parse-md.txt` (`ECCE-MDSPEED`/`ECCE-MDMSD` records without blanks);
  `gromacs.md` turns them into the PropVecString keys MDSPEED and MDMSD
  (`gromacs.desc`, File= parse like the pull and trajectory files);
  `GromacsImport.C` stages the file by name.
- Every dynamics job runs it after mdrun (gensub `gromacsMdAnalysis`):
  `$ECCE_HOME/bin/mdanalysis` (client's path, so only where ECCE is
  installed at that path; else `ECCE_MDANALYSIS`, `ecce-mdanalysis` on PATH,
  else a log line). The topology is grompp's `-pp ecce_pp.top`: the reader
  follows only relative `#include`s and ignores `#ifdef`, so a raw
  `topol.top` with `oplsaa.ff/...` has no atoms. No velocities (`nstvout`
  0) gives the MDSPEED record `none`: decodeSpeed fails on it and the Speed
  panel shows its "No velocities were saved" text instead of not appearing.
  An import without the file runs the same (`analyseTask`, grompp -pp when
  gmx is found and the task has a start .gro).
- Speeds need velocities (`nstvout`), the MSD positions; the MSD unwraps
  molecule centres by the nearest-image step between frames, so frames
  must be closer than half a box over the fastest speed (He at 1 ps in a
  12.7 nm box is fine, 4 ps would not be).
- Atoms of equal mass form one speed curve, labelled by element
  (`elementForMass`), else by atom type. The topology reader skips
  `#include` files it cannot find; only an atom with no mass on its
  [ atoms ] line then fails.
- MDMSD/GEOMTRACE of a long run arrive minutes after the energies: file-parse-properties-are-stored-last.md.
- The General Chemistry example set ships 26 trajectory frames per run plus
  the precomputed `ecce-parse-md.txt`, not the .xtc/.trr (up to 122 MB).
