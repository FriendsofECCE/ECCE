---
type: map
title: "Codes tab: one table gives the Program examples, hints and Find names; the Default account lives in MachPrefs"
area: codereg
section: "Machine configuration (CONFIG files)"
paths: ["src/apps/machregister/SchedulerQuery.C", "src/apps/machregister/WxMachineRegisterScript.C", "src/apps/machregister/WxMachineRegister.C", "src/apps/machregister/WxMachineRegisterTools.C", "src/dsm/xml/CodeFactory.C"]
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
- `ProgramHelp::signature` is a byte string the real program's file contains;
  Find and the **Check** button search the file with `grep -a -F` after
  `readlink -f` and never run it (running an unknown `orca` starts the
  screen reader). States: yes / no / missing / found (empty signature, so
  never checked or warned about). Find lists confirmed candidates first and
  marks the rest "not confirmed"; Check only reports (`code:check` label,
  hook command `check-program <code> <path>`); a wrapper script is "no",
  which the text says is expected. Warn, never block. Signatures were taken
  from real binaries (g09/g16 "Entering Gaussian System", ORCA 6.1.1 "ORCA_",
  Debian nwchem "NWChem", pw.x "PWSCF", mopac "MOPAC", gmx "GROMACS", cp2k
  "CP2K") and each was checked to be absent from the others; "ORCA" alone is
  also in the screen reader script, and "MOPAC" also occurs in cp2k.psmp.
  GAMESS-US, Polyrate, Gaussian-03/98 and the rest have none: check a new
  binary with `grep -a -c -F` before adding one.
- On Windows, for this computer, the examples are Windows paths
  (`programHelp(code, true)`); when nothing is on the PATH, Find looks in a folder named after
  the code below the user folder, `AppData\Local[\Programs]`, Program Files
  and the drive root (`findInstallDirsScript`); elsewhere it looks below
  `$HOME`, `/opt` and `/usr/local` (and `/Applications` on a Mac). A test
  that moves `USERPROFILE` to a fresh profile hides a real install there.
- The frame grows after a code is shown (`showCode` -> `growToFitSizer`):
  a code's texts can widen the page, and the pages' vertical scroll bar
  is added to their natural width; both cut off the right-hand column.
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
