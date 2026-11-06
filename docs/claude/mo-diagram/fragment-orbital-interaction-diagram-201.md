---
type: map
title: "Fragment-orbital interaction diagram: ecce-fragorb diagram and the Details window's Diagram page (#201 step 3)"
area: mo-diagram
section: "The MO correlation diagram (#132)"
paths: ["include/tdat/FragmentAnalysis.H", "src/tdat/resultsh5/FragmentAnalysis.C", "src/tdat/resultsh5/ecce_fragorb.C", "src/apps/organizer/FragOrbDiagram.C", "src/apps/organizer/FragOrbDetailsDialog.C", "tests/apps/fragorb_diagram_test.py", "tests/fragorb_h5/run_tests.py"]
issues: [201]
---
Separate from the Builder's MO Diagram panel (MoDiagram*, #132); nothing is
shared with it.

- **What is drawn is the library's answer** (`fragorb::diagram`), the GUI only
  places it. One rule, no per-case exceptions: window levels, then the closure
  "MO level sharing an FO level (>= threshold) with a drawn occupied MO level",
  then FO levels with a line. Do not add special cases for a molecule; change
  the threshold or window instead.
- The closure can reach far: on Cr(CO)6 the 13.5 % Cr 4p share of MO 49-51
  pulls in MO 86-88, 97-99 and 151-153 (up to +57 eV). Hence the axis breaks
  (empty stretches > 5 eV, never between HOMO and LUMO) in FragOrbDiagram.C.
- Level-level share = sum over the FO level, mean over the MO level (invariant
  to mixing within degenerate sets). Degeneracy 1e-4 Eh (CLI --levels); the
  box's FragOrbBox uses 0.003 eV, nearly the same but not identical.
- Bonding/antibonding = sign of the per-electron fragment-fragment Mulliken OP
  of that MO (from `dos()`), only when lines come from two fragments.
  High virtuals can have |OP| >> 1 (non-orthogonal basis); that is data.
- Fragment names in verdict/notes come from `--name`; the GUI passes its
  display names so the CLI text can be shown verbatim.
- Tests: `fragorb_analysis` recomputes levels/lines from `analyse --json` in
  Python (oracle) and checks sum n_i OP_i = dos occupied overlap;
  `apps_fragorb_diagram` compares the GUI layout dump (`fragdiagram` hook)
  with the CLI and runs profile_layout.py's overlap check (bars are dumped as
  segments of their height).
