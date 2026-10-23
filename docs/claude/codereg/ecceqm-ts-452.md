---
type: map
title: "ECCE-QM transition-state search (#452): runtype ts, P-RFO with mode following, run type name"
area: codereg
paths: ["src/qm/optimizer.cpp", "src/qm/main.cpp", "src/qm/input.cpp", "scripts/parsers/ai.ecceqm", "scripts/parsers/verifyinput", "data/client/cap/ECCE-QM.edml", "tests/qm/ts_test.py"]
issues: [452, 419]
---
`runtype ts` in the engine input is `optimize_ts` (partitioned RFO from a
computed Hessian, Bofill updates, `ts_hessian_every n` to recompute) followed
by the ordinary frequency run; stage 1 of #452, IRC is not part of it. The
Hessian is the one frequency runs use (analytic for closed-shell RHF,
finite differences otherwise), now behind the `derivatives` lambda in
`main.cpp`.

* The output is an optimisation's (`progress opt`, GEOMTRACE, TE) plus the
  frequency blocks, so no parser key was added; the Progress box shows the
  `opt` stage. The run exits 1 unless the final frequencies have exactly one
  imaginary mode.
* The run type in the Calculation Editor is `Transition State (Opt-TS)`
  (`ECCE-QM.edml`, `RunType()` in `ai.ecceqm`). The existing "Transition
  State (QST)" and "(NEB-TS)" names are not reused: they are the experimental
  #419 flow (`ECCE_EXPERIMENTAL_TS`, reactant and product structures picked
  from other calculations, `CalcEd::tsRuntypeSelected`), which ORCA and
  Gaussian use to build their own guess; a search from the editor's own
  structure needs none of it. A single atom is not offered the run type
  (the exclusion list in `CalcEd.C`).
* No constraints and no dummy atoms (input error; `ai.ecceqm` and
  `verifyinput` say so too).
* The followed mode is the lowest at step 1, then the one with the largest
  overlap with the previous step's mode (not "the lowest" each time).
* Oracle: ORCA 6.1.1 `OptTS TightOpt Freq` (`gen_ts_oracle.py`,
  `ts_oracle.json`), HCN <-> HNC HF/STO-3G and HF/6-31G*; the start
  geometry is a bridged guess in `ts_cases.py`.
