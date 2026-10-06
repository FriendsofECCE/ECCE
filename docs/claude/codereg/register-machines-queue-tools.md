---
type: map
title: "Register Machines asks the scheduler (discover, test) and previews the job script from the unsaved form"
area: codereg
section: "Machine configuration (CONFIG files)"
paths: ["src/apps/machregister/WxMachineRegisterTools.C", "src/apps/machregister/SchedulerQuery.C", "src/apps/machregister/JobPreview.C", "scripts/gensub", "tests/queues/stubsched.py", "tests/machregister/gui_test.py"]
issues: ["212"]
---
Three buttons, one file for the dialogs (`WxMachineRegisterTools.C`), two wx-free
helpers (`SchedulerQuery`, `JobPreview`).

- **Discover queues...** (Queues tab). `SchedulerQuery::discover` runs the
  manager's listing through `RCommand` (Slurm `sinfo -h -o '%R|%l|%D|%c|%m'`, PBS
  `qstat -Qf`, SGE `qconf -sql` + `-sq`, LSF `bqueues -l`, HTCondor
  `condor_status -af Cpus Memory` as one queue `pool`; Moab has none). Every
  command is an argv whose words are single-quoted (`quote`), because RCommand
  takes a command string. The connection comes from the *form*, not the saved
  files: `connection()` reads `qmgrPath`, `perlPath`, `xappsPath`, `libPath`,
  `sourceFile`, `shell` and the front-end keys from the draft and builds the
  PATH with `RefMachine::shellPathFor`, the same function `shellPath()` uses, so
  the Launcher and this agree. The login name and remote shell are the saved
  `MachinePreferences` ones; there is no password (keys or local).
  Rows land in `p_queues` through `addDiscovered`: limits only, a queue already
  there keeps its defaults (a default above a new limit is zeroed, because Add
  Queue would refuse it). Memory units: the scheduler's "gb" is ECCE's GB, 1000 MB.
  Slurm's memory is per node and its processors are the partition's total.
- **Preview job script...** (Job script tab and Codes tab). The form's draft is
  written as the file Save would write (`draftConfigText`, the `writeConfig`
  path without `save`) into a temporary directory and gensub reads it through
  `GENSUB_USER_CONFIG`, or `GENSUB_SITE_CONFIG` under `-admin` (with
  `ECCE_REALUSERHOME` pointing at an empty directory). `GENSUB_ANNOTATE=1`
  makes gensub print `#@ecce-section <part> <layer>` marker lines where a CONFIG
  key supplies text (`request`, `before`, `environment`, `command`, `after`,
  then `ecce`); `JobPreview::generate` strips them and tags the lines that
  follow. Without the variable gensub's output is unchanged (the golden scripts
  in `tests/queues/golden` are the check). The layer is the file that last set
  the key, recognised by name, so a draft file counts as `user` or `site`.
- **Test submission...** (Queues tab). Copies a script made of the request lines
  alone (plus `exit 0`) to the home directory on the machine, runs
  `sbatch --test-only`, `qsub -verify` or HTCondor's `condor_submit -dry-run` on
  it, and prints the scheduler's answer verbatim. PBS, LSF and Moab have no
  dry run: `qsub -h`, `bsub -H`, `msub -h` and then the cancel command, behind
  a checkbox that must be ticked first. The script has no calculation in it, so
  a failed cancel leaves a held job, never a running one; the dialog says so and
  names the cancel command. The script file is removed afterwards.

Things that are easy to get wrong:

- The dialogs are not modal while `ECCE_MACHREG_SCRIPT` runs, and register
  `disc:*`, `prev:*` and `test:*` fields; `toolClosed` unregisters them. A
  `set` of a `wxChoice` sends no event, so the hook clicks `prev:update`.
- A request line whose placeholder is empty is dropped by gensub, so a preview
  with Memory 0 has no `--mem` line, as a real job has none.
- HTCondor needs the run directory to exist and not be under /tmp, so the test
  uses the machine's home directory; the preview shows `/path/to/run`.
- Slurm's `sinfo` prints one row per node type of a partition; `parseSinfo`
  adds their processors and takes the largest node's memory.
- The stand-in `stubsched.py` is also installed as `sinfo`, `sbatch`, `qconf`,
  `bqueues` and `condor_status`; a queue named `nosuch` in the script is refused.
  Its PBS, LSF, SGE and HTCondor listings are formats from the vendors'
  documentation, not captured from live installations (Slurm's `sinfo` was
  checked against the real one on niobium).
