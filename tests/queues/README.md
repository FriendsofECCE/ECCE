# tests/queues -- batch-queue support

```
tests/queues/run_tests.py                       everything (golden, stand-ins, real Slurm, SGE, HTCondor)
tests/queues/run_tests.py --suite local         golden + PBS/LSF/Moab stand-ins, no scheduler needed
tests/queues/run_tests.py --suite slurm         this machine's real Slurm (or: sge, htcondor)
tests/queues/run_tests.py --manager pbs --code mopac
tests/queues/run_tests.py --update-golden       rewrite tests/queues/golden/
tests/teaching/run_tests.py --queue slurm --case o2 --case h2se --case benzene
```

CTest: `queues` (local), `queues_slurm`, `queues_sge` and `queues_htcondor`; each
real one SKIPs (77) when its client commands are missing or no queue, partition or
pool answers.  State and ports are the run's own: a fresh `~/.cache/ecce-queue-<random>`
removed at exit, and OS-chosen data server and broker ports, so two runs
can go at once (see docs/claude/services).

## What runs

* **golden**: `scripts/gensub` fed a parameter file for each manager (PBS, LSF,
  Moab, Slurm, SGE, HTCondor, Shell) and each of three settings profiles (full, multi-node,
  nothing set) and compared with `golden/<mgr>.<code>.<profile>.sh`, normalised for
  date, version and paths.  The directive lines are also checked against an
  independent list, and the real parser of Slurm (`sbatch --test-only`), SGE
  (`qsub -verify`) and HTCondor (`condor_submit -dry-run`) must accept them.
  A port of gensub from csh to sh has to reproduce these files, or change them
  on purpose with `--update-golden`.
* **live**: MOPAC, NWChem and a long MOPAC job launched through the real `Launch`
  (`launchjob`) on a machine registered under the manager, with queue, nodes,
  processors, wall time, memory and account set as the launcher sets them.  Checks:
  the id ECCE parsed, the scheduler's own record of the job (partition, time
  limit, tasks, memory under Slurm), the submitted script's directives,
  `completed` with TE and GEOMTRACE in `Props/`, the real scheduler's own record
  of the job (Slurm `scontrol`, Grid Engine `qstat -j`/`qacct`, HTCondor's job ad:
  queue, time limit, cpus, memory, account, run directory) and for the long job
  `RunMgmt::terminate` (the Organizer's Kill) ending the scheduler job and
  leaving the calculation `killed`.

## The stand-in schedulers are not PBS, LSF or Moab

`stubsched.py` is installed as qsub/qdel/qstat, bsub/bkill/bjobs and
msub/mjobctl/checkjob.  The machines' `qmgrPath` (the "queue manager path" of
Machine Registration) puts its directory first, because this host also has Grid
Engine's own `qsub`.  Each records the script it was given, runs it detached
with the directives ignored, prints a job id and answers status queries.
Formats come from the vendors' documentation; none of the three is installed
here, so they are unchecked against a live installation:

| manager | submit prints | ECCE's parse (siteconfig/QueueManagers) |
|---|---|---|
| PBS | `12345.stubserver` (PBS Pro and Torque: `<seq>.<server>`) | `.*` |
| LSF | `Job <12345> is submitted to queue <normal>.` (`... to default queue <normal>.` without `-q`) | `Job <` `.*` `> is submitted` |
| Moab | a blank line, `12345`, a newline (`Moab.12345` over Torque: `STUB_MOAB_PREFIX=1`) | `[0-9]+` |

The test output says "STAND-IN" for these rows.

## Faults of ECCE that the suite reports and does not fail on

`TaskJob::getDataFile(PRIMARY_OUTPUT)` on a calculation that has not run yet
sometimes returns `Outputs` (the collection) instead of the declared output name
(`mopac.mopout`), so the job writes one file and the monitor reads another, and
MOPAC loses TE and GEOMTRACE.  It comes from c610bf6c's fallback for imported
calculations, is not deterministic, and is listed under KNOWN ECCE FAULTS with
every run; `--strict` counts it as a failure.  The TE/GEOMTRACE check of an
affected MOPAC job is skipped, so that a queue fault is not blamed on it.
