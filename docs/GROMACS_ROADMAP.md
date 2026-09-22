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

## 6. Spike: what was actually run, 2026-09-22

GROMACS 2025.2 was installed, so the guesses above were checked against
the real program rather than left as documentation claims. A complete
chain ran successfully on this machine:

```
gmx solvate -cs spc216.gro -box 2 2 2 -o box.gro      # 221 SPC waters
gmx grompp -f em.mdp -c box.gro -p topol.top -o em.tpr
gmx mdrun -deffnm em            # Steepest Descents converged in 5 steps
gmx grompp -f md.mdp -c em.gro -p topol.top -o md.tpr
gmx mdrun -deffnm md            # 500-step NVT, v-rescale, PME, h-bond constraints
```

**Confirmed, previously inferred:**

* `grompp` really does consume `.mdp` + `.top` + `.gro` and emit a binary
  `.tpr` (11 KB here) which `mdrun` then executes. The two-step shape in
  §3.2 is right, and `<LaunchPreprocessor>` is the place for `grompp`.
* Force fields are enumerable by directory scan: 15 of them in
  `/usr/share/gromacs/top/*.ff`. So the §3.1 option 2 (a force-field
  picker) is mechanically straightforward; the hard part is the topology
  model, not discovering the list.

**Better than expected — monitoring:**

`mdrun`'s `.log` is plain text and unusually well suited to
`eccejobmonitor`'s Begin/End mechanism, more so than NWChem's
`ecce_print` trace or MOPAC's blocks:

```
           Step           Time
              2        2.00000

   Energies (kJ/mol)
        LJ (SR)   Coulomb (SR)   Coul. recip.      Potential Pressure (bar)
    2.32600e+03   -9.67730e+03    1.85069e+02   -7.16623e+03    5.66364e+03
```

And the units are already known to ECCE: `EnergyConverter` has
`kJoule/Mole`, so unlike Quantum ESPRESSO — whose Rydberg has no entry at
all and must be converted by every parser — GROMACS energies can be
published as they come.

`scripts/parsers/gromacs.energy` is a working proof of that, extracting
TE and TEVEC from a real block. It is deliberately **not reachable from
the GUI**: there is no `GROMACS.edml`, so no calculation can be created
for it. It exists to prove the monitoring path, and to carry the two
format traps below in its comments.

**Worse than expected — two format traps, both found by running it:**

1. **The energy block WRAPS.** GROMACS prints five terms per row, so a
   minimisation has one header/value pair and a dynamics run has two:

   ```
        LJ (SR)   Coulomb (SR)   Coul. recip.      Potential    Kinetic En.
    1.37191e+03   -1.02702e+04    1.01441e+02   -8.79686e+03    1.45954e+03
   Total Energy  Conserved En.    Temperature Pressure (bar)
   -7.33732e+03   -8.13312e+03    3.01028e+02   -8.59611e+02
   ```

   The first version of the parser stopped after the first value row. It
   still found `Potential`, so it still produced a plausible energy for a
   dynamics run — while never seeing `Total Energy`, the value it says it
   prefers. Silently returning the wrong quantity: this codebase's single
   most repeated failure shape. Caught only by running an actual NVT job.

2. **Which terms appear depends on the run.** A minimisation has no
   kinetic energy or temperature; a thermostat adds `Conserved En.`; PME
   adds `Coul. recip.`. Columns must be located by NAME, and the header
   is fixed-width rather than whitespace-delimited (`LJ (SR)` and
   `Coul. recip.` both contain spaces), so it is split on 15-character
   fields.

**Difficulty estimate, now grounded:** the *monitoring and input
generation* halves are comparable to the ORCA and MOPAC integrations —
a `.desc`, a handful of parse scripts, an `.mdp` generator and a
`grompp` launch preprocessor. What makes GROMACS materially harder than
those is not the plumbing but §3.1: the topology and force-field model,
which has no counterpart in ECCE's data model at all, and §5's finding
that `MDEdBase` is hard-bound to a concrete `NWChemMDModel`. Import-only
scope avoids the first and still needs the second.

## 7. What is now built (2026-09-22)

Three of the four non-GUI pieces exist and are verified against real
GROMACS 2025.2. None is reachable from the GUI: there is still no
`GROMACS.edml`, which is the deliberate next step and the one that forces
the §3.1 scope decision.

| file | does | verified by |
|---|---|---|
| `scripts/parsers/gromacs.desc` | frames mdrun `.log` energy blocks | replayed through the eccejobmonitor simulation over two real runs |
| `scripts/parsers/gromacs.energy` | TE / TEVEC from a block | pinned values in `tests/parsers` |
| `scripts/parsers/gromacs.null` | swallows the AVERAGES summary | asserted by block count AND by TE's value |
| `scripts/parsers/ai.gromacs` + `gromacs.tpl` | generates the `.mdp` | grompp accepted and mdrun ran all three runtypes |
| `scripts/parsers/gromacs.launchpp` | runs grompp, surfaces its errors | tested on success and on a deliberately broken topology |

**Round trip proven end to end:** `ai.gromacs` generated a `.mdp` →
`grompp` built the `.tpr` → `mdrun` ran → `gromacs.desc` parsed the
resulting log back into TE/TEVEC. Nothing in that chain is mocked.

### Three findings that change the estimate

**1. The AVERAGES block is a trap, and needed an unusual fix.** GROMACS
ends a run with a summary whose `Energies (kJ/mol)` marker is
byte-identical to a real step's. Parsed as a step it puts a run-average
on every energy-vs-step plot and leaves scalar TE holding that average.
No `Begin` pattern separates them. The fix is a no-op `[NULL]` entry that
uses **End-line starvation deliberately** — the hazard CLAUDE.md
documents — to consume the summary's marker before the real entry is
offered it. This turned out to be an established idiom:
`gaussian-16.null` exists for exactly this purpose.

**2. Block framing is harder than it looks, and three approaches were
measured before one worked.** Anchoring on the `Step Time` header with a
blank `End` feeds nothing (there is a blank before the marker); skipping
past it fails because GROMACS interleaves messages (`Writing
checkpoint...`) so the preamble is not fixed; and a fixed line count
cannot serve both shapes — GROMACS wraps energy terms at five per row, so
a dynamics block is longer than a minimisation's, and a count generous
enough for one swallows the next step's `Begin` in the other (6 of 6
dynamics steps but 3 of 5 minimisation steps). Anchoring on the energies
marker itself is self-delimiting and immune to all of it.

**3. grompp failure is the common case and must stop the launch.** A
mismatched topology is what a new user hits first, and grompp's
diagnostics are good. `gromacs.launchpp` prints them and exits non-zero
rather than letting a job reach a queue that cannot run.

### What this does NOT change

The §3.1 problem is untouched and remains the real work: ECCE has no
concept of a force field or a topology, and `ai.gromacs` deliberately
generates only the `.mdp`. Everything above is import-only scope — the
user brings `.top` and `.gro`. Likewise §5's finding stands: `MDEdBase`
is hard-bound to a concrete `NWChemMDModel`, so wiring any of this to the
MD applications means extracting an interface from working code first.

**Revised estimate:** the monitoring and input-generation halves are
done, and were ordinary work. What remains is a GUI decision (§3.1) and a
refactor (§5), neither of which is parser work.

## 8. Still unverified

* Whether `mdprepare` can be bypassed entirely for an imported topology,
  or whether the resource graph requires a `*_md_prepare` node to exist
  before a task node can be created. This decides whether the
  import-only scope in §3.1 is actually reachable, so it is the next
  thing to check.
* Everything in this document about GROMACS itself. No binary was
  consulted.
