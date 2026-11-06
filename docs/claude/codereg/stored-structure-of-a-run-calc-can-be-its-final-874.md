---
type: pitfall
title: "A calculation's starting structure is GEOMTRACE step 0, not chemsys.mvm; compare structures by distances, not coordinates"
area: codereg
paths: [src/apps/organizer/WorkflowSteps.C, src/apps/organizer/CalcMgr.C, src/apps/launch/ecce-workflow.C, tests/launch/ecce_workflow_create_test.py]
issues: [874]
---
For an imported output (every stored example) `Parameters/chemsys.mvm`, and so
`PropertyTask::fragment()`, holds the *final* structure, not the input one.
The starting structure is the first frame of `GEOMTRACE`
(`getFragmentStep(frag, 0)`); only a calculation without a trace (not run
yet) has its start in the fragment. `WorkflowSteps::proposeLinks` uses that
start and the source's result geometry (`finalGeometry`).

Gaussian writes its trace in standard orientation, while the next
calculation's input (and its first frame) can be in another frame: the SN2
reactant's last frame and the scan's first differ by a rotation. Matching
"same coordinates" therefore fails; `WorkflowSteps::sameStructure` compares
the element sequence and every interatomic distance (0.01 A).

Provenance that ECCE does record: `ecce:orbitalguesssource` and
`ecce:hessianfromsource` (the source's *name*, set by
`OrbitalGuess::attach`/`attachHessianFile`). Rerun, Duplicate and Continue
from Last Geometry record nothing; the last one is found by structure.
Several matching sources: one that changed the structure (opt, scan) wins
over a single point that ended where it started, so an SP and a frequency
run on one geometry do not link to each other in a loop.
