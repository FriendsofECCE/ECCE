---
type: pitfall
title: "Windows MSI: install time goes by the file and the component; Strawberry keeps core modules in vendor\\lib"
area: services
paths: ["packaging/windows/make-msi.ps1", "packaging/windows/bundle-tools.ps1", "packaging/windows/perl-core-only.pl"]
issues: [257]
---
Measured on NerdEgg (Windows 10, Defender on) with the 9.0.0-beta.1 CI MSI,
12 143 files in 12 193 components: install 66 s, of which the file copy was
48 s (~4 ms a file) and component registration 6 s; uninstall 104 s (48 s
removing files, two operations per file, 7 s unregistering components). A
plain .NET unzip of the same files took 28 s, so Defender and the file
count set the floor, not the cabinet.

- `make-msi.ps1` makes one component per directory (GUID from the path);
  the major upgrade is scheduled `afterInstallValidate`, so the old product
  is removed before the new one is installed and a directory's file list may
  change between versions.
- `bundle-tools.ps1` keeps only perl's core library: ECCE's scripts use
  core modules only. **Strawberry moves the newer releases of dual-life core
  modules (Socket, Scalar::Util, ...) into `perl\vendor\lib`**, so deleting
  `vendor\lib` broke gensub ("Can't locate Socket.pm"); `perl-core-only.pl`
  removes only what `Module::CoreList` does not list for this perl.
- Check a pruned tree with `perl -c` over every script in `scripts\` and
  `tests/windows/launch_local.py complete|cancel` (WINTEST_HOME).
