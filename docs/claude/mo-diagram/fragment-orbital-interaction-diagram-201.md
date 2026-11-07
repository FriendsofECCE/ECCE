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
- Fragment display names come from `groupedFormula()` (Recipe.C): bonded
  molecules by BFS over bonds (covalent bonds made when the fragment has none);
  n identical multi-atom molecules give "(CO)6", anything else the plain formula.
- The verdict separates window levels from far partners: a far partner below
  90 % gives "Representative in the window; ... far partner levels ...", and
  `window_representative` stays true.
- The Details window fits 1280 x 800 (splitter, wrap sizer for the controls);
  `fragdiagram ... size=WxH` reports `fit ... cut=N` for the test.
- Mulliken symbols come from `fragorb::orbitalSymmetry()` (autosym + symops +
  `fullLabelSpectrum`, angular table built for the results.h5 canonical order).
  Linking it pulls in `TChartOfNuclides`' static `TPerTab`, which asserts on
  a missing ECCE_REALUSERHOME while the program loads: ecce-fragorb defaults
  it (and ECCE_REALUSER) in a constructor(101) function. A single atom gets
  no group (no spherical table): Cr FO levels read "?".
- tests/fragorb_h5/run_tests.py's `check()` returns None: never use it as a
  condition (`elif check(...)` silently skipped the label checks once).
- The `cr` fixture (RHF singlet Cr) has 3d⁴-like occupied d orbitals in an
  arbitrary orientation (t2g MOs take 7-33 % from them, unequally): it is not
  the t2g⁶ "state in the complex".
