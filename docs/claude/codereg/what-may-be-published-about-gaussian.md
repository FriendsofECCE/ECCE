---
type: rule
title: "Gaussian: describe ECCE's support for it; never publish a comparison of Gaussian with another program"
area: codereg
section: "Publishing"
paths: [scripts/parsers/ai.gauss16, scripts/codereg/ged16theory.py, tests/parsers/fixtures/gaussian-16, tests/qm, help/src, docs, CHANGELOG.md]
issues: []
---
Gaussian's licence restricts publishing comparisons of Gaussian with other
programs. ECCE itself contains a quantum chemistry program (ECCE-QM) and
drives several others, so the project publishes no such comparison at all
(Andy, 2026-10-13). "Published" means anything that leaves this machine:
release notes, CHANGELOG, help pages, the wiki, repository docs, issue and
pull-request text, commit messages, stored test data, videos, the manual
and papers.

**Allowed**
- What ECCE does with Gaussian 16: which keywords the input writer
  produces, which results the readers take from the output, how to set up
  `g16` (and NBO) for ECCE, and bugs in that handling.
- Input files ECCE writes for Gaussian, and output files from our own small
  runs kept as reader fixtures (`tests/parsers/fixtures/gaussian-16`,
  `tests/jobstore`), used only to check that ECCE reads them.
- Gaussian keyword names, with a link to Gaussian's own documentation
  instead of copied text.

**Not allowed**
- Any statement that results, accuracy, speed or behaviour of ECCE-QM or
  another code agree with, differ from or were "checked against" Gaussian,
  with or without numbers.
- Timings or benchmarks of Gaussian.
- Gaussian reference numbers stored in the repository, or test output,
  logs or tables showing how another program's results compare with
  Gaussian's.
- Gaussian binaries, and text copied from its manuals.

**Testing is fine.** Tests may run `g16` live and hold ECCE-QM or another
code to its results (`tests/qm/charges_test.py` does, for Merz-Kollman
charges). What is published is the test, not the comparison: the outcome
stays on the machine that ran it and is not quoted anywhere public.

Before a release, grep new public text for Gaussian or G16 near "against",
"agree", "within", "compared" or "checked".
