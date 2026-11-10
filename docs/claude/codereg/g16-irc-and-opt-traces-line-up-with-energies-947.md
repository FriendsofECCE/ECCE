---
type: pitfall
title: "Gaussian 16 traces need one structure per energy point: IRC TS, signed-zero repeats, TS-search energies (#947)"
area: codereg
paths: [scripts/parsers/gaussian-16.irc, scripts/parsers/gaussian-16.ircgeom, scripts/parsers/gaussian-16.ircts, scripts/parsers/gaussian-16-trace.pl, scripts/parsers/gaussian-16.energy, scripts/parsers/gaussian-16.desc, tests/parsers/g16_trace_align_test.py]
issues: [947, 946, 498, 417]
---
results.h5 keeps a trace's energies only when their count equals the
structures' (`ResultsFromCalc` `addTrace`), so an off-by-one is silent there.

- IRC: the TS point has no CURRENT STRUCTURE block. `geomtrace`/`-input`
  keep the last structure printed (`g16IrcInput`), `[IRCTS]` freezes it at
  "Start new reaction path calculation", and `gaussian-16.irc` publishes all
  IRCGEOM frames in summary order with the TS in place (the IRC panel's
  `frames == n` branch). The IRC job reorients: the TS is fitted rigidly
  onto its two neighbours (Horn quaternion), as a Link1 IRC reads it in the
  previous job's orientation. IRCGEOM therefore appears only at the end of
  the job, with IRCENERGY.
- Optimisation: the converged geometry repeated after "Optimization
  completed" can print `-0.000000` where the step printed `0.000000`; the
  duplicate test in `gaussian-16-trace.pl` compares numerically.
- TS searches (runtype "Transition State ...") got no TEVEC because
  `gaussian-16.energy` ran only for /Geo/; it now runs for both.
- The Thermochemistry block ends at the next long rule, not at "Cartesian
  Forces", so the frequency job's Forces table (the last) reaches EGRADVEC.
