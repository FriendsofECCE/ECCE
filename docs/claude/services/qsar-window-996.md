---
type: map
title: QSAR window (Tools > QSAR..., #996)
area: services
paths: [src/apps/organizer/QsarWindow.C, src/apps/organizer/QsarWindow.H, src/apps/organizer/CalcMgr.C, tests/apps/qsar_pca_test.py, tests/apps/qsar_pls_test.py, tests/apps/qsar_table_test.py]
issues: [996]
---
- One `QsarWindow` (wxFrame, Table | PCA | PLS tabs) per folder replaces the old
  QSAR Table dialog (ChecksDialog QSAR mode, now only used by Checks/Rank) and
  `QsarDialog`. CalcMgr keeps the open windows and raises an existing one for
  the same folder (parent path of the first calculation).
- The window owns the table (`table.csv` in its scratch dir); PCA/PLS run
  `ecce-qsar` on it and read its CSV files. The "T2 limit", "R2Y ... Q2 ..."
  lines of the CLI report are parsed with sscanf, so changing those lines in
  `ecce_qsar.py` breaks the status line and the outlier marks.
- Test hooks `qsartable|qsarpca|qsarpls` build a window with the same code
  path; with a png argument the hook captures the chosen tab. The outcome is
  `ok | <status line>`.
- A PLS descriptor that estimates the modelled property (SlogP for logP, SMR,
  TPSA) is flagged and left out by default via `--exclude`.
