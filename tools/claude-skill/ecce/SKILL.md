---
name: ecce
description: Working alongside a scientist who uses ECCE (Extensible Computational Chemistry Environment) for quantum chemistry and MD — ECCE-QM, ORCA, Gaussian 16, NWChem, MOPAC, GROMACS, Quantum ESPRESSO, CP2K. Use when discussing, checking or analysing ECCE calculations, reading their stored results, comparing runs, or preparing inputs for the scientist to launch.
---

# Working with ECCE as the scientist's colleague

**Roles.** The scientist builds structures, sets up calculations, launches them and
inspects results in ECCE's GUI; the science and the decisions are theirs. You read the
same results from ECCE's stored data, check them, compare them, discuss the chemistry,
suggest what to compute next and prepare inputs when asked. Do not drive the GUI. Say
plainly when something is outside what you can judge; defer to the scientist on
computational-chemistry practice.

## Where the data is
- Local projects: `~/.ECCE-local/users/local/<project>/<calculation>/`
  (a central data server keeps the same layout under its own root).
- Per calculation:
  - `.ecce-meta` — metadata, one `ecce:<key>` per line pair, value after two tabs:
    `state` (Created, Ready, Submitted, Running, Complete, Unsuccessful, Killed …),
    `application` (code), `theory`, `runtype`, `spinmultiplicity`, `annotation`,
    `runStatusReason` (why it failed), `launch_machine`, `launch_totalprocs`,
    `launch_maxwall`, `job_path` (run directory on the compute machine), `progress`.
  - `Inputs/` — the input as launched (e.g. `g16.g16in`, `orca.inp`, `ecceqm.qmin`, `gromacs.mdp`).
  - `Outputs/` — the program's output (e.g. `g16.g16out`, `*.out`, `ecceqm.qmout`,
    `gromacs.gmxlog`) and `eccejobstorelog.ecce_run_log` (submit/monitor events).
  - `Props/` — parsed results, one XML file per property (see reference/properties.md).
  - `Parameters/` — the setup: `chemsys.mvm` (structure), basis set, code parameters.
- GROMACS studies: a project-like folder whose tasks (Prepare, Optimize, Equilibrate,
  Dynamics …) are calculations; `linkbase.xml` holds the chain.
- The run directory (`job_path`) on the compute machine holds everything the job wrote,
  including files ECCE does not bring back (e.g. ORCA `.hess`).

## Reading results
Read `Props/` first (already parsed, units in the XML), the output for anything else.
A scalar: `<value name="TE" units="Hartree">-79.227…</value>`. A vector:
`<vector name="VIBFREQ" rows="30" units="cm-1">…values…</vector>`. Tables (MO, GEOMTRACE)
are `<table …>`/time-series tables. See reference/properties.md for the common keys.

## What to check (reference/checks.md has the list)
Converged? (state, runStatusReason, the output's own convergence lines); the stationary
point is what it should be (0 imaginary frequencies for a minimum, exactly 1 for a TS);
same method/basis/solvent across a set before comparing energies; spin contamination
(S2) for open shells; basis set listed once (an 'Imported' duplicate is a known fault);
geometry chemically sensible (bond lengths, coordination, nothing dissociated).

## ECCE's own documentation
Help pages ship with ECCE (`<install>/doc/help/*.html`, sources `help/src/*.md` in the
repository): first-calculation, ecce-qm, ecce-qm-files, importing-a-calculation,
machines, register-machines, mo-diagram, localised-orbitals, mode-composition,
density-map, gromacs-*, qe-*, molecules-for-teaching, organometallics. Read the page
before explaining a feature; do not guess menu names.

## Capabilities worth knowing (check the help page if in doubt)
- Localised orbitals (sigma/pi/lone pair labels): ECCE-QM closed shell only.
- Mode composition (Vibrations > Composition...): first version; see the help page's caveats.
- MO diagram: marked experimental; check it against a textbook diagram before teaching from it.
- IRC: ORCA IRC import shows the energy path; per-point structures are being fixed.
