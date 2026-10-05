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
- Widgets only reach the draft through change events that bubble to the frame;
  programmatic fills are guarded by `p_inCtrlUpdate`.

**Test hook.** With `ECCE_MACHREG_SCRIPT=<file>` the app runs the commands in the
file against the real frame (one per timer tick; they call the button handlers),
does not subscribe or publish, answers prompts from `answer yes|no|cancel`, and
prints the binary and `$ECCE_HOME`. `snapshot <dir>` saves one PNG per tab. A
`wxScreenDC` capture goes stale after its first use under Xvfb; the hook blits
from a `wxClientDC` of the frame instead. `tests/machregister/gui_test.py`
(ctest `machregister_gui`) runs user, `-remote` and `-admin` scenarios and then
reads the files back on its own.
