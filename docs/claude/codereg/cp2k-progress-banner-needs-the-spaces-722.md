---
type: pitfall
title: "A [PROGRESS] pattern written with single spaces never matches a banner CP2K centres with runs of spaces (#722)"
area: codereg
paths: ["scripts/parsers/cp2k.desc", "tests/launch/progress_final_chunk_test.py"]
issues: [722, 614]
---
The live snapshot of a finished CP2K CELL_OPT lacked `converged=yes` while a
post-mode replay had it: not a monitor bug (the last lines of the output are
folded in and flushed before the final snapshot, also when the last line
arrives in the final chunk), but the run used a `cp2k.desc` from before the
pattern allowed `\s+` between `***` and the title. Check the build date of
the installed `.desc` before looking in `eccejobmonitor`.
`tests/launch/progress_final_chunk_test.py` streams a CP2K output in pieces,
the banner cut mid-line in the last one, through the live monitor.
Calculations that ran with the old rule get the flag from Re-Parse Outputs.
