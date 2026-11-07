---
type: map
title: "Organizer auto-refresh of the shown folder (#889): watch local, poll remote, only when idle"
area: services
paths: ["src/apps/organizer/CalcMgr.C", "src/dsm/edsiimpl/SftpEDSI.C", "tests/apps/organizer_autorefresh_test.py"]
issues: [889]
---
- The shown folder is the selected collection (a selected calculation is not
  auto-refreshed: its panel would be rebuilt under the user). Local: a
  non-recursive `wxFileSystemWatcher` (create/delete/rename only; a job writes
  inside calculation folders and `.ecce-meta` rewrites are ECCE's own), 1 s
  debounce, then `refresh(folder)` + `onSelectionChange(true)`. Server/SSH: a
  30 s timer and window activation compare one stamp (`SftpEDSI::folderStamp`
  = one `stat`; DAV `getlastmodified`).
- `autoRefreshIdle()` defers (retry each second) while a modal dialog, popup
  menu, drag, text control focus, mouse capture, startDisabler or a launch
  batch is active. `p_uiBusy` brackets `ewxPopupMenu` and `DoDragDrop`.
- Hooks: `select URL`, `selection`, `treechildren URL`, `autoreloads`,
  `autoshown`, `modal N`. Not kept across a reload: the project table's row
  selection and scroll.
