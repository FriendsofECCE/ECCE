---
type: map
title: "Register Machines is a tabbed frame over a draft; `ECCE_MACHREG_SCRIPT` drives the real window"
area: codereg
section: "Machine configuration (CONFIG files)"
paths: ["src/apps/machregister/WxMachineRegister.C", "src/apps/machregister/WxMachineRegisterScript.C", "tests/machregister/gui_test.py", "scripts/processmachine"]
issues: ["212"]
---
The frame (Machine, Connection, Codes, Job script, Queues tabs) is hand-written;
there is no generated base class or `.pjd` any more. What the form holds is
pushed into a `MachineConfigDraft` on every change (`syncDraft()`), so Save is
enabled, and the title gets a `*`, exactly when the draft differs from what was
loaded, or the queue form differs from the selected queue.

Save is two writers. `ProcessMachine::run` writes `Machines`/`MyMachines`,
`Queues` and `<m>.Q`, with `config=external` so `processmachine` neither
rewrites nor unlocks `CONFIG.<m>` (and sees each code's *effective* path, so the
Machines code list includes codes whose path is inherited). Then
`MachineConfigDraft::applyTo` + `ConfigFile::save` write `CONFIG.<m>`. If the
second step fails the message names the file; the first has already been
written.

Things that are easy to get wrong:

- An emptied field whose value is inherited writes `key: -` ("no value"); an
  emptied field with nothing inherited removes the line.
- A changed Name is a new machine: it gets a draft of its own, because the
  loaded one belongs to the old name.
- The Connection tab's rows (`addCfgRow`) are one key each: control, source
  tag, undo button (only on the user's own value). The noRemoteAccess/userSubmit
  keys are edited through two exclusive checkboxes (`jobs:user`, `jobs:none`)
  under Advanced that drive two hidden ones; neither ticked is normal; noRemoteAccess wins when both keys are set. The draft comes from `GENSUB_EXPLAIN` (`explain()`), and
  these twelve keys are read by C++ only, so their value and tag ignore
  `submit.site` and vendor layers (`effective`/`tag` with `cppOnly`). `true` and
  `yes` are one value for the check/choice rows, so `syncCfg` compares them in
  canonical form: an absent `singleConnect` means "no", not "auto".
- The Job script tab edits three text blocks (`BlockRow`): the header named
  after the Queues tab's queue manager (`slurm`, `pbs`, ...), `setup` and
  `wrapup`. A block's text *replaces* the inherited text (gensub never merges
  them); the inherited text is shown read-only above, from gensub's layers
  (`cppOnly` false, so `submit.site` counts), the box below holds only the
  user's own. An empty box inherits; "Use no text" writes `key: -`; copy
  fills the box with the inherited text. Switching queue manager syncs the
  old header key first (`blocksRetarget`). csh in setup/wrapup is checked by
  `ecce-csh2sh --check` on the box text (600 ms after typing, and on Save) and
  only reported, as gensub reports it at submit time. `condorAllowTmp` is a
  `CfgRow` with `gensubOnly`, shown for HTCondor only.
- "Advanced: edit file" edits the raw edited-layer file in a dialog (Check /
  Save; an unclosed block is an error, unknown keys and csh are warnings) and
  saves through `ConfigFile::setText` + `save`. Unsaved form changes are saved
  or discarded first; the form is reloaded afterwards. In the hook the dialog
  is not modal: fields `raw:text`, `raw:report`, `raw:check`, `raw:save`,
  `raw:cancel`; `shot-dialog FILE` captures it.
- Widgets only reach the draft through change events that bubble to the frame;
  programmatic fills are guarded by `p_inCtrlUpdate`.

**Test hook.** With `ECCE_MACHREG_SCRIPT=<file>` the app runs the commands in the
file against the real frame (one per timer tick; they call the button handlers),
does not subscribe or publish, answers prompts from `answer yes|no|cancel`, and
prints the binary and `$ECCE_HOME`. `snapshot <dir>` saves one PNG per tab. Source tags are fields named
`tag:<key>` (`expect label tag:shell site`), `undo <key>` is a row's undo button (`undo jobs` for the job radios). A
`wxScreenDC` capture goes stale after its first use under Xvfb; the hook blits
from a `wxClientDC` of the frame instead. `tests/machregister/gui_test.py`
(ctest `machregister_gui`) runs user, `-remote` and `-admin` scenarios and then
reads the files back on its own.
