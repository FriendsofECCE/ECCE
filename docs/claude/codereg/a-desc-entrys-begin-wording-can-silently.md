---
type: pitfall
title: "A `.desc` entry's `Begin` wording can silently stop matching between versions of the same code"
area: codereg
section: "Pitfalls found more than once (check for siblings)"
paths: [".desc", "Gaussian-16.expt", "gaussian-03.desc", "gaussian-09.desc", "gaussian-16.desc"]
issues: [80]
---
**A `.desc` entry's `Begin` wording can silently stop matching between
versions of the same code**, not just between different codes.
`gaussian-16.desc`'s `MULLIKEN` entry had `Begin= Mulliken atomic
charges\:`, copied from `gaussian-09.desc`/`gaussian-03.desc` — but
Gaussian 16 actually prints "Mulliken charges:" (no "atomic").
`Gaussian-16.expt` (the full-output post-hoc parser) has no Mulliken
handling of its own, so this property is *only* ever captured live
via `eccejobmonitor` — the mismatched `Begin` regex meant it was
silently never extracted, for every G16 job, ever (#80). Same
no-error/job-completes-normally shape as the other `.desc` bugs
here. Fixed with an alternation (`Mulliken (atomic )?charges\:`)
rather than assuming the new wording fully replaced the old one.
**Caveat that applies to any fix in this family**: because there's
no reparse-from-saved-output path for a live-monitor-only property,
fixing the regex does *not* retroactively recover data for
already-completed jobs parsed under the old pattern — only a fresh
run monitored under the fixed `.desc` will have the property. If a
"we fixed the parser but the old job still shows nothing" report
comes in, check whether the job actually predates the fix before
assuming it didn't work.
