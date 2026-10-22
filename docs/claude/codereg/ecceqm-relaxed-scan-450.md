---
type: map
title: "ECCE-QM relaxed scans: input block, output blocks, parser keys (#450)"
area: codereg
paths: ["src/qm/main.cpp", "src/qm/input.cpp", "src/qm/output.cpp", "scripts/parsers/ai.ecceqm", "scripts/parsers/ecceqm.desc", "scripts/parsers/ecceqm.parse", "scripts/parsers/verifyinput", "tests/qm/scan_test.py", "tests/apps/ecceqm_scan_panel_test.py"]
issues: [450, 393, 410, 457]
---
A Geometry run with a scan constraint from the Builder becomes a `scan`
block in the engine input (`ai.ecceqm`, `readCnstrntFile`: `<Scan steps= step=>`
gives from = value, to = value + steps * step, steps + 1 points, atoms
counted from 1). The engine loops constrained optimisations; the property
keys come from the output itself, not from a side file as for ORCA:

* `begin scan_table` is rewritten whole after every point, so
  `[SCANCOORD][SCANENERGY]` use `Frequency=last`. With `Frequency=all` the
  parser would deliver a growing table seven times. Anything that counts
  table rows in an engine output (tests!) must take the last block.
* `begin scan_geometry` is one block per point, `[SCANGEOM]` `Frequency=all`,
  one SCANGEOM key per block, as GEOMTRACE does.
* `progress scan point=k/n` is the `scan` stage of the Progress box, the same
  stage name ORCA's and Gaussian 16's `.desc` files use.
* TE, TEVEC, GEOMTRACE and EGRAD behave as in a plain optimisation (every
  step of every point is in the trace; TE is the last point's energy).

Two engine facts found on the way: a diatomic with its bond held has an empty
step space, which crashed the model-Hessian step (guarded in
`optimize_geometry`); and a UHF singlet never leaves the closed-shell
solution from a symmetric guess, which is why `guess_mix` exists (applied at
every scan point so an ascending H2 scan finds the broken-symmetry curve
beyond the Coulson-Fischer point and returns to RHF before it).

The dihedral sign is the engine's (IUPAC, `atan2(|b2| b1.(b2xb3), n1.n2)`); a
test that recomputes torsions must use that formula, the other common
`atan2(m1.n2, n1.n2)` form has the opposite sign.
