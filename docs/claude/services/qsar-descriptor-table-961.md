---
type: map
title: QSAR descriptor table (ecce-qsar, #961)
area: services
paths: [scripts/ecce_qsar.py, scripts/ecce_qsar_chem.py, scripts/ecce_qsar_crippen.py, src/apps/organizer/SiExport.C, src/apps/organizer/ChecksDialog.C, tests/qsar/descriptors_test.py, tests/apps/qsar_table_test.py]
issues: [961]
---
- The Organizer table is `ChecksDialog` with a QSAR mode: `SiExport::writeFolder`
  makes a scratch folder per calculation (now also ORBENG, ORBOCC, DIPOLE,
  MULLIKEN and `ecce:qsar.*`), `ecce-qsar table` reads the folders. Anything the
  script reads must therefore be added to `writeFolder` too, or the GUI table
  has an empty column that the CLI on the stored folder does not.
- Descriptors come from the stored SMILES (wx-free, pure Python); there is no
  perception from 3-D coordinates, so a calculation not made by Batch Add from
  SMILES has no descriptors (the row says "no SMILES stored").
- Gasteiger follows RDKit: halogens sp3, amide/aniline N sp2, S sp3 in sulfones.
  Open Babel differs on those (and on charged amines); only compare it on
  molecules without them. Open Babel's MR table differs from RDKit's for H.
- SMARTS typing of Crippen uses a small matcher (`_parse_smarts`), enough for
  the patterns of `ecce_qsar_crippen.py`; a new pattern with recursive SMARTS
  or ring bonds is not supported.
- `ecce_qsar_crippen.py` is RDKit's Crippen.txt (BSD 3-clause); keep its notice.
- PCA/PLS (`ecce_qsar_chemo.py`, `ecce-qsar pca`): Q2 of PCA must be Wold's
  element-wise CV; leaving whole rows out makes PRESS fall with every extra
  component, so the "significant components" rule would never stop. The
  Organizer dialog (`QsarDialog`) only runs `ecce-qsar` and draws its CSV files.
