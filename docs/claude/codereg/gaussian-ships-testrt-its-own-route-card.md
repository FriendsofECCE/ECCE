---
type: rule
title: "Gaussian ships `testrt`, its own route-card parser — use it in `tests/verify`, never in the shipped script"
area: codereg
section: "The input checker (`scripts/parsers/verifyinput`, #148)"
paths: ["tests/verify"]
issues: [148]
---
**Gaussian ships `testrt`, its own route-card parser — use it in
`tests/verify`, never in the shipped script.** ECCE submits to
remote machines, so the checking client is the machine least likely
to have Gaussian installed; a check that degrades to UNSURE on most
installs is not a check (Andy, 2026-09-25). In the suite it is the
oracle the reverse-engineered route rules are validated against, and
it skips when absent. It rejects unbalanced parentheses and stray
characters, and **accepts `Freq=()`** — which ECCE emits on nearly
every deck, so a paren rule written from intuition would have
condemned the whole corpus. Compare only route-card findings with
it; a broken basis block has a perfectly good route card.
