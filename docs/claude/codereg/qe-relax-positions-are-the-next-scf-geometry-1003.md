---
type: pitfall
title: "QE relax / vc-relax: an ATOMIC_POSITIONS block is the geometry of the NEXT SCF, so GEOMTRACE, LATTICEVEC and TEVEC are published together by qe.energy (#1003)"
area: codereg
paths: [scripts/parsers/qe.desc, scripts/parsers/qe.energy, scripts/parsers/qe.geomtrace, scripts/parsers/qe.latvec, scripts/parsers/qe.version, tests/parsers/qe_trace_align_test.py]
issues: [1003]
---
pw.x prints one `!` energy per SCF including the starting geometry, and the
`ATOMIC_POSITIONS` (and `CELL_PARAMETERS`) printed after an SCF belong to the
next one; the last block repeats the final geometry. A parser script sees one
block, so `qe.geomtrace` / `qe.latvec` stash their record in
`$key/qeStash.*` and `qe.energy` (the `!` entry) publishes it with that
energy plus TEVEC. n energies give n-1 frames; the starting geometry is not a
frame and the final repeat is dropped unless a final SCF follows (vc-relax).
`qe.version` clears stale stashes at the start of a run. md (runtype
`Dynamics`) publishes at once, with `qe.mdstep`'s own vectors.

`tests/parsers/run_tests.py` replays `qe.desc` in file order (`by_file`) for
this reason, and the relax cases list `silent_ok` for the stash-only entries.
