---
type: pitfall
title: "An imported run with a companion file (MOPAC .mgf) found no primary output"
area: codereg
paths: ["src/dsm/edsiimpl/TaskJob.C", "tests/apps/progress_backfill_test.py"]
issues: [525]
---
`TaskJob::getDataFile(PRIMARY_OUTPUT)` on an imported calculation falls back
to "the one stored output that is not declared" because the user's file name
never matches the declared one. MOPAC imports store `ammonia.out` and
`ammonia.mgf`: two candidates, so the fallback returned the declared name and
every caller (progress backfill, output viewer) found nothing. The fallback
now drops `.mgf`/`.arc`/`.aux` companions before it counts. A code whose
import stores another companion extension needs it added there.
