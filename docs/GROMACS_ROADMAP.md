# Adding GROMACS to ECCE — implementation roadmap

Status: **design document, nothing implemented.** Written 2026-09-22
against `main` at `71d8879`. GROMACS is **not installed on this
machine**, so every statement about GROMACS itself here comes from its
published documentation, not from a binary — unlike the statements about
ECCE, which come from reading this tree and are cited by file and line.
Where something is an inference it says so.

The maintainer's standing instruction is that **NWChemMD must not be
retired, because it is the intended basis for GROMACS support** (stated
2026-09-22; recorded in `data/client/cap/disabled/README.txt`). This
document works out what "the basis for" actually buys us.

Read `CLAUDE.md`'s code-registration section and its **New-code
checklist** first. This document assumes them. The headline finding is
that **most of that checklist does not apply**, because MD does not go
through the same registration path at all.

---

## 1. The finding that shapes everything else

**MD is a second, parallel code-registration mechanism.** It is not the
electronic-structure path with different scripts in it.

An ES code declares `<InputGenerator>` in its `.edml`, and
`CodeFactory::getFullySupportedCodes()` (`src/dsm/xml/CodeFactory.C`)
treats that plus `<Template>` as the definition of "wired up".
`NWChemMD.edml` declares **neither**. It declares two different keys:

```xml
<Template>       nwch.tpl    </Template>
<PrepareInputGenerator>md.nwprep</PrepareInputGenerator>
<TaskInputGenerator>md.nwtask</TaskInputGenerator>
```

and they are read from two different places:

* `MDPrep.C:3393` — `codecap->get_string("PrepareInputGenerator", ...)`
* `MDEdBase.C:253` — `codecap->get_string("TaskInputGenerator", ...)`

So there are **two generation steps**, not one: a *prepare* step that
builds the system (topology, solvation, counter-ions) and a *task* step
that runs something on the prepared system. That split is not an NWChem
quirk. It is exactly how GROMACS works too — `pdb2gmx`/`editconf`/
`solvate`/`grompp` to build, then `mdrun` to run — which is the real
reason NWChemMD is the right starting point, and a much better reason
than "it is the only MD code present".

**Consequence:** the New-code checklist's central items (`<InputGenerator>`
required, one `ai.<code>` script, `CodeFactory` discovery, the CalcEd
code-switch toolbar) are all about the ES path and do **not** transfer.
Do not start by writing `ai.gromacs`.

## 2. What already exists and would be reused

This is more than scaffolding; it is four applications and a widget
toolkit, all currently driven by one code.

| Piece | Size | Role |
|---|---|---|
| `src/apps/mdprepare` | ~8400 lines | System preparation: the big one |
| `src/apps/mdenergy` | ~365 lines | Single-point energy task editor |
| `src/apps/mdoptimize` | ~374 lines | Minimisation task editor |
| `src/apps/mddynamics` | ~396 lines | MD run task editor |
| `src/wxgui/mdtools` | 16 `.C` files | Shared panels: Control, Dynamics, Optimize, Constraint, Interaction, Files, plus `MDEdBase` |

`mdtools` is where the leverage is. `MDEdBase` is the base class the
three task editors derive from, and it is what reads
`TaskInputGenerator` — so a second MD code slots in underneath the
existing editors rather than needing its own.

The resource graph already models the MD workflow as distinct node
types: `md`, `nwchem_md_study`, `nwchem_md_prepare`, `nwchem_md_energy`,
`nwchem_md_optimize`, `nwchem_md_dynamics`
(`data/client/config/ResourceDescriptor.xml`). A GROMACS integration
needs its own parallel set (`gromacs_md_*`), in **both**
`ResourceDescriptor.xml` and `ResourceDescriptorRxn.xml` — that part of
the checklist does still apply, and missing either file silently omits
the code from the New-Calculation menu with no error anywhere.

## 3. Where GROMACS does not fit, and what to do about it

These are the real design problems. They are not sequencing details.

### 3.1 Force fields and topology are GROMACS's centre of gravity

NWChem MD builds its topology from its own fragment/segment database.
GROMACS expects a force field chosen at `pdb2gmx` time (AMBER, CHARMM,
OPLS-AA, GROMOS) and produces `.top`/`.itp` files that are then
effectively the system definition. ECCE has no concept of "which force
field", and `mdprepare`'s UI is built around NWChem's segment model.

This is the largest single piece of work and the one most likely to be
underestimated. Options, in increasing order of ambition:

1. **Import-only**: the user brings a `.top`/`.gro` built elsewhere, and
   ECCE runs and monitors it. Cheapest, and immediately useful.
2. **Force-field picker driving `pdb2gmx`**: ECCE offers the force
   fields installed with GROMACS and shells out. Needs a way to
   enumerate them, which is a filesystem scan of the GROMACS share
   directory — inferred from the documented layout, not verified here.
3. **Full parity with the NWChem prepare UI**: a genuine rewrite of an
   8400-line application. Not a starting point.

Recommend (1) first: it makes GROMACS *usable* in ECCE without touching
`mdprepare` at all, and it is what tells us whether the rest is worth
doing.

### 3.2 Multi-executable, multi-file, and a binary run control file

GROMACS is not one binary reading one text deck. `grompp` consumes
`.mdp` + `.top` + `.gro` and emits a **binary** `.tpr`, which `mdrun`
then executes. ECCE's model is "generate a text input file, submit it,
tail the output".

Implications, each of which needs a decision:

* The `.mdp` is the file ECCE would generate and the user would edit.
  The `.tpr` is a build artefact. `<DataFiles>` should declare the
  `.mdp` as the primary input, **not** the `.tpr`.
* `grompp` has to run **before** submission. That is exactly what
  `<LaunchPreprocessor>` is for — see `nwchem.launchpp` for the
  pattern — and it is required unconditionally anyway
  (`Launch::postProcessInput()` shells out with no empty-string check).
* `grompp` failures are the common case for a new user (bad topology,
  mismatched atom counts). Its stderr must reach the user rather than
  being discarded — note the `eccejobmaster.log` fix of 2026-09-22 for
  the same class of silent failure.
* Give the `.mdp` a distinctive extension in `<DataFiles>` **and** add a
  matching `AddType` to `packaging/dataserver/httpd.conf.ecce`, or the
  upload silently becomes `text/plain` and the primary-input lookup
  cannot match it. But heed the MOPAC lesson: let the code dictate the
  filename. GROMACS tools are strict about extensions.

### 3.3 Monitoring and trajectories

`eccejobmonitor`'s `.desc` mechanism tails a text file for `Begin`/`End`
markers. GROMACS writes `.log` (text, tailable — good), plus `.edr`
(binary energies) and `.xtc`/`.trr` (binary trajectory).

* Energies and progress can come from the `.log` in the normal way.
* Trajectories cannot. ECCE has a `TrajectoryPanel`
  (`src/apps/builder/TrajectoryPanel.C`) already, so the display side
  exists; the gap is a converter. `gmx energy` / `gmx traj` can emit
  text, so a launch-time or post-processing step is the likely answer.
* Whoever writes the `.desc` should read CLAUDE.md's accumulated
  `.desc` hazards first — `End`-line starvation, `Skip=N` counting the
  `Begin` line itself, and `Begin` strings doubling as hash keys. Those
  have each cost a debugging session.

### 3.4 `gensub` needs a `sub gromacs()`

`$fct = $lccode` is a bare dispatch on the lowercased application type
(`scripts/gensub`), so a code with no matching sub simply cannot be
launched. Easy to miss because it is not in the per-code file set. MPI
launch (`mdrun -ntmpi`/`-ntomp`, or `gmx_mpi`) belongs here.

## 4. Suggested order

1. **Decide the scope question in §3.1.** Everything else depends on it.
   Import-only is the recommended first target.
2. Add `gromacs_md_*` resource types to both descriptor files, and a
   `GROMACS.edml` with `<Template>`, `<TaskInputGenerator>` and a
   `<LaunchPreprocessor>`.
3. Make one task editor work end to end — `mddynamics` is the one people
   actually want — reusing `MDEdBase` and the `mdtools` panels.
4. `md.gmxtask` generating a `.mdp`, and a launch preprocessor running
   `grompp`, with its errors surfaced.
5. `gensub` sub, then `gromacs.desc` against a **real** run's `.log`.
   Test with something that has more than one output shape; the MOPAC
   integration found a property that existed only in one runtype.
6. Trajectory conversion, last.

## 5. Checked while writing this, so it is not guesswork

**The four MD applications still build and are packaged.** `mdprepare`,
`mdenergy`, `mdoptimize` and `mddynamics` all produce binaries in
`build-cmake/` and all four are in the `.deb` at `/opt/ecce/bin/`. So
this is live code, not an abandoned corner. Caveat: **nothing in either
test suite exercises them and no fixture in `tests/` is an MD job**, so
"builds and is packaged" is not "works". Someone should open one by hand
before step 3.

**`mdtools` is only half code-agnostic, and this is the main correction
to §2's optimism.** The panels are clean — `ControlPanel.C` contains
*zero* references to NWChem — but `MDEdBase.C` contains fifteen, and
they are not comments. It includes `dsm/NWChemMDModel.H`, its member
`p_model` is constructed as `new NWChemMDModel(panels)`, and it exposes
`getNWChemMDModel()` returning a concrete `NWChemMDModel&` which
thirteen call sites use directly.

So the *mechanism* is generic — `MDEdBase.C:253` reads
`TaskInputGenerator` from the code cap and shells out to whatever script
it names, which is exactly the seam GROMACS needs — but the *model* is
hard-bound to a concrete NWChem class. Step 3 therefore includes
extracting an interface from `NWChemMDModel` (or accepting a
GROMACS-specific subclass behind a common base), and that is real
refactoring of working code, not configuration. Budget for it, and do it
under the dialog/parser suites rather than by inspection.

## 6. Still unverified

* Whether `mdprepare` can be bypassed entirely for an imported topology,
  or whether the resource graph requires a `*_md_prepare` node to exist
  before a task node can be created. This decides whether the
  import-only scope in §3.1 is actually reachable, so it is the next
  thing to check.
* Everything in this document about GROMACS itself. No binary was
  consulted.
