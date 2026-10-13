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
pull-request text, commit messages, test data and test code, videos, the
manual and papers.

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
- Reference values computed with Gaussian stored as the expected answer of
  a test of another program (test oracles come from ORCA, NWChem or another
  program whose terms allow it). Running `g16` privately, on this machine,
  to look at a question is fine; the outcome is not written down in the
  repository or anywhere public.
- Gaussian binaries, and text copied from its manuals.

Before a release, grep new public text for Gaussian or G16 near "against",
"agree", "within", "compared" or "checked".
