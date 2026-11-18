# Scripting and AI assistants

Everything ECCE does can be done from the command line, so scripts and AI
assistants can work with it too. The commands are `ecce-launch`,
`ecce-terminate`, `ecce-rerun`, `ecce-annotate`, `ecce-move`, `ecce-copy`, `ecce-rename`,
`ecce-reconnect`, `ecce-fetch` and
`ecce-contacts`; see [Launch Selected](launch-selected.md) for the launch and
terminate commands. `ecce-reconnect <calculation> jobid=N` attaches a job
found in the queue after a `submitted?` answer.

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

## Builder shortcut checks

In an isolated GUI test, `ECCE_BUILDER_SCRIPT` accepts
`key text cut|copy|paste|undo|selectall`. The hook uses the focused editable
single-line text field, or the first visible enabled one. If the layout
contains none, it opens the Symmetry panel and uses its first text field.
It sets the text to `abc`, focuses the field, selects its text and sends
the shortcut command. `key text selectall` selects text while leaving the
structure's selection alone; `key viewer selectall` focuses the 3-D view
and selects all atoms.

## Fragments and ORCA counterpoise inputs

The Builder's `ECCE_BUILDER_SCRIPT` hooks can define fragments and export
counterpoise single-point inputs. Atom and fragment numbers start at 1:

```text
fragments auto
fragments
fragment 1 atoms 1-3
fragment 2 atoms 4,5,6
fragment 1 charge 0 mult 1
fragment 2 charge 0 mult 1
counterpoise orca cp-water 1 HF def2-SVP
save
```

`fragments auto` uses connected molecules, numbered by their first atom;
`fragments` prints the assignments and fragment charge/multiplicity. An atom
override moves the listed atoms into that fragment. Automatic states are
neutral, with multiplicity from electron parity. They do not infer ionic
charges: for Na⁺···H₂O with total charge +1, set the sodium fragment to
`charge 1 mult 1`. A state change whose fragment charge sum differs from the
structure's total charge is rejected. Automatic defaults on a charged
structure remain pending until that sum is corrected. Save stores the
assignments and states in the MVM structure; XYZ does not preserve them.
Atom overrides retain existing states; correct any incompatible state after
moving atoms. Each state command edits one fragment, so setting +1 and −1
on a neutral ionic pair in two sequential commands is not supported yet.

`counterpoise orca FOLDER [MULT [ROUTE...]]` writes `complex.inp` and one
`fragment-N.inp` per fragment at the same geometry, using `Symbol:` ghost
centres for every partner atom. Each monomer uses its fragment charge and
multiplicity. The optional MULT is the complex multiplicity; the default
comes from the calculation, or electron parity for a structure file. The
default route is `HF def2-SVP`. Use an empty destination folder. The export
checks charges and electron/multiplicity consistency before writing.

These are inputs only: launch, collection, corrected energy properties and
the calculation checkbox are not provided yet. Once energies are available,
the Boys–Bernardi interaction energy is
ΔE_CP = E_AB(AB) − E_A(AB) − E_B(AB), where the parentheses denote the
full complex basis and every energy uses the complex geometry. Relative to
ΔE_raw = E_AB(AB) − E_A(A) − E_B(B), the positive BSSE correction is
ΔE_CP − ΔE_raw. There is no corrected optimisation in this workflow.

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
`calc-1: moved to archive` or `calc-2: not moved: reason`. Without a running
ECCE the commands print a message and exit with status 2. The Organizer
confirms each request as soon as it arrives, so a command sent while the
Organizer is still busy with an earlier one is not mistaken for "not running".
If the Organizer confirmed but gave no answer in time, each calculation gets
the line `no answer from the Organizer; it may have been carried out - check
before repeating` and the exit status is 4. If the Organizer ends after
confirming, the command notices within a second or two, prints `the Organizer
ended before answering; the request may have been carried out` and exits with
status 5. This check needs the Organizer to run on the same computer as the
command; where that cannot be told, the command waits for the full time.

`ecce-copy <calculation or folder>... <folder>` copies in the same way, and
the target folder may be in another tree (Local data, an SSH folder or an ECCE
server), given by its URL, for example
`ecce-copy proj/calc-1 sftp://me@host/data/project`. `ecce-move` to another
tree copies, checks the file count and sizes, and only then removes the
original; on a failure the partial copy is removed and the original stays.
A name that exists in the target is never overwritten. See
[Copying and moving between trees](installation.md#copying-and-moving-between-trees).

## Moving a queued calculation

`ecce-launch --move on=MACHINE <calculation>...` cancels a Submitted job that
is still pending in a Slurm queue and submits the calculation on `MACHINE`,
keeping its name and run log. A job that has started is refused. See
[Launch Selected](launch-selected.md). Exit status as for `ecce-launch`.

## Rerunning a calculation

`ecce-rerun [on=MACHINE] [launch] <calculation>...` asks the running
Organizer for **New from This > Duplicate**. It makes a copy in the same folder,
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
1 any refusal, 2 no running ECCE session, 4 no answer in time (it may have
been carried out; check before repeating), 5 the Organizer ended before
answering, 64 invalid usage.

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

## Counterpoise correction

The interaction energy of a complex AB, E(AB) − E(A) − E(B), is too negative in
a finite basis because each monomer borrows its partner's basis functions in
the complex (basis-set superposition error, BSSE). The counterpoise correction
of Boys and Bernardi evaluates the monomers in the full complex basis, with the
partner's atoms as ghost atoms (basis functions, no nuclei, no electrons):

```text
ΔE_CP  = E_AB(AB) − Σ E_X(AB)        X = each fragment, at the complex geometry
ΔE_raw = E_AB(AB) − Σ E_X(X)         monomers in their own basis
BSSE   = ΔE_CP − ΔE_raw = Σ E_X(X) − Σ E_X(AB)   (positive)
```

Define the fragments in the Builder first (`fragments auto`, above). In the
Calculation Editor choose an Energy run, open Runtype Details and tick
*Counterpoise correction*. The box is available for Energy runs.
For ECCE-QM and ORCA, the second tick, *Monomers in their own basis too*, adds
one run per fragment and gives the raw energy and the BSSE. One job runs the complex, then each
monomer in sequence, in the same run folder. The fragment charges must add up
to the calculation's charge.

The Energies panel shows *Counterpoise-corrected Interaction Energy*, and with
the second tick the *Raw Interaction Energy* and the *BSSE*, in the energy unit
you chose, plus a *Counterpoise Summary* line in kJ/mol and kcal/mol.

Gaussian 16 uses its native `Counterpoise=N` keyword with `(Fragment=n)` atom
tags and each fragment's charge and multiplicity on the charge line. Tick
*Counterpoise correction* in its Runtype Details for an Energy run after defining
the fragments. The fragment charges must add up to the calculation's charge.
Gaussian 16 returns the corrected interaction energy, raw interaction energy
and BSSE in one job; all three appear in the Energies panel, together with the
summary. There is no separate own-basis checkbox for Gaussian 16. The corrected
total energy printed in its log is distinct from the corrected interaction
energy displayed by ECCE.
