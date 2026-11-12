# ECCE for scripts and AI assistants

Much of what ECCE does can be driven from the command line, so scripts and AI
assistants can work with it too. This file says what ECCE is and can do, how
to do the usual tasks, how to set up compute machines and codes, how ECCE
stores its data, which commands there are, and how to check a result. It is read by people,
scripts and coding assistants alike. The scientist stays in control: the
assistant prepares, checks and explains, and acts on ECCE only when asked to.
Nothing here says that an assistant gets the chemistry right; the results are
the scientist's to judge.

Privacy: what an assistant reads (inputs, outputs, structures, this file) is
sent to the provider of that assistant. Do not point an assistant at data that
may not leave the machine.

## Contents

1. Safety rules
2. What ECCE is
3. What ECCE can do (codes, run types, help pages)
4. How to do things (task recipes)
5. Compute machines and HPC
6. Registering a new code (a developer task)
7. Data layout
8. Commands
9. Checking a result
10. Documentation shipped with ECCE
11. Keeping this file current

## Safety rules

1. Never edit a stored calculation by hand (`.ecce-meta`, `Inputs/`,
   `Outputs/`, `Props/`, `Parameters/`, `linkbase.xml`). ECCE keeps the files
   consistent with each other; a hand edit makes the calculation look
   finished, failed or different from what ran. To change a calculation, use
   ECCE, or make a new one.
2. Launch, terminate, reconnect or delete only what the user asked for, and
   only the calculations the user named.
3. Before submitting, say what will be submitted: which calculations, on which
   machine, with which queue, cores and wall time. Wait for the user's yes.
4. For experiments, use a separate state directory so the user's own data and
   settings are not touched (see "Data layout", "A separate state directory").
5. Say plainly what was checked and what was not. Do not raise optimisation
   cycle limits to make a calculation converge; find the cause.

## What ECCE is

ECCE (Extensible Computational Chemistry Environment) is a desktop
application for setting up, running and analysing computational chemistry
calculations. It builds the input for a code, submits the job to the
computer it runs on, follows it, parses the output, and stores everything
with the calculation. It does not contain most of the codes: ORCA, Gaussian,
NWChem, Quantum ESPRESSO and the others are installed on the machine where
they run. ECCE-QM, a small Hartree-Fock and DFT program, is built in.

Your user works in ECCE's windows. You work on the same stored data and
through the command-line tools in "Commands". You cannot click in the windows.

### The programs

| Program | What it is | How it starts |
|---|---|---|
| Organizer | The main window: the tree of projects and calculations, the project table, the Run Mgmt menu (launch, terminate, reconnect, re-parse) and the Tools menu. | `ecce` (it opens the Organizer directly) |
| Builder | Builds and edits a structure; the Viewer panels show results (orbitals, spectra, trajectories, properties). | From a calculation in the Organizer |
| Calculation Editor | Sets up the calculation: theory, basis set, run type, details; writes and checks the input. | From a calculation in the Organizer |
| Launcher | Chooses machine, queue, cores, wall time and allocation profile, and submits. | Opened from the Organizer for a calculation |
| Register Machines | Registers compute machines: codes, job script, queues. | **Tools > Register Machines...**, or `ecce -machine` |
| Machine Browser | Runs a few status queries on a registered machine over ECCE's login. | From the Organizer or Launcher |
| Basis Set Tool | Looks at and edits basis sets. | From the Organizer or Calculation Editor |

The commands (`ecce-launch`, `ecce-series`, ...) are separate and are the
way an assistant acts on ECCE. The help pages (**Help > ECCE Help**, `help/src/*.md`)
describe every window; `help/src/index.md` lists them.

### The life of a calculation

A calculation has a run state (help page `run-states`):

1. **Created**: it exists but has no input file.
2. **Ready**: the input file exists and it can be launched. A failed launch
   leaves it Ready with the reason in the run log.
3. **Submitted**: the job is in the queue of the compute machine.
4. **Running**: the job runs and ECCE follows its output. **Running?** and
   **Waiting for Login** mean ECCE has lost the connection to the job, which
   may still be running; Run Mgmt > Reconnect Selected (or `ecce-reconnect`)
   takes it up again.
5. **Completed** (the code finished normally, not necessarily with a right
   result), **Imported** (read from an output made elsewhere), **Incomplete**
   (ended without a complete result; the reason names the cause),
   **Killed** (cancelled by a user), **Monitor Error**, **System Error**.

In `.ecce-meta` the state is `state`; the reason for an ended job is
`runStatusReason`.

### Projects, studies and workflows

- A **project** is a folder. Calculations live in a project, optionally in
  subfolders. File > New > Project... makes one.
- A **study** is a project-like folder whose members are tasks with
  dependencies: GROMACS and NWChem MD studies (Prepare, Optimize, Equilibrate,
  Dynamics); a task starts from the one before it and is launched after that
  one has completed.
- A **workflow** draws calculations that start from the geometry of an earlier
  one as a tree, with **Launch Ready Steps** (help page `workflows`). A
  **recipe** is a workflow template for a kind of study (`recipes`,
  `writing-a-recipe`).

### Where the data is, and deployment modes

- **Local data**: the folder printed by `ecce-localdata`
  (`~/.ECCE-local`). Settings and the machine registry are in `~/.ECCE`.
- **Data server**: calculations stored on a server that several users reach
  (`ecce -remote`); see "Data server" below.

There are three deployment modes (GETTING_STARTED.md, "Deployment modes"),
decided by the site's configuration in this order: a shared system broker
(`siteconfig/SharedBroker`), then `ecce -remote`, otherwise local.

1. **Local**: one user, one broker in `~/.ECCE`, no network port.
2. **Central server**: a data server and broker on one machine; users connect
   as clients with `ecce -remote` (the teaching set-up, also the original
   PNNL set-up).
3. **Shared broker**: one system service (`ecce-broker.service`) on an
   application server.

The mode changes who may connect, not how you use the commands.

## What ECCE can do

The codes ECCE knows are the `.edml` files in `data/client/cap/` of the
source (`data/client/config/ResourceDescriptor.xml` lists which calculation
types a project may hold). Run types come from `scripts/codereg/<code>runtype.py`; the Calculation
Editor's Run Type list is the authority for what one version offers. Codes
marked (experimental) are newer and less tested.

| Code | Run types (as offered) | Typical use | Help page |
|---|---|---|---|
| ECCE-QM (built in, `localhost`, no setup) | Energy, Gradient, Geometry, Vibration, GeoVib, Opt-TS, IRC; relaxed scans through Geometry Constraints | HF and DFT (SVWN, PBE, B3LYP, PBE0) on molecules of a few tens of atoms; localised orbitals, MO diagram | `ecce-qm`, `ecce-qm-files` |
| ORCA | Energy, Geometry (minimum or TS), Vibration, GeoVib, IRC | DFT, composite methods, post-HF | `orca-composite-methods`, `running-codes-on-linux` |
| Gaussian 16 | Energy, Gradient, Geometry (minimum or TS), Vibration, GeoVib | Molecular QM; licensed, installed by the site | `running-codes-on-linux` |
| NWChem | Energy, Geometry, GeoVib, Property; plane-wave PSPW and BAND | Molecular QM; MD through NWChemMD studies | `first-calculation` |
| MOPAC | Energy, Geometry, GeoVib, Vibration | Semi-empirical | `running-codes-on-linux` |
| xTB | Energy, Geometry, GeoVib, Molecular Dynamics, Conformer Search, Docking | GFN2-xTB, GFN1-xTB, GFN-FF | `xtb` |
| CREST | Conformer Search, Solvation Shell, Protonation Sites, Tautomers | Conformer ensembles (needs `xtb`) | `crest` |
| Quantum ESPRESSO (experimental) | Energy, Geometry, vc-relax, Dynamics | Periodic plane-wave DFT | `qe-first-calculation`, `qe-choosing-settings` |
| CP2K (experimental) | Energy, Geometry | Periodic and large DFT | `cp2k-scf`, `cp2k-basis-potential` |
| GROMACS (experimental) | Prepare, Minimisation, Equilibrate, Dynamics (a study); MD Analysis, umbrella sampling | Biomolecular and condensed-phase MD | `gromacs-first-calculation`, `gromacs-prepare`, `gromacs-md-settings`, `md-analysis`, `umbrella-sampling` |
| MLP, MACE (experimental) | Geometry, Vibration, Molecular Dynamics | Screening and relaxation of periodic cells | `mlp` |
| Vina | Receptor and Ligand docking | AutoDock Vina poses and scores | `vina` |

`data/client/cap/` also holds older codes (Gaussian 98, 03 and 09, GAMESS-UK,
MOLCAS, NWChemMD, MetaDyn); do not expect new features for them.

Property views (which panel shows what) depend on what is stored in `Props/`;
see "Properties". Analysis views: the MO diagram (experimental; check it
against a textbook diagram), Density Map, ELF/LOL/NCI, localised orbitals,
density of states and OPDOS, dipole moment, Compare Energies, Energy Calculator,
thermochemistry, spectra and band matching, MD speeds and diffusion, MD
Analysis. Each has a help page named in `help/src/index.md`.

## How to do things

For each task: what the user does in the GUI, and what you run.

**Build or import a structure.** User: the Builder, a structure file or the Structure Library (`importing-a-structure`); a calculation
from a finished output: **File > Import Calculation from Output File...**
(`importing-a-calculation`). You: write `.xyz` files with charge and
multiplicity in the comment line as `charge=-1 mult=2`.

**Set up one calculation.** User: select a project, File > New, choose the
code, then Builder (structure) and Calculation Editor (theory, basis, run
type, Theory Details); the first calculation is `first-calculation`. You
cannot do this step; ask the user to make a template calculation. The input
is generated when the calculation reaches Ready; check it in `Inputs/`.

**Launch.** User: Run Mgmt > Launch Selected (`launch-selected`) or the
Launcher. You: after the user's yes, `ecce-launch [on=MACHINE] [profile=NAME]
[queue=QUEUE] [wall=H:MM] <calculation>...`.

**A series or a batch.** User: New Series (`new-series`) for a list of
structures; Tools > Batch Add from SMILES (`batch-from-smiles`). You:
`ecce-series <template> <folder> <file.xyz>...` makes one Ready calculation
per structure with the template's setup; `ecce-batch-smiles` does the
SMILES table; then `ecce-launch` the Ready calculations. A different run
type needs a different template.

**Check a result.** See "Checking a result". Read `.ecce-meta`, the run log,
the output, `Props/`.

**Fetch files** the run directory holds and ECCE did not copy back (ORCA
`.hess`, cube files, PDOS): `ecce-fetch`.

**Rerun.** User: a calculation's right-click menu, then Launch.
You: `ecce-rerun <calc>` (an identical copy) or `ecce-rerun --last-geometry`
(continue from the last geometry).

**Terminate.** User: Run Mgmt > Terminate Calculation... or Terminate
Selected.... You: `ecce-terminate <calc>`. A user cancel gives Killed.

**Reconnect** (state Waiting for Login, Running?, or after a restart): Run
Mgmt > Reconnect Selected..., or `ecce-reconnect <calc>`.

**Re-parse outputs.** After a parser fix, or when fewer properties show
than the output contains: **Run Mgmt > Re-Parse Outputs** (`reparse-outputs`),
which needs no login. There is no command for it in this version
(`src/apps/launch/` has no `ecce-reparse`); ask the user to run it.

**Workflows and recipes.** User: build a workflow from calculations
(`workflows`), or Apply Recipe (`recipes`). Launch Ready Steps starts what
can start. `ecce-workflow` is the command.

**QSAR.** Tools > QSAR Table and QSAR Analysis (`qsar-table`,
`qsar-analysis`); commands `ecce-qsar table` and `ecce-qsar pca`.

**Docking.** Vina calculations (`vina`); `ecce-pdbqt` prepares PDBQT files;
CREST/xTB docking is a run type of xTB.
The Builder Poses panel shows cached ligand-only thumbnails for docking.
For Vina, the line above the table reports the retained count, energy range
and maximum modes from the stored input deck (settings fallback).
Builder script `poses note` returns that text; `poses thumb 1 <file.png>`
saves the cached thumbnail of pose 1 for image checks.

**Diffraction.** Tools > Diffraction (`fibre-diffraction`); `ecce-xrd`.

**Other analysis from the command line** (thermochemistry, RMSD, bands,
MD analysis, orbital composition, Morse fits): see "Other command-line tools".

## Compute machines and HPC

A calculation runs on a **machine** registered in ECCE. `localhost` is
registered for the user. A cluster is one more entry. **A machine
registered in ECCE is the way to reach an HPC cluster: ECCE logs in with its
own ssh library, creates the run directory, copies the files, submits to
Slurm (or PBS, LSF, Grid Engine, HTCondor), follows the job and copies the
output back.** If a user needs a cluster, first check whether it is
registered (Machine Browser, or `~/.ECCE/MyMachines` and the site machine
list) before looking for another way in.

### Registering a machine (user, in the window)

Help pages: `machines`, `register-machines`, `allocations`,
`running-codes-on-macos`. **Tools > Register Machines...** in the Organizer
(or **Job > Register Machines...** in the Launcher, or `ecce -machine`).
Tabs:

- **Machine:** host (fully qualified), name, cores and nodes, user name,
  account, run directory, scratch directory, **Allocation profiles** (one per
  allocation; each holds account, run directory, scratch directory and
  default queue; #541).
- **Connection:** shell, login script, **Allocation command**, directory of
  `sbatch`/`squeue`, **How jobs reach this machine** (Advanced).
- **Codes:** for each code, **Modules**, **Program** (full path), **Environment
  variables**, **Command line**; **Find** and **Check** confirm the path.
- **Job script:** the scheduler request lines and commands before and after
  the program; **Preview job script...**, **Advanced: edit file...**.
- **Queues:** **Queue manager**, **Allocation accounts used**, queues with
  limits, **Discover queues...** (Slurm: `sinfo`), **Test submission...**
  (Slurm: `sbatch --test-only`, which submits nothing).

Steps for a cluster: New Machine; host, name, processors, nodes; Codes tab
program paths; Queues tab; Save. Before the first real job run **Preview job
script...** and **Test submission...**. Allocation accounts and per-allocation
run and scratch directories are profiles; once a machine has more than one,
the Launcher shows a **Profile:** choice, and a calculation records
`launch_profile` and `launch_account` in `.ecce-meta`. **Tools >
Allocations...** shows what is left.

### How ECCE logs in

ECCE uses its own ssh library and keeps a master connection. It asks for the
password, one-time code or unknown host key in dialogs. **You cannot type into
those dialogs; ask the user to** (and to tick **Remember on this computer**
if a keyring is available; a remembered password is never offered for jump
hosts or one-time codes). A machine reached through a port-forward must not be
registered as `localhost`: give it an alias in `~/.ssh/config` and register
the alias (#144).

Machines ECCE cannot log in to (two-factor): `noRemoteAccess: true` in
`CONFIG.<machine>` or the `dummy` machine: ECCE writes the input and job script
locally and stops; the user copies them over, submits, and imports the output
with **File > Import Calculation from Output File...**. `userSubmit: true`
still transfers files over ssh and leaves only the submit command to the user.

### What an agent can do on the cluster

Through ECCE's commands, only: `ecce-launch` (submit), `ecce-terminate`,
`ecce-reconnect`, `ecce-fetch` (bring files from the run directory, `job_path`
in `.ecce-meta`, into `Outputs/`), plus `ecce-diagnose` for a problem report.
**There is currently no documented command to put a file on a machine or to
run a command there (issue #1002).** The Machine Browser buttons (Machine,
Disk Usage, Process Status, Allocation) run fixed queries, started by the
user. So when something has to be installed or placed on the cluster (a data
directory, a pseudopotential set, a basis library), tell the user exactly
what and where, and ask them to do it, or to allow you to use their own `ssh`.
Do not use credentials you were not given and do not look for another way in.

### Setting up a code on a machine

In the machine's **Codes** tab (stored in `~/.ECCE/CONFIG.<name>`):

- **Program:** the full path of the executable on that machine. A job without
  one stops with "Path for <code> not found". ORCA needs the full path
  (it finds its helper programs next to itself, and needs the Open MPI named
  in its archive name to run in parallel); on Debian the package `orca` is the
  GNOME screen reader, not ORCA.
- **Modules:** `module load` names for that code only; machine-wide ones go
  on the Job script tab.
- **Environment variables:** `NAME value` per line; names containing `PATH`
  are appended to, others replace. Stored as `<Code>Environment { }`.
- Gaussian 16: `g16root` and a scratch directory (`GAUSS_SCRDIR`); ECCE sets
  `GAUSS_EXEDIR` and `LD_LIBRARY_PATH` itself.
- **Quantum ESPRESSO: the pseudopotential files (UPF) must exist on the
  cluster.** ECCE picks one file per element from the pseudopotential
  directory, which is a field in the calculation's **Theory Details**
  (`ES.Theory.PW.PseudoDir`, default `/usr/share/espresso/pseudo`) and is
  scanned on the computer where ECCE runs, so the same set must be there for
  the input to be generated. On a cluster that directory is elsewhere: set the
  variable **`ESPRESSO_PSEUDO`** in the Quantum ESPRESSO **Environment
  variables** box (`QuantumESPRESSOEnvironment` block) to the cluster's
  directory. At job time it replaces `pseudo_dir` in the input. If that
  directory does not hold the files for the elements used, `pw.x` fails.
  Fill it, or have the user fill it, before the first job (for example with
  the SSSP set; Debian ships it as `quantum-espresso-data-sssp`).
- **ORCA:** program path as above; for NBO add `NBOEXE <path>` to its
  environment.
- **GROMACS:** `gmx` must be on the path of the shell that runs the job
  (module or **Program**). A Prepare task also runs `pdb2gmx` and needs its
  force-field data from the same installation.
- **CP2K:** the version on the cluster must contain the basis sets and
  potentials the input names; an older CP2K can lack ones a newer local CP2K
  has. Check the data files of the version that will run. Locally, the
  data directory is `CP2K_DATA_DIR`.
- **MLP:** the Program is a Python from an environment with PyTorch, ASE and
  MACE; **Set up...** next to Program builds one (`ecce-mlp-setup`).
- **xTB, CREST, MOPAC, NWChem:** program path; CREST runs `xtb` itself, so
  both must be installed.

### Checking a machine before launching

In Register Machines: **Find** and **Check** on each code, **Preview job
script...**, **Test submission...**. In the Machine Browser: the Machine
button (Slurm `sinfo`), Disk Usage, Process Status (the queue), Allocation.
From a terminal: `ecce-diagnose [-o directory] [run-directory]` collects ECCE's
version, logs and configuration into one text file; it changes nothing and
collects no passwords, but names hosts and users, so the user reads it first.

### Lessons from real use

- **A `Slurm { }` block in `CONFIG.<name>` replaces ECCE's whole header.** A
  block with one line gave jobs with no account, partition or time, and sbatch
  refused them ("You must supply an account"). Write the complete header.
  **Copy site setting** on the Job script tab keeps the site's lines.
- **No time-limit capability (`TL`)** means `#SBATCH --time` is dropped and the
  job runs with no limit. **No memory line** means Slurm's default; on a shared
  workstation that can give a 1-core job the whole node's memory, and on a big
  node a per-core default can exceed the node so the job sits as
  `BadConstraints`. `squeue` shows `BadConstraints` for harmless reasons too:
  check `sacct -j <id> -o ReqMem` first.
- **`ecce-launch on=<machine>`** on a calculation with no stored queue uses the
  machine's default queue; make sure that is not a short debug queue.
- **Kebnekaise (HPC2N):** only the `batch` partition accepts jobs and the node
  type comes from `--constraint` (`zen4`: 2 x 128 cores; `skylake`: 28 cores).
  Use one ECCE machine entry per node type (queue **Request lines** can hold
  `#SBATCH --constraint=zen4`). `--mem-per-cpu=2500M` works on Zen4. Software is
  listed at https://docs.hpc2n.umu.se/software/apps/ and by `module spider`.
  Allocations are a rolling 30-day window.
- **Local Slurm on a Linux workstation** (single node, `select/cons_tres` with
  `CR_Core_Memory`, a `DefMemPerCPU`, cgroup enforcement) makes ECCE queue and
  limit jobs properly. Slurm does not support macOS: use `localhost` with no
  queue manager.

### The settings files, for reference (user layer, `~/.ECCE`)

A user setting **replaces** the site setting for that key; it is not added.

| File | Holds |
|---|---|
| `MyMachines` | one tab-separated line per machine: name, host, three `Unspecified` fields, `procs:nodes`, `ssh`, `:Code1:Code2:...`, and capabilities (`MN:RD:SD:UN:PW` plus `AA` allocation account, `Q` queues, `TL` time limit, `MM` memory limit) |
| `CONFIG.<name>` | `<Code>: /full/path`, `<Code>Modules: mod1 mod2`, `<Code>Environment { NAME value }`, `<Code>Command { ... }`, and the scheduler header block (`Slurm { #SBATCH ... }`) |
| `Queues` | `Queues:` list of machines with queues, and per machine `<name>\|queueMgrName: Slurm`, `<name>\|queueMgrVersion: 2.0~`, `<name>\|prefFile: <name>.Q` |
| `<name>.Q` | `Queues: q1 q2`, and per queue `minProcessors`, `maxProcessors`, `runLimit` (minutes), `memLimit` (MB), `scratchLimit`; 0 means no limit |
| `MachPrefs`, `MachProfiles` | the user's launch defaults and allocation profiles; written by the window, do not hand-edit |

The safest order: the user creates the machine in the window (New Machine, host,
name, cores, user, account, run directory, Save); then you fill in
`CONFIG.<name>` and the queues, with ECCE closed or the window reopened
afterwards. **Job script > Advanced: edit file...** opens `CONFIG.<name>` as
text and its **Check** names keys ECCE does not read. Copy a file to
`<file>.bak-<date>-<reason>` before editing it. A machine named in `Queues`
that is not registered stops ECCE from starting.

Placeholders in request lines and commands: `$nodes`, `$totalprocs`, `$ppn`,
`$wallTime`, `$memory`/`$memoryM`, `$account`, `$queue`, `$scratchDir`. A request
line whose placeholder is empty is left out. Placeholders do not work in
environment variables.

## Registering a new code (a developer task)

Adding a code to ECCE is a change to the source tree, not a setting. Treat it
as development: work in a clone of the repository, read the checklist first,
and test with an isolated state directory. The pieces:

1. **`data/client/cap/<Code>.edml`**: the code's capabilities (theories, run
   types, basis, properties, which tools handle it) and its importer.
2. **`data/client/config/ResourceDescriptor.xml`**: a `<code>_es` resource type
   and a `ContainsResource` entry under `project`.
3. **`scripts/codereg/`**: the Calculation Editor's dialogs, for example
   `<code>theory.py` and `<code>runtype.py`, written in the dialog grammar.
4. **`scripts/parsers/ai.<code>`**: the input generator, filling the deck
   (and `<code>.launchpp` for launch-time changes; `scripts/gensub` writes the
   job script section for the code).
5. **`scripts/parsers/<Code>.desc`** and the parser scripts it names: what to
   extract from the output into `Props/`; **`<Code>.expt`** imports an output
   made elsewhere.
6. The monitor (`scripts/eccejobmonitor`), property panels
   (`data/client/config/PropertyPanelDescriptor.xml`, `properties`), the
   input checker (`scripts/parsers/verifyinput`), help page and this file.

Read `docs/claude/codereg/index.md`, in particular "New-code checklist"
(found integrating ORCA, #38) and the MOPAC additions (#86), and follow the
entries that apply to each file you change (`grep -rl '<file>' docs/claude`).
The wiki page "Where to find things in the code"
(github.com/FriendsofECCE/ECCE/wiki) is the longer orientation. Memory is
entered in GB for every code and converted in `ai.<code>` or its template.

## Data layout

### Local data

With local data (the default of a stand-alone install), projects and
calculations are ordinary folders. `ecce-localdata` prints the data folder in
use (its default is `~/.ECCE-local`). Below it:

    <data folder>/users/local/<project>/<calculation>/

A project is a folder and may hold projects. A calculation is a folder that
contains `.ecce-meta`.

### Data server

With a central data server (`ecce -remote`, `ECCE_REMOTE_SERVER`), the server
keeps the same layout under its own root and ECCE reaches it over HTTP
(WebDAV). A calculation is then named by a URL, not a path. Give the URL to
the commands; a folder path is only turned into an address for local data.
The URL is shown in the Organizer and has the form
`http://<server>:<port>/<root>/users/<user>/<project>/<calculation>`.

### A calculation

| Item | Contents |
|---|---|
| `.ecce-meta` | metadata, below |
| `Inputs/` | the input as launched: `orca.inp`, `g16.g16in`, `ecceqm.qmin`, `gromacs.mdp`, ... |
| `Outputs/` | the program's output (`*.out`, `g16.g16out`, `ecceqm.qmout`, `gromacs.gmxlog`, ...) and `eccejobstorelog.ecce_run_log`, the submit and monitor events |
| `Props/` | parsed results, one XML file per property, see "Properties" |
| `Parameters/` | the setup: `chemsys.mvm` (the structure), basis set, code parameters |

`Inputs/` and `Outputs/` are what the job read and wrote. `Props/` is what
ECCE parsed from `Outputs/`. `Parameters/` is what the GUI edited; the input
is generated from it when the calculation becomes Ready.

### .ecce-meta

One property per line pair, `ecce:<key>`, the value after two tabs. Keys that
matter when checking or launching:

| Key | Meaning |
|---|---|
| `state` | Created, Ready, Submitted, Running, Running?, Waiting for Login, Completed, Imported, Killed, Incomplete, Monitor Error, System Error (help page "Run states") |
| `runStatusReason` | why a calculation failed, was killed or is waiting |
| `application` | the code (ORCA, Gaussian 16, NWChem, MOPAC, GROMACS, ECCE-QM, ...) |
| `theory`, `runtype` | method and run type |
| `spinmultiplicity` | spin multiplicity |
| `annotation` | the user's note (set with `ecce-annotate`) |
| `launch_machine`, `launch_queue`, `launch_totalprocs`, `launch_maxwall` | the stored launch settings |
| `job_path` | the run directory on the compute machine |
| `progress` | fraction done while running |

Read it; do not write it. To set the `annotation`, use `ecce-annotate`.

**Is it safe to write `ecce:annotation` into `.ecce-meta` while the Organizer
or a job monitor has the calculation open?** Only if the write does what ECCE's
own does, and then it is no better than `ecce-annotate`. One `.ecce-meta`
holds the properties of every calculation in its folder (one line per
property: resource name, property name, type, value, tab-separated, with
backslash escapes for tab and newline). ECCE changes it by taking an exclusive
`flock` on `.ecce-meta.lock` in that folder, reading the whole file, changing
one property and replacing the file by renaming a temporary file over it, all
under the lock. A reader never sees half a file, because of the rename. A
writer that skips the lock can lose an update: a job monitor storing progress
or state in the same folder between your read and your rename overwrites your
change, or you overwrite its, and the state of a sibling calculation goes
wrong. A running Organizer also keeps the values it has read, so it shows a
hand-written annotation only when it next re-reads the calculation, and can
write its old copy back. Use `ecce-annotate`; it goes through the Organizer
when ECCE is running.

### Studies

An MD study (GROMACS, NWChem MD) is a project-like folder whose tasks
(Prepare, Optimize, Equilibrate, Dynamics, ...) are calculations. `linkbase.xml`
in the study holds the chain: a task takes its starting structure from the one
before it, so it can only be launched after that one has completed.

### The run directory

The run directory on the compute machine (`job_path`) holds everything the
job wrote, including files ECCE does not copy back (for example ORCA `.hess`).
`ecce-fetch` brings such files into `Outputs/`.

### A separate state directory

ECCE's settings, machine registrations and per-user service state live in
`$ECCE_REALUSERHOME/.ECCE`, which the launchers set to `$HOME`. For an
experiment, point both the home and the data folder somewhere disposable
before starting ECCE and the commands:

    export ECCE_REALUSERHOME=$(mktemp -d)
    export ECCE_LOCAL_DATA=$ECCE_REALUSERHOME/data
    ecce &

All commands below must run with the same two variables as the ECCE they talk
to. Other variables: the `ENVIRONMENT.md` file next to this one.

## Commands

The `ecce-launch`, `ecce-terminate`, `ecce-reconnect`, `ecce-fetch`,
`ecce-annotate`, `ecce-rerun`, `ecce-move`, `ecce-copy`, `ecce-mkdir`, `ecce-rename` and `ecce-workflow` commands do not start ECCE; they ask the running Organizer to act, so ECCE has
to be running with the same data mode. Without it they print `ECCE is not
running: start ECCE first` and do nothing. A calculation is given as a folder
(local data) or a URL (data server). One line is printed per calculation.

Exit codes of these four: 0 everything done; 1 something was not done; 2 no
running ECCE session; 3 the Organizer is busy with a Launch Selected batch
(try again later); 4 no answer within the time limit (it may have been
carried out: check before repeating); 64 usage error.

### ecce-launch

    ecce-launch [on=MACHINE] [profile=NAME] [queue=QUEUE] [wall=H:MM] <calculation>...

Submits Ready calculations with the rules of **Run Mgmt > Launch Selected**,
using the settings stored in each calculation unless overridden. `on=` and
`profile=` choose a registered machine and allocation profile; `queue=` and
`wall=` replace the stored values. Output per calculation: `name: job id N` or
`name: not launched: reason`; `name: note: ...` lines say which setting was
filled in or dropped (for example no stored queue).

    ecce-launch ~/.ECCE-local/users/local/Examples/water-opt
    ecce-launch on=cluster queue=short wall=2:00 proj/calc-1 proj/calc-2

### ecce-rerun

    ecce-rerun [--last-geometry] [cores=N] [wall="D H:M"] [on=MACHINE] [launch] <calculation>...

Uses the Organizer's **New from This > Rerun**: copies the full setup
(structure, code, theory, basis, run type and launch settings) into the same
folder, removes the copy's results and makes a finished, killed or failed
calculation Ready. The original stays untouched. Names are `<name>_r2`,
`_r3`, and so on, taking the first free name. Submitted, Running and Waiting
for Login calculations and non-calculations are refused with their reason.

`--last-geometry` uses **New from This > Continue from Last Geometry**
instead: the copy starts from the last geometry in the calculation's stored
geometry trace, and its annotation begins `continued from <name> (last
geometry)`. A Submitted, Running or Waiting calculation is ended first, as
`ecce-terminate` does; the result is the same copy as for a killed run. With no
stored geometry yet the command says so (`not rerun: no stored geometry yet`),
makes no copy and leaves a running job alone. `cores=N` and `wall=` (`D H:M`,
`H:M` or minutes) are stored as the copy's launch settings, with or without
`--last-geometry`; on a machine other than the stored one, `on=` still applies
that machine's limits at launch.

With `launch`, submits the new copy as `ecce-launch` does; `on=MACHINE`
overrides its launch machine. Output: `name: rerun as new`, optionally
`, job id N`, or `name: not rerun: reason`. A launch failure identifies the
copy that was made and leaves it available. Other calculations still proceed.
Exit codes, as ecce-move: 0 all done, 1 any refusal, 2 no running
session, 4 no answer in time (may have been carried out; check before repeating), 64 usage. Local data paths and data-server URLs only.

    ecce-rerun project/killed-job
    ecce-rerun on=localhost launch project/finished-job
    ecce-rerun --last-geometry cores=16 wall="2 0:00" launch project/slow-opt

### ecce-terminate

    ecce-terminate [--reason "text"] <calculation>...

Cancels submitted, running or waiting calculations without a confirmation
question. Calculations in other states are refused with their state. The reason
is stored as `runStatusReason`. Output: `name: terminated, job id N` or
`name: not terminated: reason`.

    ecce-terminate --reason "wrong basis set" proj/calc-1

### ecce-annotate

    ecce-annotate <calculation>... "text"

Sets the Annotation field (the note shown in the Organizer) of calculations
and of the tasks of an MD study; the last argument is the text, `""` clears
it. With ECCE running the Organizer stores it as its Annotation editor does
and shows it at once. Without a running session, a calculation in the local
data folder is written through the same data layer (with its locking); with a
data server it needs the running Organizer and says so (exit 2). Output:
`name: annotated` or `name: not annotated: reason`.

    ecce-annotate proj/calc-1 "ORCA B3LYP/def2-SVP, tests the new basis"

### ecce-move, ecce-copy and ecce-rename

    ecce-move <calculation or folder>... <folder>
    ecce-copy <calculation or folder>... <folder>
    ecce-rename <calculation or folder> <new name>

Move calculations (or folders) into an existing folder of the same data store,
or give one a new name, with the Organizer's own move, so its tree is updated
at once. Never move them with `mv` while ECCE is running: the Organizer keeps
stale nodes. Refused, with the reason: a calculation whose job is Submitted,
Running or Waiting for Login, or a folder holding one (the others still move,
exit 1); a name that already exists in the target (nothing is overwritten); a
system folder. The folder may be in another tree (Local data, `sftp://` SSH
folder, ECCE server; give its URL): the move is then copy, verify file count
and sizes, remove the source, and a failure removes the partial copy and
leaves the source. `ecce-copy` copies (a running calculation may be copied).
Output: `name: copied to folder`, `name: moved to folder`, `name: renamed to new name`, or
`name: not moved: reason` / `name: not renamed: reason`. Without a running
ECCE nothing is done (exit 2).

    ecce-move proj/calc-1 proj/calc-2 archive
    ecce-rename proj/calc-1 calc-1-b3lyp

### ecce-mkdir

    ecce-mkdir [-p] [--json] <project or folder>...

Makes a project or folder with the Organizer's own New Project, so its tree
shows it at once; then `ecce-move` can put calculations into it. The parent
must exist unless `-p` is given, which makes the missing parents too and is
not an error when the folder already exists. Output: `path: created`,
`path: exists` (with `-p`), or `path: not created: reason` (exists without
`-p`, missing parent, invalid name, outside the data folder, inside a
calculation); with `--json` an array of `{"path", "ok", "created", "url"}` or
`{"path", "ok": false, "reason"}`. Exit codes as ecce-move: 0 all done, 1 any
refusal, 2 no running session, 4 no answer in time, 64 usage.

    ecce-mkdir -p proj/series/step-1
    ecce-move proj/calc-1 proj/series/step-1

### ecce-reconnect

    ecce-reconnect <calculation>...

Reconnects calculations that were left Waiting for Login, or Submitted or
Running with nothing following the job (ECCE restarted, connection lost).
Reconnecting reads the job's output again from the start. A machine that needs
a password asks for it in the Organizer window. Output: `name: reconnected,
job id N` or `name: not reconnected: reason`.

    ecce-reconnect proj/calc-1

### ecce-fetch

    ecce-fetch [--list] <calculation> [pattern ...]

Copies files from the run directory into the calculation's `Outputs/`, for a
job whose automatic copy-back missed them. Works on finished calculations
too. Patterns are file-name patterns relative to the run directory; the
default is `*.cube *.cub *.pdos`. `--list` lists the run directory and stores
nothing. Output per file: `name: fetched (N bytes)`, `pattern: no file
matched`, or `name: not fetched: reason`. Exit 0 needs at least one file
fetched (or listed) and nothing refused.

    ecce-fetch --list proj/calc-1
    ecce-fetch proj/calc-1 '*.hess'

### ecce-contacts

    ecce-contacts [--format text|csv|json] [--hbonds] [--group] [--cutoff-extra A] <file>

Reads a structure or an ensemble file and prints coordination shells,
hydrogen bonds and, for an ensemble, a pattern key per frame (`--group` keeps
one representative per pattern with its count). It reads files and talks to no
ECCE session.

    ecce-contacts --hbonds --format json solvated.xyz
    ecce-contacts --group --hbonds trajectory.xyz

Exit code 0 on success; anything else is an error message on stderr.

### ecce-rate

    ecce-rate REACTANT[,REACTANT2] TS [PRODUCT[,PRODUCT2]] [--T 298.15 | --T LOW:HIGH:STEP] [--standard-state 1M|1atm] [--treatment SPEC] [--sigma N] [--tunnelling none|wigner|eckart|all] [--json]

Conventional TST rate constant k = kappa sigma (kB T/h) c0^(1-m) exp(-dG‡/RT)
from stored calculations; dG‡, dH‡, dS‡ and the ZPE-corrected barriers come from
`ecce-thermo` (same `--treatment`, default `qrrho-grimme:100`). The TS must have
exactly one imaginary mode (otherwise exit 1 with the reason); reactants and
products with imaginary modes give a warning. kappa is none, Wigner or Eckart
(unsymmetric, from the imaginary frequency and the ZPE-corrected forward and
reverse barriers; symmetric with a note when no product is given). With a
product: reverse k and K_eq consistent with dG_rxn. Over a `--T` range:
Arrhenius (A, Ea) and Eyring (dH‡, dS‡) fits. Units are in the output
(`unit` in `--json`): s^-1, M^-1 s^-1 or cm^3 molecule^-1 s^-1. Half-life is
ln2/k, pseudo-first order at the standard concentration for two reactants.
Formulas in `ecce-rate --help`. Exit 0 done, 1 refused or failed.

    ecce-rate proj/complex proj/ts proj/product --T 250:400:25 --json

### ecce-ensemble

    ecce-ensemble pick <crest calc> [n=N] [--rmsd A] [--skip-failed] [--create TEMPLATE FOLDER] [--json]
    ecce-ensemble minimum <calculation|folder|pattern>... [--treatment SPEC] [--json]
    ecce-ensemble benchmark <spec file> [--treatment SPEC] [--T K] [--save NAME] [--dir DIR] [--json]

Reads stored calculations; talks to no ECCE session. `pick` lists the N
(default 5) lowest distinct members of a CREST ensemble: members with the same
protonation and a heavy-atom RMSD below `--rmsd` (default 0.50 A, after a
fit, atoms in stored order) are merged. Each pick is checked against the
input structure: protonation unchanged, every ion that is bound in the input
still bound, metal-N/O/S contacts listed with distances (cutoffs in `--help`).
`--create` makes one calculation per pick through `ecce-series` from the
setup of a template calculation, without launching. `minimum` gives the
lowest-G calculation without imaginary modes; calculations with imaginary
modes, without frequencies or without G are listed and excluded, and
calculations of different methods are refused. `benchmark` reads reactions
over species (several calculations per species: the lowest-G minimum is used),
and gives dG with the 1 M standard state (as `ecce-thermo`), log K,
experimental log K with its source and calc - exp; `--save NAME` stores
`NAME.benchmark.json` next to the spec file. The spec file format is in
`ecce-ensemble --help`. Exit 0 all done, 1 something not done.

    ecce-ensemble pick proj/crest-run n=5 --json
    ecce-ensemble minimum 'proj/conformers/*' --treatment qrrho-grimme:100
    ecce-ensemble benchmark proj/logk.txt --save logk-set1

### ecce-batch-smiles

    ecce-batch-smiles [--template CALC] [--name COL] [--smiles COL] [--response COL]... [--no-responses] [--header yes|no] [--replace] [--dry-run] [--launch] [--json] <folder> <table.csv|.tsv|->

Makes one calculation per row of a table of names, SMILES and measured values
in `<folder>`, as **Tools > Batch Add from SMILES** does: the structure comes
from the SMILES (formal charges give the charge) and is cleaned, and the setup
is that of `--template` (default: xTB GFN2-xTB optimisation). `-` reads the
table from standard input. Columns are given by header or 1-based number;
without `--name`, `--smiles` and `--response` they are guessed, and `--header`
says whether the first line is a header. The measured values are stored on each
calculation (`ecce:qsar.measured`, JSON keyed by the column header). `--replace`
makes again the calculations of the same name that were never launched;
`--dry-run` only reads the table and reports the rows; `--launch` runs
`ecce-launch` on the new calculations and needs a running ECCE. Output per
row: `name: made <url> (charge N)` or `name: not made: reason`. Exit 0 all
made, 1 some not, 2 no folder or table, 64 usage error.

    ecce-batch-smiles --dry-run proj/series logk.csv
    ecce-batch-smiles --template proj/xtb-template proj/series logk.csv

### ecce-qsar

    ecce-qsar table <folder|calc>... [--groups G,...] [--no-qm] [--csv FILE] [--json]
    ecce-qsar table --from-csv FILE [--smiles-col N] [--name-col N] ...
    ecce-qsar pca TABLE.csv [--y COL]... [--exclude COL]... [--scale uv|pareto|ctr|none] [--components N] [--cv G] [--outdir DIR] [--json]
    ecce-qsar pls TABLE.csv --y COL [--exclude COL]... [--scale uv|pareto|ctr|none] [--components N] [--cv G] [--test-names A,B | --test-fraction F --seed S] [--permutations N] [--save-model FILE] [--outdir DIR] [--json]
    ecce-qsar predict MODEL.json TABLE.csv [--csv FILE]
    ecce-qsar columns TABLE.csv
    ecce-qsar groups

Reads stored calculations and table files; talks to no ECCE session and
changes no calculation. `table` makes one row per calculation of a folder made
by Batch Add from SMILES: name, state, measured values, descriptors computed
from the SMILES, and the calculation's own results (HOMO, LUMO, dipole,
charges); calculations whose optimisation did not converge are flagged in the
columns `opt_converged` and `problems`. `pca` and `pls` fit a model to such a
table, `predict` applies a saved model, `columns` lists a table's columns and
`groups` the descriptor groups. `ecce-qsar --help` has the rest.

### ecce-umbrella

    ecce-umbrella windows <pull calc> [--spacing 0.1] [--from D] [--to D]
    ecce-umbrella extract <pull calc> --out DIR [same choices]
    ecce-umbrella wham <window calc>... [--bins 200] [--temp 300] [--zero end|min|none] [--jacobian]

Umbrella sampling from a stored GROMACS pull run; talks to no ECCE session.
`windows` lists the frames of the pull trajectory at the chosen spacing of the
pull coordinate (frame, time, distance); `extract` writes `window_NN.gro` (the
whole system, box included) into `DIR`; `wham` runs `gmx wham` on the windows'
`topol.tpr` and `pullf.xvg` and prints the PMF (kJ/mol) and the histograms,
with overlap warnings. Files are looked for in a calculation's `Outputs`
folder, then in the folder itself. `gmx` is `--gmx`, else `$ECCE_GROMACS`,
else `gmx` or `gmx_mpi` on PATH. `--json` is available for the Builder panel's
use.

### ecce-workflow

    ecce-workflow make <calculation> [name]
    ecce-workflow move <calculation> <workflow>
    ecce-workflow create <workflow-or-new-name> <calculation>... [--link A:B[=role]]... [--auto-links]

**Make Workflow...** and **Move into Workflow >** of the Organizer: `make`
moves a calculation into a new workflow of its folder as a step, `move` into an
existing one, and `create` does this for several calculations of one folder
with the links given (A and B are calculation names) or, with `--auto-links`,
the links the dialog would propose. Like `ecce-launch` it asks the running
Organizer, so ECCE has to be running. Output: `ok: <workflow URL>` or
`error: reason`. Exit 0 done, 1 refused or no answer, 2 ECCE is not running,
64 usage error.

### ecce-xrd

    ecce-xrd fibre STRUCTURE [--select LIST] [--axis auto|selection|x,y,z] [--map film|rz] [--out FILE]... [--json] ...
    ecce-xrd powder STRUCTURE [--source cu|mo|co] [--tt-min D] [--tt-max D] [--fwhm D] [--measured FILE] [--table FILE.csv] [--out FILE]... [--json] ...
    ecce-xrd layer STRUCTURE (--zone h=N|k=N|l=N | --normal U V W [--offset N]) [--resolution D] [--out FILE]... [--json] ...
    ecce-xrd frame STRUCTURE [--source cu|mo|co] [--phi D] [--osc D] [--distance MM] [--label N] [--out FILE]... [--json] ...

Calculates X-ray diffraction patterns of a structure file (.xyz, .pdb or
.mvm; `--select` takes atom numbers from 1, such as `1-40,57`); talks to no
ECCE session. `fibre` is the pattern of molecules aligned on one axis and
rotated about it, as a film or an (R, Z) map. `powder`, `layer` and `frame`
need a periodic structure: a .cif, an .mvm with lattice vectors, or an .xyz
whose comment line has `Lattice="ax ay az bx by bz cx cy cz"`. `powder` gives
intensity against 2θ with the K-alpha1/K-alpha2 doublet when `--source` is
given, and draws a measured pattern (text columns, .xrdml or .brml) as a
second curve with `--measured`. `layer` is a reciprocal-lattice layer (hk0,
h0l, 0kl or a higher layer) with spots sized by |F|^2. `frame` is the
detector frame of a rotating crystal. `--out` takes `.png`, `.csv` or, for
`fibre` and `frame`, `.npy`, and may be repeated. Wavelengths and cell
lengths are in Å, angles in degrees. `ecce-xrd <subcommand> --help` lists
every option; the help page **Diffraction** describes the views.

    ecce-xrd fibre helix.mvm --map rz --rmax 0.7 --zmax 0.7 --out rz.png
    ecce-xrd powder nacl.cif --source cu --measured scan.xy --out powder.png

### Other command-line tools

On Linux these commands are on PATH. In the macOS app they are in
`ECCE.app/Contents/Resources/bin`; add that folder to PATH to use them from a
terminal. The Windows package currently supplies only the main `ecce.cmd`
launcher, without command wrappers for these tools.

| Command | Purpose |
|---|---|
| `ecce-localdata` | Print the local data folder |
| `ecce-series` | Work with a series of calculations (command line only) |
| `ecce-check-props` | Check stored property dimensions |
| `ecce-si-export` | Export structures and a Supporting Information table; `--checks` makes the Checks table (with core-hours), `--rank` the Compare Energies table. Command line only |
| `ecce-displacement` | Compare geometries and aligned RMSD |
| `ecce-bandshift` | Match vibrational bands and report shifts |
| `ecce-spectrum` | Process spectral curves |
| `ecce-displace` | Displace a structure along a normal mode |
| `ecce-thermo` | Recompute thermochemistry from stored results |
| `ecce-rate` | TST rate constants with Wigner/Eckart tunnelling from reactant, TS and product calculations |
| `ecce-force-compare` | Compare stored forces (command line only) |
| `ecce-mlp` | Run an ML potential job from its input file |
| `ecce-confsearch` | Run a simple conformer search from its input file |
| `ecce-qm` | Run the bundled quantum chemistry program, when built |
| `ecce-results` | Convert and inspect results files, when built with HDF5 |
| `ecce-fragorb` | Fragment-orbital analysis (`analyse <complex> <fragment>...`) and PDOS/OPDOS (`dos`) from results.h5 files of a complex and its fragment calculations at one geometry; `--json`, `--help` has the syntax |
| `ecce-morse` | Morse fit of a stored bond-length scan: De, a, re, ω_e, ω_e x_e, D0 (`ecce-morse <calc> [--window KJ_PER_MOL] [--atoms I,J] [--masses M1,M2] [--json]`) |
| `ecce-gmxanalysis` | GROMACS analysis of a stored GROMACS calculation, as text, CSV or JSON: `<calc> groups`, `rdf`, `energy`, `rms`, `gyrate`, `density`, `hbond`; selections like `"residue:SOL & element:O"`; needs `gmx` |
| `ecce-mocomp` | Atomic-orbital composition of one molecular orbital (`CALC --mo N\|homo\|lumo [--beta] [--method mulliken\|c2] [--min PCT] [--by ao,shell,type,atom] [--tsv]`), `--list` the orbitals, `--spin` Mulliken spin populations |
| `ecce-density-field` | ELF, LOL, reduced density gradient, sign(λ2)ρ or density from a calculation's stored orbitals as a cube file (`CALC --field F [--spacing A] [--margin A] [--threads N] -o out.cube`), or all fields at points (`CALC --at X,Y,Z`); local data |
| `ecce-pdbqt` | Prepare AutoDock PDBQT files for docking: `--frag FILE --atoms LIST --as ligand\|receptor -o OUT`, `--ligand FILE.sdf`, `--receptor FILE.pdb`, `--info FILE` |
| `ecce-site-screen` | Interstitial or vacancy site screening of a periodic host: `make <template> interstitial\|vacancy --element X`, then `results <folder>` for E - E(host) in eV |

See the matching help page for inputs, options and units.

### Administration

These run on the machine that hosts or connects to a data server, not on a
calculation.

| Command | Purpose |
|---|---|
| `ecce-diagnose [-o directory] [run-directory]` | Collects what is needed to diagnose a problem into one text file, `ecce-diagnostics-<host>-<time>.txt`. Changes nothing and collects no passwords, but the file holds host names, user names and paths: read it before attaching it to an issue. `ecce --bug` runs it too |
| `ecce-remote-setup <server-host> [data-port]` | Points this client installation at a central server; run once per client installation, as the owner of the installation. Users then start `ecce -remote` |
| `ecce-dataserver-start`, `ecce-dataserver-status`, `ecce-dataserver-stop` | Start, check and stop this user's data server (`ecce-dataserver-stop --if-unused` refuses while another session or a job still uses it) |
| `ecce-dataserver-adduser` | Add an account to this user's data server (`-b USER PASS [FIRST] [LAST]` or `--stdin USER [FIRST] [LAST]` without prompts) |
| `ecce-site-publish [DIR]` | Copy this install's site machine list to where the data server serves it to `-remote` clients |

## Checking a result

In this order:

1. **State.** `.ecce-meta` `state`: Completed means the code finished
   normally, not that the result is right. For Killed, Incomplete, Monitor
   Error or System Error read `runStatusReason`.
2. **The run log.** `Outputs/eccejobstorelog.ecce_run_log` shows submit,
   queue and monitor events; a failed submit leaves the calculation Ready with
   the reason there.
3. **The output.** The end of `Outputs/` file and the code's own convergence
   lines.
4. **The parsed properties.** Read `Props/` first, units are in the XML. A
   scalar is `<value name="TE" units="Hartree">...</value>`, a vector
   `<vector name="VIBFREQ" rows="30" units="cm-1">...</vector>`; orbitals and
   geometry traces are tables.
5. **The chemistry.** An optimisation converged by the code's criteria rather
   than stopped; 0 imaginary frequencies for a minimum and exactly 1 for a
   transition state, along the intended coordinate; the same method, basis,
   dispersion and solvent model before energies are compared; electronic,
   ZPE-corrected and Gibbs energies not mixed; open shells with spin
   multiplicity as intended and S2 near its ideal value; the basis set listed
   once; the geometry sensible (bond lengths, nothing dissociated); for MD,
   equilibration reached before averages are taken.

State what was checked and what was not. Do not claim agreement with the
literature or another program without having compared.

### Properties

`Props/<KEY>` files. Common keys:

- Energies: TE (Hartree), TEVEC (per optimisation step), DELTAE, EGIBBS, ETIME.
- Geometry: GEOMTRACE (per step); SCANENERGY, SCANGEOM, SCANCOORD (relaxed
  scans); IRCCOORD, IRCENERGY, IRCGEOM (IRC path).
- Gradient: EGRAD, EGRADVEC.
- Vibrations: VIBFREQ (cm-1; leading near-zeros are translations and
  rotations), VIBIR (km/mol), VIBRAM, VIBSYM.
- Orbitals: MO, ORBENG, ORBOCC, MOAOORDER; LOCMO, LOCMOENG, LOCMOOCC,
  LOCMOLABEL (localised orbitals, ECCE-QM).
- Charges: MULLIKEN, LOEWDIN, HIRSHFELD, CHELPG, MK, APT where computed.
- Other: DIPOLE, S2; for MD TIMEVEC, TEMPVEC, PRESSVEC, MDPEVEC, MDKEVEC.

The full list with units is `data/client/config/properties` in the ECCE
source. `ecce-results` converts orbitals and Hessians from Molden, fchk and
ORCA files into one HDF5 file and checks the conversion (help page "The
results file format").

## Documentation shipped with ECCE

The help pages are in `<install>/doc/help/` (sources `help/src/*.md` in the
repository). Read the page before explaining a feature, for example
`launch-selected`, `reconnect-selected`, `run-states`, `ecce-qm-files`,
`machines`, `register-machines`, `importing-a-calculation`. Do not guess menu
names.

## Keeping this file current

A new command or option is added to this file in the same change that adds it,
as for the help pages. `tests/ai/agents_md_test.py` fails when a command
named under "Commands" is not in the build.
