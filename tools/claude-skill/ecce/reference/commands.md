# ECCE command-line tools

Source: `AGENTS.md` and the help pages `launch-selected`, `scripting-and-ai` and `new-series`. Run `<command> --help` for the full syntax of the installed version.

**macOS:** the commands are in `/Applications/ECCE.app/Contents/Resources/bin`; put that folder on `PATH`. If a command can't find its environment from a plain shell, run it through the app:

```
ECCE_APP_RUN=1 /Applications/ECCE.app/Contents/MacOS/ecce <command> ...
```

The commands below **ask the running Organizer to act**, so ECCE must be open, with the same data mode and state directory. Without it they print `ECCE is not running: start ECCE first` and exit with 2. A calculation is given as its folder path (local data) or URL (data server). They print one line per calculation.

Exit codes: 0 all done; 1 something not done (or no answer in time); 2 no running ECCE; 3 the Organizer is busy with a Launch Selected batch, so try again later; 64 usage error.

| Command | What it does |
|---|---|
| `ecce-launch [on=MACHINE] [profile=NAME] [queue=Q] [wall=H:MM] <calc>...` | Submits Ready calculations with the rules of Run Mgmt > Launch Selected. Uses the stored settings unless they are overridden; output `name: job id N` or `name: not launched: reason`. `name: note: ...` lines say what was filled in or dropped. **Read them**: e.g. no stored queue means the machine's default queue is used, and a machine without the time-limit capability silently drops the wall time. |
| `ecce-terminate [--reason "text"] <calc>...` | Cancels submitted, running or waiting jobs without asking. The reason goes into `runStatusReason`. It then looks for programs still running in the run directory and stops them. |
| `ecce-reconnect <calc>...` | Re-attaches the monitor to a job that is Waiting for Login, or Submitted/Running with nothing following it (after an ECCE restart or a lost connection). |
| `ecce-fetch [--list] <calc> [pattern ...]` | Copies files from the run directory into `Outputs/`; default `*.cube *.cub *.pdos`. Works on finished jobs too. `--list` only lists the files. |
| `ecce-annotate <calc>... "text"` | Sets the Annotation; `""` clears it. Works without a running ECCE for local data. |
| `ecce-move <calc or folder>... <folder>` | Moves within the same data store. Refuses Submitted, Running or Waiting jobs, and never overwrites. |
| `ecce-rename <calc or folder> <new name>` | Renames. |
| `ecce-rerun [--last-geometry] [cores=N] [wall="D H:M"] [on=MACHINE] [launch] <calc>...` | New from This > Rerun: a copy `<name>_r2`, `_r3`, ... in the same folder, Ready, with its results removed. `--last-geometry` starts from the last geometry in the stored trace (terminating a running original first). `launch` submits the copy. |

Commands that work on the stored data directly (no running ECCE needed for local data):

| Command | What it does |
|---|---|
| `ecce-series [--charge N] [--mult M] <template> <folder> <structure>...` | One Ready calculation per `.xyz` file or stored calculation, with the template's code, theory, run type, details, basis, solvent and launch settings. Charge and multiplicity come from the xyz comment line (`charge=-1 mult=2`), else charge 0 and the lowest multiplicity. Constraints aren't copied. An open Organizer shows the new calculations once the folder is refreshed. |
| `ecce-localdata` | Prints the data folder. |
| `ecce-si-export [--checks csv] <folder>` | SI export; `--checks` gives the Checks table with core-hours. |
| `ecce-batch-smiles [--template CALC] [--dry-run] [--replace] [--launch] [--json] <folder> <table.csv\|.tsv\|->` | One calculation per row of a table of names, SMILES and measured values (Batch Add from SMILES). `--launch` needs a running ECCE. |
| `ecce-qsar table\|pca\|pls\|predict\|columns\|groups ...` | Descriptor table of a folder of calculations, PCA and PLS models of it. `ecce-qsar --help`. |
| `ecce-umbrella windows\|extract\|wham <calc>...` | Umbrella-sampling windows from a GROMACS pull run, and WHAM (needs `gmx`). |
| `ecce-xrd fibre\|powder\|layer\|frame <structure> [--out FILE]...` | Calculated diffraction patterns; `--measured FILE` overlays a measured powder pattern. |
| `ecce-workflow make\|move\|create ...` | Make or extend a workflow from calculations of a folder. Asks the running Organizer, so ECCE must be open. |
| `ecce-morse <calc> [--json]` | Morse fit of a stored bond-length scan. |
| `ecce-gmxanalysis <calc> groups\|rdf\|energy\|rms\|gyrate\|density\|hbond` | GROMACS analysis of a stored GROMACS calculation (needs `gmx`). |
| `ecce-mocomp CALC --mo N\|--list\|--spin` | Atomic-orbital composition of a molecular orbital, orbital list, spin populations. |
| `ecce-density-field CALC --field elf\|lol\|rdg\|signl2rho\|density -o out.cube` | Field cube file from stored orbitals; `--at X,Y,Z` gives values at points. |
| `ecce-pdbqt --frag\|--ligand\|--receptor ... -o OUT.pdbqt` | AutoDock PDBQT preparation for docking. |
| `ecce-site-screen make\|results ...` | Interstitial or vacancy site screening of a periodic host. |
| `ecce-fragorb analyse\|dos ...` | Fragment-orbital analysis and PDOS/OPDOS from results.h5 files. |
| `ecce-diagnose [-o dir] [run-dir]` | Collects a diagnostics file for an issue report; changes nothing. |
| `ecce-remote-setup <server-host> [data-port]` | Points a client installation at a central server (administrator). |

| `ecce-thermo`, `ecce-displacement`, `ecce-bandshift`, `ecce-spectrum`, `ecce-displace`, `ecce-force-compare`, `ecce-ensemble`, `ecce-rate`, `ecce-contacts`, `ecce-check-props` | Analysis tools; see `--help`. |

Opening only the machine registry window, without a session: `ecce -machine`.

Known traps:

- A study or calculation copied into the store from outside isn't known to the running Organizer until the folder has been addressed once (refreshed). A task inside a freshly copied study may be refused by `ecce-launch` until the study folder itself has been refreshed.
- An MD study task that waits for the task before it is refused ("waits for ...") until that task has completed.
- A failed submit leaves the calculation Ready, with the reason in the run log.
