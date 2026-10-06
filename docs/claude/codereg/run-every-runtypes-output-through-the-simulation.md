---
type: checklist
title: "Run *every* runtype's output through the simulation, not just the richest one"
area: codereg
section: "Added from integrating MOPAC (issue #86) — the second code through"
paths: ["scripts/parsers/mopac.desc", "tests/parsers"]
issues: [86]
---
**Run *every* runtype's output through the simulation, not just the
richest one.** MOPAC prints `FINAL HEAT OF FORMATION =` for an
optimization but `HEAT OF FORMATION =` for a FORCE job, so a
Vibration-only job extracted no energy at all — invisible in the
optimization output, which looks perfect.
