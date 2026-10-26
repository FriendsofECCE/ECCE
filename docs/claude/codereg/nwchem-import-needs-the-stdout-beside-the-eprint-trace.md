---
type: pitfall
title: "An imported NWChem trace (.eprint) had no progress record: the stdout beside it was never read"
area: codereg
paths: ["src/comm/commxt/JobParser.C", "src/dsm/edsiimpl/TaskJob.C", "scripts/parsers/nwchem.desc"]
issues: [558]
---
nwchem.desc's [PROGRESS] entries read NWChem's stdout, not the ecce_print
trace. `JobParser::importCalculation` used to give the monitor the trace as
both parse file and ##output##, so an import of `methane.eprint` stored only
the trace and had no progress. It now links a `.out`/`.nwout`/`.log` beside the
trace (if it is NWChem output) and passes it as ##output##; the stored primary
output is then that file, and `TaskJob::getDataFile` ignores `.eprint` when
choosing it. `tests/apps/calcimport_test.py` checks it on the Getting Started
methane example. Stored copies of examples must be regenerated with
`tools/examples/preparse.py` after such a change.
