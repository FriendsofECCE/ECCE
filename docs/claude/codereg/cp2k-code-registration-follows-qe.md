---
type: map
title: "CP2K is registered like Quantum ESPRESSO: its own dialogs, the library scanned in ai.cp2k, the cell from the Builder, results through cp2k.desc (#329)"
area: codereg
section: "Getting a calculation set up (the \"code registration\" system)"
paths: ["data/client/cap/CP2K.edml", "scripts/codereg/cp2ktheory.py", "scripts/codereg/cp2kruntype.py", "scripts/parsers/ai.cp2k", "scripts/parsers/cp2k.tpl", "scripts/parsers/cp2k.desc", "scripts/parsers/cp2k.launchpp", "scripts/gensub", "src/apps/machregister/SchedulerQuery.C", "tests/cp2k_features/run_tests.py"]
issues: [329]
---
What exists (Quickstep/GPW, ENERGY, ENERGY_FORCE, GEO_OPT; stages 1 to 4 of #329):

- **Registration.** `CP2K.edml` (`needsBasis="false"`, theory `DFT`/`GPW`, runtypes Energy and
  Geometry, `RequiresPeriodic`), `cp2k_es` in both `ResourceDescriptor*.xml` and in
  `project`'s `ContainsResource`, `sub cp2k` in `gensub` (the CONFIG key `CP2K` is the path of
  `cp2k.psmp`; `.ssmp`/`.sopt` run once with `OMP_NUM_THREADS=$totalprocs`, MPI builds under
  `mpirun -np` with one thread per rank), `.cp2kin`/`.cp2kout` in `httpd.conf.ecce` and
  `config/mimetypes`, `SchedulerQuery` (Find, names `cp2k.psmp`, `cp2k.popt`, `cp2k.ssmp`),
  `WxMachineRegister::builtinCommand`, `WxLauncher::missingMpiLauncher`, the dummy and
  localhost CONFIG examples and `siteconfig/Machines`. `cp2k.launchpp` is a no-op that must exist.
- **Basis and potential per element are chosen in `ai.cp2k`**, because a dialog is never told the
  elements. Theory Details gives the functional (which fixes the GTH family: PBE and revPBE use
  GTH-PBE), a basis level (`DZVP-MOLOPT-SR-GTH` and so on) and the library directory and file
  names; `ai.cp2k` scans `BASIS_MOLOPT` and `GTH_POTENTIALS` and takes, per element, the
  potential carrying the plain family alias (`GTH-PBE`), else the one with the most valence
  electrons for which a basis of the same `-q<n>` exists (U has `GTH-PBE-q14` and `-q32` and
  a basis for q14 only), and a basis of the chosen level with that `q` (short-range sets fall
  back to the plain ones and the reverse). Three per-element rows override either; a name
  not found in the scanned file is a warning, a missing basis a refusal that names the element.
- **File names in the deck.** Bare (`BASIS_MOLOPT`) while the directory is `/usr/share/cp2k`,
  because CP2K finds its own data directory on whatever machine runs the job; a directory
  typed in the dialog is written in full.
- **Nothing for memory**: CP2K has no input keyword for it.
- **Not yet**: CELL_OPT and its per-step cell (LATTICEVEC), MD, several kinds of one element
  (antiferromagnetic Fe up/down), `ELEMENT` kinds, k-points, hybrid functionals, restart.
- **Test**: `tests/cp2k_features/run_tests.py` (ctest `cp2k_features`) writes decks with the real
  generator, runs `cp2k --check` and the input checker on them, runs silicon and a NiO+U
  optimisation for real and compares the parsed properties with values read from CP2K's output
  by separate patterns. The fixtures of `tests/parsers/fixtures/cp2k` are those outputs;
  `tests/launch/cp2k_test.py` (ctest `launch_cp2k`) runs both through the real Launch, with the
  Organizer's progress messages, and `tests/launch/cp2k_calced_test.py` (ctest
  `launch_cp2k_calced`) opens a CP2K calculation in the real Calculation Editor, sends settings
  as the dialogs do, saves, and checks the deck with the input checker and `cp2k --check`.
  `tests/filedsi/resourceTest cp2k` makes the calculations (a structure with a lattice).
