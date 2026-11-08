---
type: pitfall
title: "Four places map geometry-trace rows onto a fragment; a protomer trace has more rows than the structure (#912, #920)"
area: wx-viewer
paths: [src/viz/propsgcommands/GTStepCmd.C, src/dsm/edsiimpl/ScanPoint.C, src/dsm/edsiimpl/PropertyTask.C, src/apps/organizer/CalcMgr.C]
issues: [912, 920]
---
CREST --protonate appends the proton, so each protomer frame has one row
more than the calculation's fragment. GTStepCmd (Builder trace panel),
ScanPoint::fragment/adoptTraceAtoms and PropertyTask::loadStep add the
extra atoms from the trace's "n-El" row labels; a change to how traces are
applied must be made in all of them. CalcMgr.C (around the workflow
structure carry) handles dummy atoms but not extra rows.
Symptom when one is missed: "GEOMTRACE step N holds M values for K atoms;
coordinates left unchanged" and the input structure instead of the protomer.
