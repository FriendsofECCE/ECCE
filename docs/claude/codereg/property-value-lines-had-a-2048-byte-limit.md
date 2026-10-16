---
type: pitfall
title: "Property value lines were cut at 2047 bytes, and every property after the cut was lost (ECCE-QM above ~120 functions, ORCA above ~245)"
area: codereg
paths: ["src/dsm/edsiimpl/PropertyInterpreter.C", "scripts/parsers/ecceqm.parse", "scripts/parsers/orca.mo", "src/apps/builder/MoPanel.C", "tests/parsers/propinterp_test.py", "tests/parsers/propinterp.C", "tests/apps/ecceqm_mo_large_test.py"]
issues: []
---
`PropertyInterpreter::processValues()` (eccejobstore's reader of parse-script
output) read each `values:` line with `getline(buf, 2048)`. A longer line was
truncated at 2047 bytes, mid-number, and the stream was left in the failed
state, so the rest of the batch silently disappeared. Fixed 2026-10-16: lines
of any length.

Who writes long lines: `ecceqm.parse` (a whole vector on one line, one MO's
coefficients per line), `orca.mo` (ORBENG/ORBOCC on one line),
`orca.mulliken`, `mopac.charges`, `orca.espcharge`, `atomcharges.pl`.
Thresholds: ECCE-QM's ORBENG (`%.12f`, ~17 bytes) at ~120 functions, its MO
rows (~13.5 bytes) at ~151; ORCA's ORBENG (~8.3 bytes) at ~245 MOs; atom
vectors at ~200 atoms.

What it looked like: a 156-function ECCE-QM run (benzoic acid, def2-SVP)
stored `MO` with rows=columns=156 and 152 values, `ORBENG` with 134 of 156,
and no `ORBOCC` or `MOAOORDER` at all. Opening Properties > MOs crashed the
Viewer: PropTable warned "input vector length does not match rows*columns",
ORBENG read as null, and `MoPanel::fillUI()`'s `INTERNALEXCEPTION` threw.
Note `INTERNALEXCEPTION` is **live in every build**: it is empty only under
`INSTALL`, which CMake never defines, and nothing above a property panel
catches it. The panel now shows "no orbital energies" instead.

A calculation stored before the fix keeps its cut properties; importing its
output (Outputs/*.qmout) again, or rerunning it, gives whole ones.

Tests: `tests/parsers/propinterp_test.py` (real `ecceqm.parse` + real
interpreter, 160 functions) and `tests/apps/ecceqm_mo_large_test.py` (the
9.2-stored props open without a crash; fresh ones give 156 MOs).
