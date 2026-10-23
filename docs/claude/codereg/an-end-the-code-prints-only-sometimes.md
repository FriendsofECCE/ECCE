---
type: pitfall
title: "An `End` the code prints only sometimes leaves the block open for the rest of the run: every later property is lost, progress still works (#491)"
area: codereg
paths: ["scripts/parsers/gaussian-16.desc", "scripts/eccejobmonitor", "tests/parsers/cases.py"]
issues: [491]
---
While a block is open, `JobOutputGet` feeds every line to it and tests no
`Begin`. If its `End` never comes, no later property is sent (live or at
the end, where the block is dropped with "end of file encountered while
reading parse type ..."), but `[PROGRESS]` entries still match every line,
so the Progress box looks healthy. G16's `[SOLVMODEL]` ended on
`Spheres list`, which G16 prints only for a small cavity (7 spheres yes,
26 no): every PCM job of a mid-sized molecule kept one geometry step and
nothing else. End a block on a line the code always prints right after
the data the script needs (here the rule after `Solvent :`).
`run_tests.py` flags such a block as "runaway parse type"; it only sees
the fixtures it has, so add a fixture of the larger case
(`g16-octane-cpcm`).
