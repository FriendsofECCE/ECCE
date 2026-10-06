---
type: checklist
title: "`scripts/gensub` needs a `sub <lccode>()` per code"
area: codereg
section: "Added from integrating MOPAC (issue #86) — the second code through"
paths: ["scripts/gensub"]
issues: [86]
---
**`scripts/gensub` needs a `sub <lccode>()` per code**, or the job
simply cannot be launched — `$fct = $lccode` is a bare dispatch on
the lowercased application type. Easy to miss, since it isn't in the
per-code file set.
