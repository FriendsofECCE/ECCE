---
type: pitfall
title: "GROMACS umbrella sampling (#762): a window is the pull run with rate 0; WHAM reads tpr + pullf; the r^2 volume term is in gmx wham's PMF"
area: codereg
paths: [scripts/ecce_umbrella.py, scripts/parsers/md.gmxtask, include/tdat/InteractionModel.H, src/dsm/xml/NWChemMDModelXMLizer.C, src/dsm/edsiimpl/UmbrellaWindows.C, src/comm/commxt/Launch.C, src/comm/commxt/JobStore.C, tests/gromacs_md/umbrella_test.py, scripts/parsers/ai.gromacs, src/apps/builder/PullPanel.C]
issues: [762, 410]
---
- A window is the pull task's model with `GmxPullRate` 0 and `GmxPullInit`
  at the window's target distance (`GromacsRun::pullInit`, < 0 = unset):
  `ai.gromacs` then writes `pull-coord1-start = no` and `pull-coord1-init`.
  With `start = yes` (the pull run's own setting) grompp adds the starting
  COM distance, so a window on the barrier was restrained wherever the
  frame happened to be and slid into the well during equilibration: a gap
  at 0.35 nm in the Na+ Cl- PMF. Window targets come from `ecce-umbrella`'s
  `target` (Window::target); the frame's own `distance` may be up to half a
  spacing away and the equilibration brings it there.
- Frames: `ecce-umbrella` takes the frame times from `gmx check -f traj.xtc`
  and the distance at each frame by interpolating pullx (every 50 steps), so
  the xtc interval decides how finely windows can be placed. A target whose
  nearest frame is more than half a spacing away is skipped (the pull did
  not pass there), and one frame serves at most one target.
- `extract` cuts the whole system (group 0) with `trjconv -dump`; a pull
  run whose xtc holds only some groups would give windows without solvent.
- `gmx wham -it tpr.dat -if pullf.dat` needs, per window, its own tpr and
  pullf.xvg. When pullf.xvg is not stored, the CLI writes one from the
  PULLTIME/PULLFORCE properties (time, force: the default one-coordinate
  layout).
- gmx wham's profile along a distance coordinate includes the 4 pi r^2
  volume (entropy) term. For an ion pair the contact minimum then looks too
  shallow and the plateau never flattens; `--jacobian` adds 2RT ln r. Off by
  default so the numbers match what gmx wham alone gives (the lab's numbers);
  the two differ by several kJ/mol over 0.3 to 1.2 nm.
- Summary (`summarise`): local extrema of the 5-bin-smoothed PMF, then
  neighbouring min/max pairs closer than `--noise` (default kT) removed
  smallest-first, with the profile ends as boundaries (persistence); the
  deepest point is always listed and barriers are maxima minus it. Before
  that the ends are cut where the summed histograms hold < 1 % of the
  fullest bin (`--support`): gmx wham's profile shoots up there. JSON `pmf`
  is the cut profile, `pmf_full` the whole one (the CSV writes it with
  `supported`/`extremum`/`significant` columns). Zeroing at the end uses
  the cut profile's end, not the spike.
- Overlap check: normalised histograms of neighbouring windows (ordered by
  mean) sharing less than 5 % give a warning naming the distance.
- Window tasks (`UmbrellaWindows::create`): `MdTask::copy` would keep the
  pull task's content type and branch from the pull task's own provider, so
  windows are made as File > New does (`createChild` + `addMemberAsTarget`).
  `addMemberAsTarget` sets ResumeOpt when a Dynamics follows Dynamics; the
  -eq task resets it to draw fresh velocities. Steps are
  EquilibrationSteps + DataSteps in md.gmxtask, so EquilibrationSteps is 0.
  Input is written as the editor's Save does (TaskInputGenerator, Ready),
  without the provider clock: a window's time starts at 0.
- pullx.xvg/pullf.xvg reach `<calc>/Outputs` only because both Launch's
  filesToTransfer (when pull is on) and `JobStore::isGromacsResult` name them.
- Panel: the Pull run / Umbrella sampling pages are a wxSimplebook behind
  two toggle buttons, each page a vertical wxScrolledWindow with wrap
  sizers. With GTK's native wxNotebook the Pull run page's Export button
  stayed drawn over the docked Umbrella page. RMSD... and Radius of
  gyration... open MD Analysis (`Builder::openPropertyPanel`, the same path
  as ECCE_OPEN_PANEL) and pick `rms`/`gyrate`; MD Analysis needs GEOMTRACE,
  and without it the pull page says so under its summary (no dialog).
- Hooks: Organizer `umbrellawindows <pull-url> <spacing> <from|-> <to|-> <eq>
  <prod>`; Builder `umbrellafind`, `umbrellacreate`, `umbrellawham
  [jacobian] [noise=<kJ/mol>]`, `umbrellaexport`, `umbrellashot`, `umbrellastate`, `umbrellaanalysis rms|gyrate`.
- Test: `tests/gromacs_md/umbrella_test.py` (Na+ Cl- in TIP3P; the NPT
  equilibration holds the pair at 0.28 nm, or it can drift to 0.5 nm and
  the pull leaves no windows at contact and runs past half the box; `--quick`
  for minutes, full run about an hour on 4 threads).
