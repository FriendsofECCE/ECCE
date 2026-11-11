---
type: pitfall
title: "GROMACS umbrella sampling (#762): a window is the pull run with rate 0; WHAM reads tpr + pullf; the r^2 volume term is in gmx wham's PMF"
area: codereg
paths: [scripts/ecce_umbrella.py, src/dsm/edsiimpl/UmbrellaWindows.C, src/comm/commxt/Launch.C, src/comm/commxt/JobStore.C, tests/gromacs_md/umbrella_test.py, scripts/parsers/ai.gromacs, src/apps/builder/PullPanel.C]
issues: [762, 410]
---
- `ai.gromacs` always writes `pull-coord1-start = yes`. With rate 0 grompp
  adds the starting COM distance to `pull-coord1-init`, so a window made
  from a pull-trajectory frame is restrained at its own distance and the
  value is in its `topol.tpr`. No reference-distance setting exists or is
  needed; a window is the pull task's model with `GmxPullRate` 0.
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
- Hooks: Organizer `umbrellawindows <pull-url> <spacing> <from|-> <to|-> <eq>
  <prod>`; Builder `umbrellafind`, `umbrellacreate`, `umbrellawham
  [jacobian]`, `umbrellaexport`, `umbrellashot`, `umbrellastate`.
- Test: `tests/gromacs_md/umbrella_test.py` (Na+ Cl- in TIP3P; `--quick`
  for minutes, full run about an hour on 4 threads).
