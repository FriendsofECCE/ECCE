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
- **Translations/rotations**: the zero rows get Tx..Rz and their irreps from
  the projection operator of the group's own table on the ideal motions in
  the symmetry frame (planar C2v: x and y renamed with the B1/B2 swap). A row
  with a displacement is named only if one ideal motion holds more than 90 %
  of it; a row with none (ORCA, ECCE-QM print zeros) is named by position,
  Tx Ty Tz Rx Ry Rz, with a remark in its tooltip; a mixture stays unnamed.
- **nu**: the stretch symbol is nu with a combining tilde; a bare nu is
  drawn as a v in Cantarell.
- **Layout**: the detail line is two wrapped lines (under the table, and
  under the plot in the Graph view next to a frequency line), the grid is not `AutoSize()`d (that resizes the window); the
  Mode column takes what is left of the grid's width up to 190 px and the
  cell text is ellipsized beyond; the detail lines have a tiny minimum width. Neither may widen the
  docked panel; `panel_test.py` compares each molecule's panel size with
  water's.

- **Where a mode is shown (#579):** `VibModeInfo::parts` carries the
  contributions (>= 5 %, at most 8) for the Spectrum window's strip and
  `SpectrumCanvas::setModeNotes` hands them, with the label, to every canvas.
  The Composition window's automatic set adds the description's torsions
  (`composition(..., torsions=true)`), else a torsion mode is listed as bends.
  `tests/vibanalysis/mode_view_test.py` checks strip, hover, decimals, the
  Mode-column drag (a real X drag in Xvfb) and the Composition columns on the
  galactopyranose example.
