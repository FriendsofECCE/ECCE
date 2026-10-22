---
type: pitfall
title: "Windows MSI: Repair waits ~85 s before its progress page; uninstall time is the per-file script"
area: services
paths: ["packaging/windows/make-msi.ps1"]
issues: [431]
---
Verbose logs (`msiexec /l*v`) from NerdEgg, 9.3.1 MSI, ~7600 files:

- **Progress-page placeholders** ("Copying new files File: [1], Directory: [9],
  Size: [6]"): Windows Installer prints an action's default template when the
  ActionText table has no row for it. `make-msi.ps1` adds `ProgressText` rows
  with a Message and *no* Template (wix rejects `Template=""`).
- **"Not Responding" after Repair**: the click on Repair sets
  `REINSTALLMODE=ecmus` (WixUI's VerifyReadyDlg) and the client process
  (`MSI (c)`) then spends ~83 s before the progress dialog is created, with
  nothing logged. The server step afterwards re-copies every file (~80 s).
  Both come from the `e`/`c` flags: unversioned files are compared by hash
  and an equal "version" is reinstalled. The dialog thread is blocked, hence
  the "Not Responding". Not changed: dropping `c`/`e` (`omus`) would make
  Repair stop detecting corrupt files, which is its purpose. Fewer files
  (#257) is the real fix.
- **Uninstall** (54 s on NerdEgg): `InstallFinalize`, the script of one
  RemoveFile and one registry operation per file and component, is 44 s of it;
  `RemoveFiles` scheduling is 2 s, `InstallValidate` 4 s. Same per-file
  floor as in `windows-msi-time-goes-by-file.md`.
