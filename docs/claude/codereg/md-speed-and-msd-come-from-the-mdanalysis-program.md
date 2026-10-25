---
type: map
title: "Speed Distribution and Displacement panels read MDSPEED/MDMSD, written by the mdanalysis program into ecce-parse-md.txt; live runs do not make it yet"
area: codereg
paths: [include/util/MdAnalysis.H, include/util/MdTrajectory.H, include/util/MdTopology.H, src/util/mdanalysis, src/apps/mdanalysis/mdanalysis.C, src/apps/builder/MdAnalysisPanels.C, scripts/parsers/gromacs.md, scripts/parsers/gromacs.desc, src/apps/organizer/GromacsImport.C, tests/mdanalysis, tests/apps/md_analysis_panels_test.py, data/client/examples/genchem]
issues: []
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
  `GromacsImport.C` stages the file by name. An imported run gets the
  panels only if its task folder holds `ecce-parse-md.txt`; a live run's
  job script (gensub) does not call the program, so it has none (open).
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
