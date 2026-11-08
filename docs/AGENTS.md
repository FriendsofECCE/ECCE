# ECCE for scripts and AI assistants

Everything ECCE does can be done from the command line, so scripts and AI
assistants can work with it too. This file says how ECCE stores its data,
which commands there are, and how to check a result. It is read by people,
scripts and coding assistants alike. The scientist stays in control: the
assistant prepares, checks and explains, and acts on ECCE only when asked to.
Nothing here says that an assistant gets the chemistry right; the results are
the scientist's to judge.

Privacy: what an assistant reads (inputs, outputs, structures, this file) is
sent to the provider of that assistant. Do not point an assistant at data that
may not leave the machine.

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
`ecce-annotate`, `ecce-rerun`, `ecce-move`, `ecce-copy` and `ecce-rename` commands do not start ECCE; they ask the running Organizer to act, so ECCE has
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

See the matching help page for inputs, options and units.

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
