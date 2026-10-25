---
type: pitfall
title: "File= properties (GEOMTRACE, MDMSD, MDSPEED) are stored only after the whole output has been parsed; a long GROMACS log delays them by minutes"
area: codereg
paths: [scripts/eccejobmonitor, src/comm/commxt/JobStore.C, scripts/parsers/gromacs.desc, tests/apps/examples_test.py]
issues: [544]
---
- The monitor sends File= entries (`gPdTableFiles`) in `JobFilesGet`, after
  the job output is read; the job store parses and stores them in its
  final file phase. Until then a calculation holds only the properties of
  its output blocks.
- The NaCl-dissolving example's prod.log has 10001 energy frames: its job
  store needs about 6 minutes (beside the other example imports) before GEOMTRACE, MDMSD and the `*AVG`
  properties appear. Stopping ECCE before that (SIGTERM, exit 5) leaves
  the calculation without them, and it looks like a lost property.
- #544 was this: `examples_test.py` declared the import done after 20 s
  without a Props change and stopped the gateway. It now waits for every
  expected property. The string-vector path itself stores three records of
  ~1250 characters whole (`tests/parsers/propinterp_test.py`).
