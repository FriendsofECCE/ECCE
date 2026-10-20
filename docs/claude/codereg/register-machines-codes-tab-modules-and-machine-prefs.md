---
type: pitfall
title: "Codes tab Modules is a machine-file key; the Machine tab's run/scratch/account/username are not"
area: codereg
section: "Machine configuration (CONFIG files)"
paths: ["src/apps/machregister/WxMachineRegister.C", "src/apps/machregister/SchedulerQuery.C", "scripts/gensub"]
issues: []
---
- `<Code>Modules` (a Codes-tab line, gensub key `<code>modules`) makes gensub
  write `module load <line>` just before the code's setup block;
  `addModulePrologue` then defines `module` under sh because the script uses it.
  Find runs the same prologue and load first (`SchedulerQuery::moduleLoadScript`,
  a copy of gensub's `modulePrologue`: keep the two in step); a failed load is
  reported from the text between `ECCE-MODULE-FAILED` and `ECCE-MODULE-END`.
- Run directory, Scratch directory, Account and Username on the Machine tab are
  per-user `MachinePreferences` (written by `saveDefaultAccount`), never
  machine-file keys, so they stay editable-only-by-the-user even when an admin
  registers the machine (disabled in admin mode). `userPrefsDiffer()` makes the
  form dirty.
- A block's name must sit on the row directly above its own box (the site line
  and Copy/Disable are on that row). A setting's label three lines above its
  box once made the Environment box read "User setting"; the hook command
  `check-block-layout` asserts it.
