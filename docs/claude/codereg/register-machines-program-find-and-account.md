---
type: map
title: "Codes tab: one table gives the Program examples, hints and Find names; the Default account lives in MachPrefs"
area: codereg
section: "Machine configuration (CONFIG files)"
paths: ["src/apps/machregister/SchedulerQuery.C", "src/apps/machregister/WxMachineRegister.C", "src/apps/machregister/WxMachineRegisterTools.C", "src/dsm/xml/CodeFactory.C"]
issues: [234]
---
- `SchedulerQuery::programHelp(code)` is the only place that knows a code's
  executable name (what gensub runs as `$<code>`): the examples under the
  Program box, the box's hint, and the names Find looks up with `command -v`
  on the machine over its connection (login setup first). The hook command
  `check-program-hints` fails if an example's basename is not a name Find
  looks for. A code whose name a program shares (ORCA and the GNOME screen
  reader) needs a `companion` file; its search walks the PATH, because
  `command -v` returns only the first match.
- The Codes list shows a code only when a resource descriptor registers it
  (`CodeFactory::isRegistered`: an `applicationType` in
  ResourceDescriptor*.xml) or the machine already has a setting for it. A
  code with an .edml and a gensub sub but no descriptor (GROMACS) is
  groundwork and stays hidden.
- The Default account (Queues tab) is per user and stored in MachPrefs, where
  the Launcher reads it; it is not part of the draft, so `isDirty` compares it
  with `p_accountLoaded` and Save writes it after the settings files. The
  Account boxes of Preview and Test submission start as it and are always
  editable (they used to be disabled unless "Allocation accounts used" was
  ticked, so a site that requires `--account` could not be tested).
- Test submission shows any failure with the command and the output; a
  command that printed nothing (timeout, login failure) gets a stated reason.
