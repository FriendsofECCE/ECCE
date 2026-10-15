> **Status: experimental.** GROMACS MD studies are in ECCE as an experimental feature.

# GROMACS in ECCE: how it fits together

This is the page for someone who will change the GROMACS support. For why
it is shaped this way (and what was rejected), see `GROMACS_ROADMAP.md`
and issue #106.

## What a user does

1. **New GROMACS MD Study...** in the Organizer (right-click a project).
2. In the study, **New GROMACS MD Optimize / Energy / Equilibrate /
   Dynamics...**. Any of them can be the first task; each new task follows
   the last one.
3. Open a task (double-click). The first tab, **Inputs**, takes the system:
   a topology (`.top`), a starting structure (`.gro`) and any include files
   (`.itp`, `.ndx`) the topology reads. These come from `pdb2gmx`,
   CHARMM-GUI or a collaborator: ECCE does not build topologies yet (see
   "Step 2" at the end). A task without its own files uses those of the
   nearest earlier task, so they are attached once. **Launch stays
   disabled until a topology and a structure are found**, and says which is
   missing.
4. The other tabs are the NWChem MD editor's, with the controls that mean
   nothing to GROMACS hidden: **Interactions** (PME or plain cutoff,
   cutoff radius, pair-list interval), **Constraints** (bonds to hydrogen),
   **Optimize** (steepest descent) or **Dynamics** (time step, steps,
   thermostat, barostat, annealing), **Files** (trajectory and log
   intervals).
5. **Launch** works as for any code: locally or on a registered machine.
   The machine's **Codes** tab lists **GROMACS**; its *Program* is the full
   path to `gmx` (**Find** looks for `gmx`). A site whose GROMACS is an MPI
   build gives `gmx_mpi`; the job script then starts `mdrun` under
   `mpirun -np <processors>`.
6. Results: **TE** (total energy; the potential energy for a
   minimisation), **PRESSURE** and the per-step traces **TEVEC**,
   **TEMPVEC** and **PRESSVEC** (Builder, *Geometry Step Plots*), and the
   final structure, which the Builder shows and the next task starts from.

A study is a chain: Optimize, Equilibrate (NVT), Equilibrate (NPT),
Dynamics. A task that continues a dynamics run ticks *Continue from the
previous task's structure and velocities* (`continuation = yes`, no new
velocities); any other dynamics task makes new velocities at its
temperature.

## The pieces

| piece | where | does |
|---|---|---|
| resource types `gromacs_md_study`, `gromacs_md_{energy,optimize,equilibrate,dynamics}` | `data/client/config/ResourceDescriptor.xml` **and** `ResourceDescriptorRxn.xml` (keep in step) | what the New menu offers; application type `GROMACS` |
| code registration | `data/client/cap/GROMACS.edml` | data files, `TaskInputGenerator`, parse spec, launch preprocessor |
| model | `include/dsm/MDCompositeModel.H` (seven sub-models, shared), `NWChemMDModel`, `GromacsMDModel` | the values the panels edit; same XML for both codes |
| editors | `src/apps/mdenergy`, `mdoptimize`, `mddynamics` (one editor serves Equilibrate and Dynamics) on `src/wxgui/mdtools/MDEdBase` | chooses the model by the task's application type, hides controls (`applyGromacsProfile` in each panel, helpers in `GromacsProfile`) |
| Inputs tab | `src/wxgui/mdtools/GromacsInputsPanel.C` | attaches, lists and removes the system's files; reads the `.gro` (`GromacsStructure`) so the Builder can show it |
| `.mdp` | `scripts/parsers/md.gmxtask` (model XML to `.param` keys) and `ai.gromacs` + `gromacs.tpl` (the `.mdp` text) | one place per concern: the mapping is in `md.gmxtask`, `.mdp` syntax in `ai.gromacs` |
| launch | `src/comm/commxt/Launch.C`, `scripts/parsers/gromacs.launchpp` | stages the inputs under fixed names (`gromacs.mdp`, `topol.top`, `conf.gro`, includes); `launchpp` runs `grompp` on the launching machine when `gmx` is there, so a mistake stops the launch and `grompp`'s own words are shown |
| job script | `sub gromacs()` in `scripts/gensub` | `grompp`, `mdrun`, `editconf` on the machine that runs the job; a failing `grompp` is copied into the output file |
| results | `scripts/parsers/gromacs.desc`, `gromacs.energy`, `gromacs.null` | frames the log's energy blocks and publishes the properties |

### Files and their types

| file | stored as | type (`httpd.conf.ecce` and `data/client/config/mimetypes`) | in the run directory |
|---|---|---|---|
| run control | `gromacs.mdp` | `chemical/x-gromacs-mdp` | `gromacs.mdp` |
| topology | `<name>.gmxtop` | `chemical/x-gromacs-topology` | `topol.top` |
| starting structure | `<name>.gro` | `chemical/x-gromacs-coordinates` | `conf.gro` |
| include files | as named | `chemical/x-gromacs-include` (`.itp`, `.ndx`) | as named |
| output | `gromacs.gmxlog` | `chemical/x-gromacs-log` | `gromacs.log` (mdrun insists on `.log`; the declared name is a symlink while it runs, a copy after) |
| final structure | `ecceSession_<calc>.gro` and `chemsys.pdb` | `.gro` as above; `chemical/x-pdb` | |

**Why `.gmxtop`.** `.top` is NWChem's topology type
(`chemical/x-nwchem-topology`) and a second `AddType` for it would change
what NWChem MD files resolve to. A GROMACS topology is therefore stored
under its own extension; the Inputs tab renames on attach and the job sees
`topol.top`. The final structure's name comes from `MdTask::getRestartName`,
which the launch code, the launcher and the job script all use; the PDB
name is `VDoc`'s constant `chemsys.pdb`.

### What the model values become in the `.mdp`

Values the editor does not show are not read. Units: the model holds nm, ps,
K, kJ/mol, Pa and 1/Pa; only the last two are converted (to bar and 1/bar).

| editor | `.mdp` |
|---|---|
| Interactions: PME / Cutoff only | `coulombtype = PME` / `Reaction-Field` (`epsilon-rf = 0`) |
| Interactions: cutoff radius | `rcoulomb`, `rvdw` |
| Interactions: B-spline order | `pme-order` |
| Interactions: pair-list update interval | `nstlist` |
| Constraints: on | `constraints = h-bonds` |
| Optimize: maximum steps, initial step, maximum force | `nsteps`, `emstep`, `emtol` (integrator is always `steep`) |
| Dynamics: time step | `dt`; `nsteps` is equilibration steps plus data steps |
| Dynamics: initial time | `tinit` |
| Dynamics: remove centre of mass motion, interval | `comm-mode`, `nstcomm` |
| Dynamics: constant temperature, temperature, relaxation time | `tcoupl = v-rescale`, `ref-t`, `tau-t` |
| Dynamics: annealing | `annealing = single`, `annealing-time`, `annealing-temp` |
| Dynamics: constant pressure, pressure, relaxation time, compressibility | `pcoupl = C-rescale`, `ref-p`, `tau-p`, `compressibility` |
| Dynamics: continue | `continuation = yes`, `gen-vel = no`; otherwise `gen-vel = yes`, `gen-temp` |
| Files: write trajectory, interval | `nstxout-compressed` (`traj.xtc`, left in the run directory) |
| Files: write energies, interval | `nstlog`, `nstenergy` (default: about a hundred over the run) |

Not mapped, hidden: NWChem's parameter sets, polarization, Ewald
convergence and FFT options, dual-range cutoffs, SHAKE tolerances and
fixed atoms, conjugate gradient, recentring, split relaxation, velocity
rethermalization, volume adjustment, the whole **Control** tab (load
balancing, processor layout), solute/solvent file selections, and
Thermodynamics (PMF). Fixed atoms and position restraints need an index
file or a `define`; they belong with step 2.

Defaults a GROMACS task starts from (`GromacsMDModel::setGromacsDefaults`):
PME, steepest descent with 500 steps / 0.01 nm / 100 kJ/mol/nm, a 2 fs time
step, 5000 data steps, a 300 K thermostat, centre of mass removal on.
`MDCompositeModel::reset()` puts them back after a reset.

**An XML detail that bites.** The model's XML writer leaves out a value
equal to NWChem's default, and a reader fills it in from *its* defaults. A
default that differs between the codes would therefore reload wrongly for a
value set to exactly NWChem's. For a GROMACS task the writer writes the
optimize values that have GROMACS defaults (`dumpOptimize(..., all)`); any
new GROMACS-specific default needs the same, or has to stay equal to
NWChem's.

## How to add an `.mdp` option, end to end

Example: expose `nstcalcenergy`.

1. **Model.** If an existing control can carry it, use that. Otherwise add
   the field to the generic sub-model in `src/tdat/nwmdtask/` (and
   `include/tdat/`), with a default, a getter and a setter, and in its
   `reset()`.
2. **XML.** `NWChemMDModelXMLizer::dump<Panel>` writes it and `deserialize`
   reads it. Write it unconditionally (or see the XML detail above).
3. **Control.** Add it to the panel (`src/wxgui/mdtools/*Panel*.C`; the GUI
   file is generated by wxDesigner, `*GUI.C` and the `.pjd`), and make
   `applyGromacsProfile(bool)` show it for GROMACS and hide it otherwise if
   NWChem has no use for it.
4. **Mapping.** `md.gmxtask`: read the value with `get("<Section>", "<Key>",
   <default the model would have omitted>)` and put it in `%p` under an
   `ES.Runtype.MD.*` or `ES.Theory.MD.*` key.
5. **Text.** `ai.gromacs`: a sub that returns the `.mdp` lines from that key,
   and its `##tag##` in `gromacs.tpl`. A sub returns `""` for no line.
6. **Test.** Add the key to a stage's model in `tests/gromacs_md/run_tests.py`,
   assert the line in the `.mdp`, and assert its *effect* against `gmx`
   itself (`gmx energy`, `gmx check`), not against what you expect to have
   written. Run it.
7. **This table.**

## Running the tests

| test | needs | does |
|---|---|---|
| `tests/gromacs_md/run_tests.py` (ctest `gromacs_md`) | `gmx`, perl, python3; no build | Optimize, Equilibrate NVT, Equilibrate NPT, Dynamics on a water box: model XML to `.mdp`, `launchpp`, the real `gensub` script run as the Shell queue runs it, the real `eccejobmonitor` and parsers, checked against `gmx energy` |
| `tests/gromacs_md/groconv_test.py` (ctest `gromacs_structure`) | `gmx`, the build | the Inputs tab's `.gro` reader against `gmx editconf` |
| `tests/parsers/run_tests.py` | perl, python3 | the log parser on two real runs |
| `tests/e2e/run_tests.py` | the codes | a real run through the monitor and parsers |
| `tests/launch/gromacs_test.py` (ctest `launch_gromacs`) | `gmx`, the build, Xvfb, apache not needed (local data) | a study made through the classes, the real editors on a private X server (attach, save, screenshots), real launches, results |

Without `gmx` the first two exit 77 and CTest reports *skipped*. Set
`ECCE_GROMACS_REQUIRE=1` to make that a failure (CI does).

**Linux.** `apt install gromacs` (Debian: `gmx` at `/usr/bin/gmx`), then
`tests/gromacs_md/run_tests.py -v`; from a build, `ctest -R gromacs`.

**macOS.** `brew install gromacs` puts `gmx` on the path
(`/opt/homebrew/bin` on Apple silicon); then run
`tests/gromacs_md/run_tests.py -v`. It needs only perl and python3, both on
the system. The suite uses `~/.cache` for scratch and `sh` for the job
script, and nothing Linux-specific. `--keep` leaves the scratch directory
(printed) for inspection: one directory per task with the `.mdp`, the job
script, `grompp.out` and the log.

Writing a failing test first is easiest by copying a stage in
`STAGES`: a stage is only a model and the structure it starts from.

## MINFF mineral topologies (#241, step 1)

`scripts/minff_topology.py` builds a GROMACS system for a mineral with the
[MINFF](https://github.com/mholmboe/minff) force field. It has no wx
dependency; the windows will only collect its options and show its summary.

```
minff_topology.py build CELL -o OUTDIR [--replicate NA NB NC]
    [--substitute FROM TO COUNT]... [--min-distance 5.5] [--seed N]
    [--variant gminff|tminff] [--mineral NAME] [--angle-k 0|250|500|1500]
    [--water opc3|opc|spce|tip3p|tip3p-fb|tip4p-fb|tip4pew]
minff_topology.py minerals [--angle-k K]     # the tailored (TMINFF) minerals
minff_topology.py fetch                       # download min.ff, print where
```

`CELL` is a `.pdb`, `.gro` or `.cif` unit cell (anything atomipy's
`import_auto` reads). `--substitute Al Mgo 4` replaces 4 atoms of type (or
element) `Al` by type `Mgo`, at least `--min-distance` angstrom apart;
`--seed` makes it repeatable. OUTDIR gets `min.itp` (the mineral molecule,
`MIN`), `conf.gro` (with the box), `topol.top`, `minff.json` (atoms, counts
per type, total charge, `untyped_atoms`, `types_missing_from_ff`, variant),
and a copy of `min.ff/` so the directory runs on any machine. For
`--variant tminff` also `min_bonded.itp`.

**Pieces.** atomipy assigns types by nearest neighbours and computes the
oxygen charges for the particular structure (so an Al-to-Mg substitution
changes the oxygens around it); its `write_itp` writes the bonds and
angles, with the angle constant `--angle-k` written into every O-M-O
angle. `topol.top` then selects the parameters the way min.ff expects:

* GMINFF: `#define GMINFF_k<k>` and `#include "min.ff/forcefield.itp"`.
* TMINFF: `#define <Mineral>_k<k>` and the tailored
  `min.ff/ffnonbonded_tminff_k<k>.itp` in place of the general file that
  `forcefield.itp` includes. `ffbonded.itp` lists bond and angle types a
  single mineral block does not define, which `grompp` rejects, so
  `min_bonded.itp` is `ffbonded.itp` restricted to the types of the block.
* both: the water model's `-D` name (`OPC3`) and its ion set
  (`OPC3_HFE_LM`), then `ions.itp` and the water `.itp`.

These are `#define`s in the topology, not `define =` in the `.mdp`, so the
`.mdp` need not know the variant. **The `.mdp` does need
`periodic-molecules = yes`**: the mineral is one molecule bonded across the
box, and without it `mdrun` stops with "inconsistent shifts" / a domain
decomposition error.

**min.ff** is not in this repository (the minff repository has no licence
yet). It is fetched at a pinned commit (`MINFF_COMMIT` in the script) into
`~/.cache/ecce/minff/<commit>` (`$XDG_CACHE_HOME` is honoured);
`ECCE_MINFF_DIR` points at an existing checkout instead.

**atomipy** is found through `ECCE_ATOMIPY_PYTHON` (a python that has it),
else `python3`; without it the script says `pip install atomipy`.

**Tests**: `tests/minff/run_tests.py` (ctest `minff`; exit 77 = skipped
without atomipy, `gmx` or network for min.ff). It builds kaolinite (GMINFF,
k=500), the montmorillonite layer of the authors' hydrated example
`Systems/conf/preem27.gro` (TMINFF; `UC_conf` has none) and pyrophyllite
with 4 Al-to-Mg substitutions. Types, charges and total charge are compared
with atomipy's own pipeline run directly (`oracle.py`), the montmorillonite
also with the authors' MATLAB-made `Systems/itp/min27.itp`; each system
then goes through `grompp` and 200 steepest-descent steps, and the
potential energy must be finite and lower at the end. Charged systems
(montmorillonite -48, substituted pyrophyllite -4) give grompp's net-charge
warning, which the test allows (counter-ions are a later step).

* **Linux:** `pip install atomipy` (a venv is fine; point
  `ECCE_ATOMIPY_PYTHON` at its `python`), `apt install gromacs`, then
  `python3 tests/minff/run_tests.py` or `ctest -R minff` from a build.
* **macOS:** `brew install gromacs`, `python3 -m pip install atomipy`
  (use a venv if the system Python refuses; set `ECCE_ATOMIPY_PYTHON`),
  then the same command. The test needs only the standard library of the
  python that runs it.

## Step 2 (not built): Prepare

Everything above takes a system as given. A Prepare task would build one
and, like NWChem's, be the first task of a study (`RootTask`) and the
provider of the topology and structure the tasks after it find through the
same search the Inputs tab uses today (`TOPOLOGY_OUTPUT`, `RESTART_OUTPUT`
on an earlier task). It needs:

* a resource type `gromacs_md_prepare` and a `PrepareInputGenerator` in
  `GROMACS.edml`; the Prepare editor (`src/apps/mdprepare` is 8400 lines
  around NWChem's segment model and is not the thing to extend: a small
  GROMACS-only editor over these steps is);
* `pdb2gmx` with a **force-field picker**: the installed force fields are
  the directories `share/gromacs/top/*.ff` (15 with 2025.2; `GMXLIB`
  adds more), each with a `forcefield.doc` title line, and a water model
  chosen from that force field's `watermodels.dat`;
* `editconf` (box type and distance), `solvate`, `genion` (a neutralising
  concentration, which runs `grompp` first and needs a replacement
  group), each as a step with its output shown, and a place to keep the
  intermediate `.gro`, `.top` and `.itp`;
* the topology's `#include`s of force-field files: those resolve against
  GROMACS's own `GMXLIB`, so only user-written `.itp` files are attached;
  a force field kept outside `GMXLIB` is a directory and needs the
  Inputs tab to take a folder;
* position restraints and index groups for fixed atoms and for
  temperature-coupling groups other than `System`, with the Constraints
  panel growing a selection that writes an index file;
* the trajectory: `traj.xtc` is written and left in the run directory.
  Showing it in the Builder means a GROMACS reader beside
  `TrajectoryReaderNWChem` and `TrajectoryReaderXyz` and a way for the
  panel to find the file; the multi-frame `.gro` that
  `gmx trjconv -o traj.gro` writes is the cheapest source (it is text,
  and `GromacsStructure` already reads a frame of it).
