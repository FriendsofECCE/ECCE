---
type: map
title: "A remote run folder is a marked copy of the calculation (#1083): staged with TreeCopy, sent with the launch's own put"
area: services
paths: [src/dsm/edsiimpl/RunFolderCopy.C, include/dsm/RunFolderCopy.H, src/comm/commxt/Launch.C, src/comm/commxt/JobStore.C, scripts/eccejobmonitor, src/wxgui/wxtools/WxResourceTreeCtrl.C, src/apps/organizer/CalcMgr.C]
issues: [1083, 890, 767]
---
- `RunFolderCopy::stage` copies the stored calculation with `TreeCopy` into a
  local `file://` folder (`.ecce-runcopy` in the launch directory or the job
  store's tmpStorage), drops the top-level folders not wanted, drops Outputs
  files over 50 MB, and adds `ecce:copyOf/copyOrigin/copyOriginRoot/copyMadeAt`
  (and the state) to the root's sidecar through FileEDSI. FileEDSI and SftpEDSI
  share the sidecar format, so the staged `.ecce-meta` files are sent as they are.
- Sending uses no new transport: `Launch::writeRunFolderCopy` (end of
  `moveFiles`) uses `putOnLogin || RCommand::put` like the deck;
  `updateRunFolderCopy` in the job store's `cleanup` uses `copyOnLogin ||
  RCommand::put` like `copyOutputs`, after `stopStream` (copyOnLogin fails while
  a stream is open). `mkdir -p` takes the relative folder because both
  connections are already in the run folder, while puts take paths from the
  remote home.
- Only over a remote login (not `shellcp`, not a frontend) and not for a calc
  stored on an SSH folder: a local run folder or an SSH-folder calc's run folder
  can be the calculation itself or lie inside it, and a sidecar written there
  would replace the original's. `ECCE_RUN_FOLDER_COPY=0` turns it off.
- The monitor (`CopyMetaState`) rewrites only the `.` state record and only in
  a sidecar that has `copyOf`, under the same `.ecce-meta.lockdir` SftpEDSI
  takes. Its Complete is provisional: the store can downgrade (#732), and the
  final upload replaces it.
- A failure never fails the launch: the launch adds a warning, the store a
  job-log line.
- The tree label (`WxResourceTreeCtrl::itemLabel`) adds the note; the test
  hook `treechildren URL` returns labels, so a copy reads `name  (copy of ...)`.
  **Open the Original** is disabled for a `file://` original from another host,
  since that path names a folder on the launching machine.
- Not done: Import as calculation for older run folders (follow-up issue).
