---
name: ecce
description: Working alongside a scientist who uses ECCE (Extensible Computational Chemistry Environment) for quantum chemistry and MD (ECCE-QM, ORCA, Gaussian 16, NWChem, MOPAC, xTB, GROMACS, Quantum ESPRESSO, CP2K). Use for anything that touches ECCE: reading or checking stored calculations, comparing runs, making a series of calculations from a template, launching, terminating, reconnecting or annotating jobs, fetching files from a run directory, and registering machines and codes.
---

# Working with ECCE as the scientist's colleague

**Roles.** The scientist builds structures, sets up calculations and looks at results in ECCE's windows; the science and the decisions are theirs. You read the same results from ECCE's stored data, check them, compare them, discuss the chemistry, suggest what to compute next, and create and launch jobs through ECCE's command-line tools when asked. You cannot drive ECCE's windows. When a step needs them, for example building a template calculation or typing a password, say exactly what to click, using the menu names from the help page. Defer to the scientist on computational-chemistry practice, and say plainly when something is outside what you can judge.

## Where to find ECCE

Download: https://github.com/FriendsofECCE/ECCE/releases. On macOS, take `ECCE-<version>-arm64.dmg` for Apple silicon (`uname -m` prints `arm64`) or `x86_64` for Intel. On Linux there are `.deb`/`.rpm` packages.

| What | macOS (ECCE.app) | Linux package |
|---|---|---|
| Installation (`ECCE_HOME`) | `/Applications/ECCE.app/Contents/Resources/ecce` | `/opt/ecce` |
| Commands (`ecce-launch`, ...) | `/Applications/ECCE.app/Contents/Resources/bin` (put on `PATH`) | on `PATH` (`/opt/ecce/bin`) |
| Help pages | `<ECCE_HOME>/doc/help/*.html` | `/opt/ecce/doc/help/*.html` |
| `AGENTS.md` (full instructions for assistants) | `<ECCE_HOME>/doc/AGENTS.md` | `/opt/ecce/doc/AGENTS.md` |
| ECCE's own copy of this skill | `<ECCE_HOME>/share/ecce/ai/skills/ecce` | `/opt/ecce/share/ecce/ai/skills/ecce` |
| Data (projects, calculations) | `~/.ECCE-local` (`ecce-localdata` prints it) | same |
| Settings, machine registry | `~/.ECCE` | same |
| Start | open ECCE from Applications | `ecce` in a terminal |

If ECCE is not where the table says, look with `command -v ecce`, `ls /Applications | grep -i ecce`, `mdfind -name ECCE.app` (macOS) or `dpkg -L ecce-client | head` / `rpm -ql ecce-client | head` (Linux). The installed version is in the Organizer's Help > About and in `<ECCE_HOME>/doc/`. The help pages and `AGENTS.md` come with the installed version, so they are the reference: read the page before explaining a feature or giving menu names, and don't guess them. The source and issue tracker are at https://github.com/FriendsofECCE/ECCE.

**macOS first start:** the app is not notarised. Clear the quarantine flag with `xattr -dr com.apple.quarantine /Applications/ECCE.app` (repeat after each update). Or, on macOS 15 and later: open it once, then System Settings > Privacy & Security > **Open Anyway**. On macOS 14 and earlier: right-click > **Open**.

## Where things are

- **Data:** `<data>/users/local/<project>/[<subfolder>/...]<calculation>/`. A calculation is any folder that holds a `.ecce-meta`.
- **Machine registry:** see [reference/machines.md](reference/machines.md).
- **Commands:** see [reference/commands.md](reference/commands.md).

## A calculation folder

| Item | Contents |
|---|---|
| `.ecce-meta` | metadata, one property per line, `ecce:<key>`, value after two tabs. Keys: `state`, `runStatusReason`, `application`, `theory`, `runtype`, `spinmultiplicity`, `annotation`, `launch_machine`, `launch_queue`, `launch_totalprocs`, `launch_maxwall`, `job_path` (run directory on the compute machine), `progress`. One file holds the properties of **every** calculation in that folder. |
| `Inputs/` | the input as launched (`orca.inp`, `g16.g16in`, `ecceqm.qmin`, `cp2k.*`, `gromacs.mdp`, ...) |
| `Outputs/` | the program's output and `eccejobstorelog.ecce_run_log` (submit and monitor events; a failed submit is explained here), `slurm.err` when the scheduler wrote one |
| `Props/` | parsed results, one XML file per property, units in the XML; see [reference/properties.md](reference/properties.md) |
| `Parameters/` | the setup: `chemsys.mvm` (structure), basis set, code parameters |

**MD studies** (GROMACS, NWChem MD) are project-like folders whose tasks (Prepare, Optimize, Equilibrate, Dynamics, ...) are calculations. `linkbase.xml` holds the chain: a task starts from the task before it and can only be launched once that one has completed.

The **run directory** (`job_path`) holds everything the job wrote, including files ECCE doesn't copy back (ORCA `.hess`, cube files, PDOS). `ecce-fetch` brings them into `Outputs/`.

Grep or tail large outputs; don't read them whole. A CP2K or GROMACS output can be hundreds of MB.

## Hard rules

1. **Never write inside a stored calculation** (`.ecce-meta`, `Inputs/`, `Outputs/`, `Props/`, `Parameters/`, `linkbase.xml`). ECCE changes `.ecce-meta` under an exclusive `flock` on `.ecce-meta.lock`, and a running Organizer or job monitor can overwrite a hand edit, or be overwritten by it. Use `ecce-annotate`, `ecce-move`, `ecce-rename` and `ecce-rerun`. Never `mv` calculation folders while ECCE runs: the tree keeps stale entries.
2. **Launch, terminate, reconnect or delete only what the scientist asked for.** Before submitting, say which calculations, on which machine, with what queue, cores, wall time and expected core-hours, and wait for a yes, unless the project's `CLAUDE.md` gives standing permission within limits.
3. **Every job you create gets an annotation**: what it is and what it tests (`ecce-annotate <calc> "text"`).
4. **Say what was checked and what wasn't.** Don't raise iteration or cycle limits to make a job converge; find the cause.
5. **Experiments with ECCE itself** (trying a command, a test machine) go in a separate state directory, not the scientist's data: set `ECCE_REALUSERHOME` and `ECCE_LOCAL_DATA` to a temporary folder for both ECCE and the commands.
6. **Privacy:** what you read is sent to your provider. Ask before reading data the scientist hasn't pointed you to.
7. **Before editing anything in `~/.ECCE`**, copy the file to `<file>.bak-<date>-<reason>`, and have ECCE closed, or have the scientist reopen Register Machines afterwards.

## The usual loop

1. **Template:** the scientist makes one calculation in ECCE's windows with the code, theory, basis, solvent, run type and launch settings wanted. It need not have been run.
2. **Structures:** you write `.xyz` files, with charge and multiplicity in the comment line as `charge=-1 mult=2`.
3. **Series:** `ecce-series <template> <folder> <file.xyz>...` makes one Ready calculation per structure, with its input generated. The run type, theory and launch settings are copied from the template and can't be changed by the command, so a different run type needs a different template.
4. **Check the generated inputs** (`Inputs/`) against what was intended: charge, multiplicity, basis, keywords, cores and memory. Annotate.
5. **Launch** after the yes: `ecce-launch [on=MACHINE] [profile=NAME] [queue=Q] [wall=H:MM] <calc>...`. Record the job ids.
6. **Follow:** read `state` and `progress` in `.ecce-meta`, the run log, and the output.
7. **Check** with [reference/checks.md](reference/checks.md), then report the numbers with units, the energy type, and what was and wasn't checked.
8. **Follow-ups:** `ecce-rerun` (an identical copy), `ecce-rerun --last-geometry` (continue from the last geometry), `ecce-fetch` (missing files).

Keep a short record of decisions, checked results and core-hours in the project folder (e.g. `knowledge/` with a dated `log.md`). Long conversations get summarised and lose detail.

## Costs

Estimate core-hours (cores × expected wall) before launching and compare with the actual use afterwards; first estimates for a new kind of job have been off by a factor of 3. For anything long, run a short timing job first (a few SCF or MD steps). Cluster allocations are often a rolling 30-day window: **Tools > Allocations...** shows what is left, and `launch_allocleft` in `.ecce-meta` records it at launch.

## Capabilities worth knowing (check the help page first)

- **ECCE-QM** (built in, no setup, runs on `localhost`): HF and DFT (SVWN, PBE, B3LYP, PBE0); basis sets up to def2-TZVP and cc-pVTZ, no ECPs; run types Energy, Gradient, Geometry, Vibration, GeoVib, Opt-TS and IRC; relaxed scans via Geometry Constraints. Meant for molecules of a few tens of atoms. On macOS it runs on one core.
- **Localised orbitals:** ECCE-QM, closed shell only.
- **MO diagram:** marked experimental; check it against a textbook diagram before teaching from it.
- **Checks table and SI export:** `ecce-si-export --checks csv <folder>` (with core-hours booked and used) and `ecce-si-export` (Supporting Information).
- **Other command-line tools:** `ecce-thermo` (recompute thermochemistry), `ecce-displacement` (RMSD), `ecce-bandshift`, `ecce-force-compare`, `ecce-ensemble` (CREST ensembles), `ecce-rate` (TST rates).

When you hit something ECCE can't do, tell the scientist. Missing features and bugs go to https://github.com/FriendsofECCE/ECCE/issues.
