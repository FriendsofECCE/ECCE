---
type: checklist
title: "A `.desc` parse-type's `Begin` value is also its hash key, AND its match priority"
area: codereg
section: "Added from integrating MOPAC (issue #86) — the second code through"
paths: [".desc", "scripts/eccejobmonitor"]
---
**A `.desc` parse-type's `Begin` value is also its hash key, AND its
match priority** — `scripts/eccejobmonitor`'s `PDFileRead()` keys its
whole parse-type table by the literal `Begin` string (`$parseHandle =
$pdBuf{$PD_KEY_BEGIN}`), *silently* dropping any later entry whose
`Begin` collides with one already read ("silently ignore duplicate
parse handles" — no error, no log). Making two entries' `Begin`
values textually different but functionally identical (e.g. wrapping
one in a no-op `(?:...)`) avoids that collision, but creates a worse
problem if both are meant to match the *same real line*:
`PDMatchBegin()` returns the first parse type whose `Begin` matches a
given line and never checks the rest for that line, so whichever
entry happens to be checked first **permanently starves the other of
ever matching, for the entire run** (confirmed live: one matched
10/10 real occurrences, the other matched zero). If a code's output
has only one marker phrase serving two logical purposes (e.g. "the
converged energy" vs "energy at every optimization step", where
NWChem happens to have two distinct marker phrases for these but not
every code does) — don't split them into two `.desc` entries at all.
Use one combined `[KEY1][KEY2]` entry at `Frequency=all`, and have
the script unconditionally emit both keys on every invocation; a
plain overwriting value naturally ends up holding the last (e.g.
converged) value once matching stops, while a step-vector-typed key
accumulates the full per-step trace from the same invocations.
