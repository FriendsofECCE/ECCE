# Run states

Every calculation has a run state. The Organizer shows it as a coloured
symbol in the **State** column and, by name, in the calculation's summary.
This page says what each state means and how a calculation gets into it.

When a calculation is **Incomplete**, **Monitor Error**, **System Error** or
**Waiting for Login**, selecting it shows the reason in the message area at
the bottom of the Organizer.

## Before the job

- **Created**: the calculation exists but has no input file yet.
- **Ready**: the input file has been made and the calculation can be
  launched. A Ready calculation whose launch failed shows the reason.

## While the job is on the machine

- **Submitted**: the job is in the queue of the compute machine and has not
  started. For a Slurm job, the message area (when the calculation is
  selected) and the summary's **Queue state** row say why it waits, in plain
  words, for example "waiting: no free resources yet; estimated start Fri 30
  Oct 14:20". A reason such as "the requested node type does not exist"
  means the job will not start until it is changed. The line disappears
  when the job starts.
- **Running**: the job runs, and ECCE follows its output.
- **Running?**: the job was running when ECCE last heard from it, and the
  connection to the machine is lost for the moment.
- **Waiting for Login**: ECCE stopped following the job (ECCE was closed,
  or the login to the machine lapsed). The job itself may still be running
  or may have finished. At the next login ECCE takes it up again; **Run
  Mgmt > Reconnect Selected...** does it at once. See
  [Reconnect Selected](reconnect-selected.md).

## After the job

- **Completed**: the code finished normally. The output and the results
  are stored with the calculation.
- **Imported**: the calculation was read from the output of a job that ran
  elsewhere, as are the example calculations. See
  [Importing a calculation](importing-a-calculation.md).
- **Killed**: the job was cancelled by a user, with **Run Mgmt > Terminate
  Calculation...** or with the queue system's own cancel command on the
  machine.
- **Incomplete**: the job ended without a complete result, and what it
  produced is kept: the output, the results read from it and the files
  copied back. The reason names the cause, for example:
  - the code stopped without converging;
  - the code ended with an error or a nonzero exit status;
  - the code exited normally but printed no normal-termination line;
  - the output file did not exist when the job ended;
  - the queue system ended the job: out of memory, wall time reached, node
    failure or preemption. The reason quotes the queue system's message
    from its error file (for Slurm, `slurm.err` in Outputs).

  An Incomplete calculation is not an ECCE or machine fault. Change the
  input (more memory, more time, a better starting geometry) and run again.
- **Monitor Error**: ECCE's own job monitoring stopped with an error that
  it could not recover from, for example when the results could not be
  stored. The job on the machine may have finished normally. **Run Mgmt >
  Reconnect Job Monitoring** reads its output again.
- **System Error**: the job disappeared from the machine without leaving an
  output file or a status file, and nobody cancelled it, or the compute
  machine reported an empty status (often a full disk). **Run Mgmt > Force
  State to Failed** also sets this state by hand.

## Changing a state by hand

**Run Mgmt > Reset State** sets an ended calculation to completed,
incomplete, imported or killed, for example after checking an output by
eye. **Run Mgmt > Reset to Rerun** returns an ended calculation to
**Ready** with the same input; **Run Mgmt > Reset for Restart** returns it to
**Ready** so that the input can be edited for a restart.

The dates in the summary are those of the job itself: **Start Date** when
the job script started on the machine, **Comp. Date** when it ended,
including after a later Reconnect.
