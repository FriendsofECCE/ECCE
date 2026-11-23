# Release round: workflow scripts W1-W6

`run_round.py` runs the six workflow scripts of a release round (wiki page
Release-9.6.0, "Workflow scripts") headlessly, each step through ECCE's own
test hooks (`ECCE_TEST_ORGANIZER` with `ECCE_TEST_ORGANIZER_ANSWERS`,
`ECCE_BUILDER_SCRIPT`, the Set-up tab's `setup ...`, `ecce-one-instance`).
Nothing is written into ECCE's data behind its back; the only files the
runner writes are the hook files and, for W6, the list of added servers.

    ECCE_TEST_HOME=<install> ECCE_TEST_WRAPPERS=<install>/../bin \
        python3 tests/releaseround/run_round.py [--only W1,W4] [--keep]

On an offload host: `tools/offload/run-offload.sh --host radium <sha>
apptest tests/releaseround/run_round.py`. ctest: `releaseround` (label
`long`, so `ctest -LE long` leaves it out).

It uses the tests/apps machinery: Xvfb, an isolated HOME
(`ECCE_REALUSERHOME`), private services, and one data folder per round
(`ECCE_LOCAL_DATA`). Each workflow starts a fresh Organizer in one-program
mode with the Normal layout and quits it. Output: one line per step,
`W1.3 PASS|FAIL|MANUAL|SKIP <evidence>`, then a summary; exit 1 on any
FAIL, 77 without an install or Xvfb. `--keep` keeps the work folder
(Organizer logs per workflow, `organizer.log`).

## How the steps map to hooks

| Step | Done with |
| --- | --- |
| W1.1 | `examples` (Getting Started), `newproject`, `server paste copy` (Copy and Paste), `duplicate URL` (New from This > Duplicate) |
| W1.2, W2.2-4, W3.1-2 | Builder script: `info`, `select_hetero`, `delkey`, `undo`, `redo`, `menu Build\|Add Hydrogen`, `ligands`, `ligand 1`, `pbc ...`, `save` |
| W1.3-5, W2.5-6, W3.3 | `start CalculationEditor`, `setup info\|runtype\|save\|launch\|launch go`, `state URL` polled |
| W1.6, W2.6, W3.4, W4.3-4 | `start CalculationViewer`, one Builder script `plist`, `list Calculation Summary`, `list Energies`, `overlay`, `selectoverlay 1`; `tabclose` once, checked |
| W1.7 | `menu exit`, a new Organizer, `state URL` |
| W2.1 | `$ECCE_HOME/libexec/ecce-one-instance FILE` (what `ecce FILE` and Open With do) |
| W4.2 | `select URL`, `desk`, `refreshtimes` (`ECCE_TIMING=1`) |
| W5 | `tabs`, `towindow`, `totabs`, `windowitems`, `windowmenu N`, `tabclose`, `menu exit`, then processes whose environment holds this round's `ECCE_LOCAL_DATA` |
| W6 | `tests/apps/fakessh/ssh` on PATH: `server connect`, `server newproject`, `server paste copy`, `server rootchoose ... Disconnect\|Connect` |

## Not scripted (reported MANUAL)

- W1.1: a pasted copy of an example keeps its Loaded (imported) state,
  whose Launch is off by design (as Reset for Rerun skips it); the runner
  then makes New from This > Duplicate of it, the route the help gives for
  rerunning, and W1.2-W1.7 use the duplicate.
- Every Builder and Viewer tab runs the script (`ECCE_BUILDER_SCRIPT_EACH=1`);
  without it only the first tab of the process does.
- W2.5: the Builder's Save As calculation dialog has no hook; the runner
  makes the Vina calculation with File > New (`newcalc`), imports the same
  PDB in its Builder, repeats the edits and saves.
- W3.2: the Periodic Builder's Centre lattice box has no hook.
- W4.2: the structure picture on the Builder button is timed, not judged.
- W5.2: dragging a tab out needs a real mouse (`towindow`/`totabs` is used).
- W5.3: the pointer moving over the Window menu's items (#1239) has no hook.
- W6.2: a Slurm submit needs a real cluster; W6 uses a copy of a finished
  calculation in an SSH folder reached through the fake ssh.

W1 launches ECCE-QM on `localhost` (this computer). W2.6 runs only when
`vina` is on PATH, the W3 launch only when `pw.x` is; otherwise they are
SKIP with the reason.

## Windows and macOS

Not run there yet. What would differ: the wrappers (`ECCE_TEST_WRAPPERS`)
are the install's `bin` (`ecce-organizer.cmd` on Windows, so the
Popen command needs the suffix); `ecce-one-instance` is in `libexec` of the
install on both; the one-instance run folder is `$TMPDIR/ecce-one` on
macOS; there is no Xvfb, so tests/apps' display handling must run on the
logged-in desktop session; there is no `/proc`, so W5.5's process check
reports SKIP; `killpg` does not exist on Windows; the fake ssh is a
`/bin/sh` script (Windows needs Git's sh or a real sshd).
