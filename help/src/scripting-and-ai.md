# Scripting and AI assistants

Everything ECCE does can be done from the command line, so scripts and AI
assistants can work with it too. The commands are `ecce-launch`,
`ecce-terminate`, `ecce-rerun`, `ecce-annotate`, `ecce-move`, `ecce-rename`,
`ecce-reconnect`, `ecce-fetch` and
`ecce-contacts`; see [Launch Selected](launch-selected.md) for the launch and
terminate commands.

## Command line only

Some functions have no menu in the Organizer and are used from a terminal, by
scripts and AI assistants:

| Command | What it does |
|---|---|
| `ecce-si-export --checks csv <folder>` | the [Checks table](checks-table.md), with the core-hours booked and used |
| `ecce-si-export` | the [Supporting Information export](si-export.md) |
| `ecce-series` | [New Series](new-series.md): one setup on many structures |
| `ecce-force-compare` | compare the stored forces of two calculations |
| `ecce-contacts` | metal-donor contacts of a structure |
| `ecce-annotate --exclude` | the [excluded mark](excluded.md); it is not shown in the Organizer |

## Setting an annotation

`ecce-annotate <calculation>... "text"` sets the Annotation field of
calculations and MD study tasks. With ECCE running, the Organizer stores the
text as its Annotation editor does and shows it at once. Without a running
session, a calculation in the local data folder is written directly, through
the same code and locking ECCE uses.

Do not write `ecce:annotation` into `.ecce-meta` by hand while the Organizer
or a job monitor has the calculation open. One `.ecce-meta` holds the
properties of every calculation in its folder, and ECCE changes it under an
exclusive lock on `.ecce-meta.lock`, by renaming a complete new file over the
old one. A write without the lock can overwrite a state or progress update
that a job monitor stores in the same folder at the same moment, and the
Organizer keeps its own copy of what it has read and can write that back. The
command avoids both.

## Moving and renaming

`ecce-move <calculation or folder>... <folder>` moves calculations into an
existing folder, and `ecce-rename <calculation or folder> <new name>` renames
one. Both ask the running Organizer, as dragging and Edit > Rename do, so the
tree is updated at once; moving the folders behind the Organizer's back leaves
stale entries in the tree. The refusals are the same as in the Organizer: a
calculation whose job is Submitted, Running or Waiting for Login, and a folder
holding one, are not moved (see [Renaming and moving
calculations](rename-and-move.md)); a name that exists in the target is
never overwritten. The other calculations in the same command still move, and
the exit status is 1. One line is printed per calculation, for example
`calc-1: moved to archive` or `calc-2: not moved: reason`. Local data and data
servers are supported. Without a running ECCE the commands print a message and
exit with status 2.

## Rerunning a calculation

`ecce-rerun [on=MACHINE] [launch] <calculation>...` asks the running
Organizer for **New from This > Rerun**. It makes a copy in the same folder,
named `<name>_r2`, then `_r3` and so on, using the first free name. The copy
keeps the structure, code, theory, basis, run type and launch settings;
finished, killed and failed calculations become Ready, with results removed.
The original stays untouched. Submitted, Running and Waiting for Login
calculations, and items that are not calculations, are refused with a reason.

Add `launch` to submit each new copy with its stored launch settings, as
`ecce-launch` does; `on=MACHINE` overrides the launch machine. For example,
`ecce-rerun on=localhost launch project/water` prints
`water: rerun as water_r2, job id N`. Without `launch` it prints
`water: rerun as water_r2`. A refused item prints `name: not rerun: reason`;
other items still proceed. If submission fails, the line identifies the copy
that was made and gives the launch failure; the copy remains available.

`ecce-rerun --last-geometry [cores=N] [wall="D H:M"] <calculation>...` uses
**New from This > Continue from Last Geometry** instead. The copy starts from
the last geometry in the stored geometry trace, and its annotation begins
`continued from <name> (last geometry)`, and its input is written again from that geometry, so the copy is Ready. If the input cannot be written, the copy stays Created and the line says to open it in the Calculation Editor. A Submitted, Running or Waiting
calculation is terminated first, as `ecce-terminate` does. A calculation with
no stored geometry yet is refused with that reason, no copy is made and a
running job is left alone. `cores=N` and `wall=` (`D H:M`, `H:M` or minutes)
set the copy's launch settings, with or without `--last-geometry`; with
`launch` the copy is submitted with them.

Local data paths and data-server URLs are supported. Exit status: 0 all done,
1 any refusal or missing answer, 2 no running ECCE session, 64 invalid usage.

## The AGENTS.md file

ECCE ships a file, `AGENTS.md`, that says how ECCE stores its data (projects,
calculations, `Inputs`, `Outputs`, `Props`, `.ecce-meta`, studies), what each
command does with examples and exit codes, how to check a result, and the
safety rules for working on a scientist's data. It is installed in the `doc`
folder of the ECCE installation (`/opt/ecce/doc/AGENTS.md` on a package
install), next to this help, and is `docs/AGENTS.md` in the source.

To use it, tell the assistant to read the file before it touches anything, for
example: "Read AGENTS.md from the ECCE installation and then check my latest
calculation." Assistants that look for an `AGENTS.md` in the working folder
find it when it is copied or linked there.

For Claude Code, the same instructions are packaged as a skill in the
`share/ecce/ai/skills/ecce` folder of the installation. Copy the `ecce` folder
to `~/.claude/skills/` to use it.

## What to keep in mind

- You stay in control. Ask the assistant to say what it will launch, on which
  machine and with which settings, before it does.
- An assistant can be wrong about chemistry. Check what it tells you against
  the output and against what you know.
- What an assistant reads is sent to its provider. Do not give it data that
  may not leave your computer.
- For an experiment, use a separate state directory so that your own data is
  not touched; AGENTS.md says how.
