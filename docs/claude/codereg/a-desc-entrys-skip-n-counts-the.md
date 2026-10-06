---
type: checklist
title: "A `.desc` entry's `Skip=N` counts the `Begin`-matching line itself"
area: codereg
section: "Added from integrating MOPAC (issue #86) — the second code through"
paths: [".desc", "scripts/eccejobmonitor"]
issues: [86]
---
**A `.desc` entry's `Skip=N` counts the `Begin`-matching line itself**,
not N lines *after* it (`scripts/eccejobmonitor`'s Begin/Skip/End
line-feeding algorithm decrements `lineSkip` starting from the Begin
match's own iteration). Getting this off by one leaves whatever
separator/dashed/blank line immediately follows `Begin` as the
*first* line fed to the parse script — harmless for a script that
tolerantly skips non-matching lines, but silently produces **zero**
output, every single invocation, for any script that does `if
(matches) {...} else { last }` on its first read (a common, natural
pattern for "read atom rows until the shape changes"). This was the
root cause of ORCA's `GEOMTRACE`/`VIBFREQ` properties never
extracting through the real monitor pipeline despite their `Begin`
regex matching correctly in isolation — found only by simulating
eccejobmonitor's actual algorithm against real captured output, not
by hand-testing the parser script with manually-picked line ranges
(which is exactly what let it hide through an earlier live-debugging
session). If a new `.desc` entry's script produces no output despite
a confirmed-matching `Begin`, simulate the real Skip/End feed before
suspecting the regex or the script's own logic.
