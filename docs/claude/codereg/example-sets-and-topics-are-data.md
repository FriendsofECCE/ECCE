---
type: map
title: "Example sets and topics are folders plus set.txt; the Organizer shows Examples/<Set>/<Topic>/<calc>"
area: codereg
paths: ["data/client/examples", "src/apps/organizer/CalcMgr.C", "tools/examples/preparse.py", "tests/apps/examples_test.py", "tests/apps/examples_upgrade_test.py"]
issues: [447, 485]
---
`data/client/examples/<set>/set.txt`: line 1 the set's name (the tree shows
it with `_` for spaces), line 2 one line of description, then one
`folder|Topic name` line per topic. An example is a folder
`<set>/<topic folder>/<code>-<name>/` with the output and `stored/`; an
example directly in `<set>/` has no topic. `CalcMgr::exampleSets()` scans the
disk (output = first `.qmout`, `.eprint`, `.out`; a `gromacs-*` folder is a
GROMACS run); only the calculation's note (`kNotes`, keyed by calculation
name) and the label code names still live in `CalcMgr.C`.

- Project names allow no spaces: Examples/Getting_Started/..., topic names too.
- A new example or topic is data only (plus a `kNotes` entry), then
  `tools/examples/preparse.py <set>`, and the counts in
  `examples_test.py` / `examples_preparsed_test.py`.
- `kExampleSetVersion` is also the version of the layout. Version 9 moved
  from flat `Examples`, `Examples_Organic`, ... projects; `removeOldExamples()`
  deletes only the shipped-named calculations of a project that carries an
  older `ecce:exampleSet` marker, and the first start after an upgrade imports
  the sets again (pref `Organizer.ExampleSet`). Calculations of the user's own
  stay.
- Set ids are the folder names (`getting-started` is the first-start set);
  the `examples SET` test command and `examples_arm.py` use them.
