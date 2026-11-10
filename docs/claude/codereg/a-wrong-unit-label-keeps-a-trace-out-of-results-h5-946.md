---
type: pitfall
title: "A wrong unit label keeps a trace out of results.h5; the views never noticed (#946)"
area: codereg
paths: [scripts/parsers/gaussian-16-trace.pl, scripts/parsers/gaussian-16.ircgeom, tests/parsers/g16_units_test.py, src/tdat/chemistry/ResultsFromCalc.C]
issues: [946, 690, 883]
---
The Geometry Trace, IRC and normal-mode views ignore the `units` of
GEOMTRACE/IRCGEOM and take the numbers as Angstrom, so a script that prints
`units: Hartree` for a coordinate table (as the Gaussian 16 trace scripts did,
copied from an energy script) looks fine in the GUI. `ResultsFromCalc` does
read the label: `toBohr` knows Angstrom, bohr, nm, "NA" and empty, and any
other unit drops the trace from results.h5 without a message (the G16 IRC
example then got no file at all, having no orbitals either).

`tests/parsers/g16_units_test.py` (ctest `parsers_g16_units`) holds every G16
key's unit in a table and fails on an unlisted one; it checks the trace and
gradient tables against the .log's own "(Angstroms)" and "Forces
(Hartrees/Bohr)" tables. A new code's coordinate script needs the same care:
G09/G03 `geomtrace` still print Hartree (retired codes, left as they are).
After fixing a label, re-run `tools/examples/preparse.py` for the sets that
hold that code's examples: the stored Props and results.h5 carry the old one.
