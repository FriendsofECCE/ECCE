---
type: map
title: "GROMACS run settings (defines, residue-name groups, barostat type, extra .mdp lines): model -> md.gmxtask -> ai.gromacs, and the index file written at launch"
area: codereg
paths: [include/tdat/InteractionModel.H, src/tdat/nwmdtask/InteractionModel.C, src/dsm/xml/NWChemMDModelXMLizer.C, src/wxgui/mdtools/GromacsRunPanel.C, scripts/parsers/md.gmxtask, scripts/parsers/ai.gromacs, scripts/parsers/gromacs.tpl, scripts/parsers/gromacs.mkndx, scripts/parsers/gromacs.launchpp, scripts/gensub, src/comm/commxt/Launch.C, tests/gromacs_md/run_settings_test.py, tests/gromacs_md/oracle_compare.py]
issues: []
---
The values are one `GromacsRun` struct in the Interaction model, saved as
`<Gmx...>` tags in the Interaction section only when they differ from the
default (all off), so NWChem's file and a default GROMACS task's `.mdp` do
not change. Free text is written with `&#10;` for newlines because
`md.gmxtask` reads the XML one line per tag.

- `md.gmxtask` turns them into `ES.Runtype.MD.{Defines,RefcoordScaling,
  GroupDefs,ComGroups,TcGroups,Barostat,PressureType,PressureInterval,
  ExtraLines}`; `ai.gromacs` writes the lines. The tc-grps repeat of
  `tau-t`/`ref-t`/annealing and the `ref-p`/`compressibility` expansion per
  coupling type are done there, where the group count is known.
- User lines are merged by `mergeExtraLines` over the file `modifyInputFile`
  wrote to `tmpfile` (not the template name, which `cleanup` renames over
  afterwards): a key replaces ours, `-`/`_`/case equal.
- The group definitions travel to the launch as `; ecce-group name = RES`
  comments in the `.mdp`. `gromacs.launchpp` runs `gromacs.mkndx` on the
  staged `conf.gro` and writes `ecce_groups.ndx` before anything else is
  checked. A `.ndx` the user attached wins and ECCE writes none. The file
  name is not `index.ndx` so that a stale ECCE file can never be taken for a
  user's. The job script (`gensub`) only passes `-n`: first any other `.ndx`
  in the run directory, else `ecce_groups.ndx`.
- Launch ships only the files it lists. `Launch::moveFiles` adds
  `ecce_groups.ndx` to that list when it exists in the local directory;
  anything else a launch-time script writes into the staging directory is
  left behind.
- grompp objects to nothing about the Berendsen barostat except one warning;
  the launch check and the job script count a `pcoupl = berendsen` line in
  the final `.mdp` into `-maxwarn`.
- A group item `mol:<name>` stands for the atoms of a `[ molecules ]` entry.
  The `.gro` has no chain column, so `gromacs.mkndx` reads the topology
  (local includes, then GROMACS's own library) and sums atom counts in
  order; it stops if the sum is not the structure's atom count.
