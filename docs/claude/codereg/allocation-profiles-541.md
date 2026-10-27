---
type: map
title: "Allocation profiles (#541): Default is the MachPrefs line, the rest are in MachProfiles, each calculation stores its own"
area: codereg
section: "Machine configuration (CONFIG files)"
paths: ["include/dsm/AllocationProfiles.H", "include/dsm/MachinePreferences.H", "src/dsm/xml/MachinePreferences.C", "src/apps/launcher/WxLauncher.C", "src/apps/machregister/WxMachineRegister.C", "src/dsm/edsiimpl/TaskJob.C", "src/comm/commxt/LaunchBatch.C", "tests/machregister/launcher_test.py", "tests/machregister/testAllocationProfiles.C"]
issues: [541, 613]
---
- **Storage.** The MachPrefs line of a machine is its profile `Default`
  (queue, run directory, scratch directory and account are its fields 3 to 6);
  its format did not change. Other profiles and the last one used
  per machine are in `~/.ECCE/MachProfiles` (`profile<TAB>machine<TAB>name|
  account|rundir|scratch|queue`, `last<TAB>machine<TAB>name`). A profile
  line must not go into MachPrefs: an older ECCE reads each key there as a
  machine. Older versions never open MachProfiles.
- **Rules** (names, file format, which profile opens) are in
  `AllocationProfiles.H`, header-only, tested by `profiles_test.py`; the
  Launcher only chooses among them (`buildProfileRow`, `refreshProfileControls`;
  the row shows only when the machine has more than one profile).
- **Profiles are made in Register Machines** (Machine tab, "Allocation
  profiles": Add/Edit/Delete; `storeProfile`/`dropProfile`). They are written
  at once through `MachinePreferences::setProfile`/`removeProfile`, not with
  Save. The four boxes of the tab are Default; editing Default there refills
  them and sets the `*Loaded` copies so the form is not left dirty. Disabled
  in admin mode and for a machine that is not registered yet. Hook:
  `profile-set`, `profile-select`, `click profile-delete`, `expect field
  profiles ...` (`gui_test.py`, `allocation_profiles`).
- **Edits go to the profile in use** (`updatePreferences`): typing a second
  allocation's account over Default no longer becomes Default's account.
- **Per calculation.** `launch_profile` and `launch_account` are written with
  the other `launch_*` values by `saveJob`, are skipped by `StoredCalcCopy`
  (another user's session has its own), and `LaunchBatch::buildArgs` submits
  with the stored account (the machine's current one only when none is stored).
  Run and scratch directory, reconnect, cleanup (`CalcMgr`) and the run-dir
  shell (`startShell`) read the calculation's `launch_rundir`/`jobpath`, not
  the machine's preferences.
- The Launcher hook commands `profile`, `account`
  and the `expect` keys `profile`, `profiles`, `profilechoice`, `account`,
  `scratch` are what `launcher_test.py` (`profile_check`) drives.
