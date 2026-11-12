# Machines, codes and queues

Help pages: `machines`, `register-machines`, `allocations`, `running-codes-on-macos`. The scientist uses **Tools > Register Machines...** in the Organizer, or `ecce -machine` from a terminal. The window has these tabs:

- **Machine:** host, name, cores and nodes, user name, account, run and scratch directory, allocation profiles.
- **Connection:** shell, login script, how jobs reach the machine.
- **Codes:** for each code, its modules, program path, environment variables and command line.
- **Job script:** scheduler request lines, and commands run before and after the program.
- **Queues:** the queue manager and the queues with their limits.

Use **Find** and **Check** on the Codes tab to confirm program paths. Before the first real job, run **Preview job script...** and **Test submission...** (for Slurm this is `sbatch --test-only`, which submits nothing).

## Files (user layer, `~/.ECCE`)

A user setting **replaces** the site setting for that key; it is not added to it.

| File | Holds |
|---|---|
| `MyMachines` | one tab-separated line per machine: name, host, three `Unspecified` fields, `procs:nodes`, `ssh`, `:Code1:Code2:...`, and capabilities (`MN:RD:SD:UN:PW` plus `AA` allocation account, `Q` queues, `TL` time limit, `MM` memory limit) |
| `CONFIG.<name>` | `<Code>: /full/path`, `<Code>Modules: mod1 mod2`, `<Code>Environment { NAME value }`, `<Code>Command { ... }`, and the scheduler header block (`Slurm { #SBATCH ... }`) |
| `Queues` | `Queues:` list of machines with queues, and per machine `<name>\|queueMgrName: Slurm`, `<name>\|queueMgrVersion: 2.0~`, `<name>\|prefFile: <name>.Q` |
| `<name>.Q` | `Queues: q1 q2`, and per queue `minProcessors`, `maxProcessors`, `runLimit` (minutes), `memLimit` (MB), `scratchLimit`; 0 means no limit |
| `MachPrefs`, `MachProfiles` | the user's launch defaults and allocation profiles; written by the window, don't hand-edit |

The safest order: the scientist creates the machine in the window (**New Machine**: host, name, cores, user, account, run directory, **Save**). Then you fill in `CONFIG.<name>` and the queues, with ECCE closed or the window reopened afterwards. **Job script > Advanced: edit file...** opens `CONFIG.<name>` as text, and its **Check** button names keys ECCE doesn't read. A machine named in `Queues` that isn't registered stops ECCE from starting.

Placeholders in request lines and commands include `$nodes`, `$totalprocs`, `$ppn`, `$wallTime`, `$memory`/`$memoryM`, `$account`, `$queue` and `$scratchDir`. A request line whose placeholder is empty is left out.

## Lessons from real use

- **A `Slurm { }` block in `CONFIG.<name>` replaces ECCE's whole header.** A block with only one line produced jobs with no account, partition or time, and sbatch refused them ("You must supply an account"). Always write the complete header.
- **No `TL` capability** means `#SBATCH --time` is dropped and the job runs with no limit. **No memory line** means Slurm's default. On a shared workstation that can give a 1-core job the whole node's memory, so nothing else runs beside it. On a big node, a per-core default can exceed the node, and the job sits as `BadConstraints`. `squeue` also shows `BadConstraints` for harmless reasons, so check `sacct -j <id> -o ReqMem` first.
- **`ecce-launch on=<machine>`** on a calculation with no stored queue uses the machine's default queue. Make sure that isn't a short debug queue.
- **Kebnekaise (HPC2N):** only the `batch` partition accepts jobs, and the node type comes from `--constraint` (`zen4`: 2 × 128 cores; `skylake`: 28 cores). Use one ECCE machine entry per node type. `--mem-per-cpu=2500M` works on Zen4. The installed software is listed at https://docs.hpc2n.umu.se/software/apps/ and with `module spider`. Allocations are a rolling 30-day window.
- **CP2K versions:** an older CP2K on a cluster can lack basis sets or potentials that a newer local CP2K has. Check the data files of the version that will run. To test locally, download `data/` of that tag from github.com/cp2k/cp2k and set `CP2K_DATA_DIR`.
- **Local Slurm on Linux workstations** (single node, `select/cons_tres` with `CR_Core_Memory`, a `DefMemPerCPU`, cgroup enforcement) makes ECCE queue and limit jobs properly. Slurm doesn't support macOS. There, use `localhost` without a queue manager.
- **ssh:** ECCE logs in with its own ssh library and asks for passwords and codes in dialogs. It can store the password in the system keychain. A login it can't answer needs the "Don't submit; only make the files" setting (help page `machines`).
