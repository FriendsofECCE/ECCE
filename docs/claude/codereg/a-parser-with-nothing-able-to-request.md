---
type: pitfall
title: "A parser with nothing able to request its input is dead code, and the suite will not tell you"
area: codereg
section: "Pitfalls found more than once (check for siblings)"
paths: ["orca.desc", "tests/parsers/cases.py"]
issues: [88]
---
**A parser with nothing able to request its input is dead code, and
the suite will not tell you.** `orca.desc` parsed `CHELPG Charges`
into ESPCHARGE from the day ORCA was integrated, but neither
`ai.orca` nor the runtype dialog could ever put `CHELPG` on the route
card, so the property never appeared for any job (#88). The parse
type simply never fired, which looks identical to "no fixture
exercises it". When adding extraction for a property, check that
something can *ask the code to produce it* — and when a parse type
never fires, establish which of the two it is (see
`tests/parsers/cases.py`'s `KNOWN_DEAD` vs `UNCOVERED`, and the gate
that now forces every non-firing entry to be classified).
