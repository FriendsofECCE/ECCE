---
type: map
title: "The Vibrational Frequencies table's Sym and Mode columns are VibAnalysis (#367): irreps from ECCE's own operations, a projection on internal coordinates, and what makes it refuse"
area: wx-viewer
section: "wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls"
paths: ["include/tdat/VibAnalysis.H", "src/tdat/chemistry/VibAnalysis.C", "src/apps/builder/NModePanel.C", "tests/vibanalysis"]
issues: [367]
---
`VibAnalysis` (wx-free, `eccetdat`) takes the calculation's structure, bonds
and VIB displacements; `NModePanel::analyseModes()` runs it once per panel.
Tests: `vibanalysis_cli` with `tests/vibanalysis/run_tests.py` (real parser
output, textbook irreps, activity and labels for H2O, CO2, NH3, CH4, HCHO,
benzene) and `tests/vibanalysis/panel_test.py` (the table in the real
Builder, `ECCE_NMODE_TABLE_DUMP=<path>`).

- **Symmetry** runs in autosym's standard frame: `SymmetryOps::find` on a
  copy, `MoFragments::symmetryOperations`, `alignFrames` to rotate the
  displacements. A degenerate set is classified together (frequencies within
  1 + 0.002 nu cm-1) and must reduce to one irrep of that dimension; else
  each mode alone, else `?`. IR and Raman activity are the irreps of x,y,z
  and of the six x_i x_j, computed from the operations.
- **Linear molecules** come out of autosym as D4h or C4v and are renamed to
  sigma/pi names (`linearName`).
- **Planar C2v**: autosym's frame can name the in-plane antisymmetric
  stretch B1; the textbook (molecule in yz) has B2, so B1 and B2 are
  swapped when needed.
- **Refusal**: modes in another orientation than the structure (MOPAC, see
  the frame entry in codereg) are detected by the share of rigid motion and
  get no labels at all. A wrong description would be worse than none.
- **Description**: B-matrix rows by central differences; share =
  S_a^2 / G_aa (mass-weighted cosine) normalised over the coordinates,
  groups of equivalent coordinates summed. A planar centre with neighbours
  A, B, B has three bends that sum to 360 degrees; the in-phase AXB pair is
  credited to BXB, or the HCHO scissor reads half HCH and half HCO.
  Torsions use one dihedral per bond through the heaviest neighbours (nine
  H-C-C-H would outvote everything).
- **Layout**: the grid is not `AutoSize()`d (that resizes the window); the
  Mode column takes what is left of the grid's width up to 190 px and the
  cell text is ellipsized beyond, the detail line under the table is an
  ellipsized static text with a tiny minimum width. Neither may widen the
  docked panel; `panel_test.py` compares each molecule's panel size with
  water's.
