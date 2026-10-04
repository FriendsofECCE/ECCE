---
type: index
title: "The MO correlation diagram (#132)"
area: mo-diagram
---
# The MO correlation diagram (#132)

Split out of CLAUDE.md 2026-10-04.

Runs `SymmetryOps::find()` (autosym — **reorients and symmetrises**) →
`MoFragments` (shells out to `symops`) → `CharacterTable::reduce()` →
`MoDiagram::{classify, connect, placeFragments}` → `MoDiagramCanvas`.
Two callers build the model and must stay in step:
`MoDiagramPanel::build()` (from a calculation) and
`include/tdat/MoSpec.H` (from a spec file, used by the offline tools).

A **standalone diagram program is planned as a separate Python
project, not part of ECCE** (Andy's call, 2026-09-24). A C++ one was
built and removed; `tools/modiagram/fromxyz.py` (XYZ → autosym →
MOPAC → spec) is the bridge it would reuse.

## Entries

### The MO correlation diagram (#132)

- [Start from the AO composition, not symmetry machinery](start-from-the-ao-composition-not-symmetry.md)
- [A fragment must be a union of orbits](a-fragment-must-be-a-union-of.md)
- [A complex is classified by its coordination skeleton, not the molecule](a-complex-is-classified-by-its-coordination.md)
- [connect() runs BEFORE placeFragments()](connect-runs-before-placefragments.md)
- [Look at `tools/modiagram/render`'s output before believing any layout claim](look-at-tools-modiagram-renders-output-before.md)
- [empty and no line is drawn](empty-and-no-line-is-drawn.md)
- [not sorted by energy](not-sorted-by-energy.md)
- [- Fragment columns may be opened out for legibility; the molecular column neve](fragment-columns-may-be-opened-out-for.md)
