# tests/queues -- batch-queue support

```
tests/queues/run_tests.py                       everything (golden, stand-ins, real Slurm)
tests/queues/run_tests.py --suite local         golden + PBS/LSF/Moab stand-ins, no scheduler needed
tests/queues/run_tests.py --suite slurm         this machine's real Slurm
tests/queues/run_tests.py --manager pbs --code mopac --transport unset
tests/queues/run_tests.py --update-golden       rewrite tests/queues/golden/
tests/teaching/run_tests.py --queue slurm --case o2 --case h2se --case benzene
```

CTest: `queues` (local) and `queues_slurm` (SKIP 77 when `sinfo` fails or lists
no partition).  State and ports are the suite's own: `~/.cache/ecce-queue-state`,
data server 8696, broker 8688.

## What runs

* **golden**: `scripts/gensub` fed a parameter file for each manager (PBS, LSF,
  Moab, Slurm, Shell) and each of three settings profiles (full, multi-node,
  nothing set) and compared with `golden/<mgr>.<code>.<profile>.sh`, normalised for
  date, version and paths.  The directive lines are also checked against an
  independent list, and for Slurm the real `sbatch --test-only` must accept them.
  A port of gensub from csh to sh has to reproduce these files, or change them
  on purpose with `--update-golden`.
* **live**: MOPAC, NWChem and a long MOPAC job launched through the real `Launch`
  (`launchjob`) on a machine registered under the manager, with queue, nodes,
  processors, wall time, memory and account set as the launcher sets them.  Checks:
  the id ECCE parsed, the scheduler's own record of the job (partition, time
  limit, tasks, memory under Slurm), the submitted script's directives,
  `completed` with TE and GEOMTRACE in `Props/`, and for the long job
  `RunMgmt::terminate` (the Organizer's Kill) ending the scheduler job and
  leaving the calculation `killed`.  Every run is repeated for
  `ECCE_TRANSPORT` unset and `direct`.

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
