---
type: pitfall
title: "Gaussian 16 GEOMTRACE keeps one frame per step: Standard orientation, or Input when symmetry is off (#498)"
area: codereg
paths: [scripts/parsers/gaussian-16.desc, scripts/parsers/gaussian-16.geomtrace, scripts/parsers/gaussian-16.geomtrace-input, scripts/parsers/gaussian-16.geomtrace-flush, scripts/parsers/gaussian-16-trace.pl, tests/parsers/g16_trace_frames_test.py]
issues: [498, 312]
---
Gaussian prints an Input (or Z-Matrix) block and, with symmetry on, a Standard
block for every step; routing all of them into GEOMTRACE gave 2-3 frames per
TEVEC point and the Geometry Trace click/Play showed the wrong step. The
Standard block is kept (normal modes and orbitals are in that frame, #312).
`geomtrace-input` holds the Input block in `$key/g16InputGeom`; the Standard
block drops it and publishes itself, and "Symmetry turned off" (entry
`[GEOMTRACE2]`, `geomtrace-flush`) publishes it instead. A frame equal to the
last published one (the converged geometry Gaussian repeats after
"Optimization completed", and for a following Freq) is dropped. A running job
can lead TEVEC by one frame, since the geometry prints before its energy.

The side files make the scripts order dependent: any harness must run the
blocks in file order in one directory (`tests/parsers/run_tests.py` does so
for `gaussian-16.desc`).
