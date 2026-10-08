---
type: pitfall
title: "ORCA 6.1.1: no orca_scf next to orca (Find), and the Intel Mac build needs macOS 12.3"
area: codereg
paths: [src/apps/machregister/SchedulerQuery.C, tests/machregister/gui_test.py]
issues: [133]
---
Register Machines' **Find** accepts an `orca` only if `orca_scf` sits next
to it (to skip the GNOME screen reader). ORCA 6.x has no `orca_scf`
(`orca_leanscf` instead), so Find found no ORCA at all, on Linux too; the
companion is now `orca_2mkl`, present in ORCA 4 to 6. Separately, the
macOS Intel tarball's libraries are built for macOS 12.3: on 10.15 `orca`
aborts with `dyld: Symbol not found: ...basic_filebuf...open` and ECCE
reports "job exited with status 134", state Unsuccessful.
