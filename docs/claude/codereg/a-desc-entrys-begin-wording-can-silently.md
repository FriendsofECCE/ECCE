---
type: pitfall
title: "A `.desc` entry's `Begin` wording can silently stop matching between versions of the same code"
area: codereg
section: "Pitfalls found more than once (check for siblings)"
paths: ["scripts/parsers/gaussian-16.desc", "scripts/parsers/gaussian-09.desc", "scripts/parsers/gaussian-03.desc", "scripts/parsers/Gaussian-16.expt", "scripts/eccejobmonitor"]
issues: [80]
---
**A `.desc` entry's `Begin` wording can silently stop matching between
versions of the same code**, not just between codes. Gaussian 16 prints
"Mulliken charges:" where 03/09 printed "Mulliken atomic charges:", and
the G16 `MULLIKEN` entry copied from them never matched. `Gaussian-16.expt`
has no Mulliken handling, so the property is captured only live by
`eccejobmonitor`, and it was simply absent for every G16 job. Match both
wordings (`Mulliken (atomic )?charges`) rather than assuming the new one
replaced the old.

A live-monitor-only property has no reparse from saved output: a `.desc`
fix does not recover it for jobs that ran before the fix, only for new
runs. Check a job's date before concluding a fix did not work.
