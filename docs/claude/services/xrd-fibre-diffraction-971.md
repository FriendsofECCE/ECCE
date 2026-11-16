---
type: pitfall
title: Fibre diffraction (ecce-xrd, #971) - helix sample geometry and Ewald effects
area: services
paths: [scripts/ecce_xrd.py, tools/fibre/make_samples.py, data/client/StructureLibrary/Teaching/Fibres, src/wxgui/wxdialogs/DiffractionDialog.C]
issues: [971]
---
- An ideal polyalanine helix at phi -57, psi -47 has a 5.64 A pitch with the
  Engh-Huber N-CA-C angle of 111.2; the canonical 3.6 residues per turn, 1.5 A
  rise, 5.4 A pitch needs N-CA-C 109.5 (`tools/fibre/make_samples.py`). Measure
  rise and turn from the rigid transform of one residue onto the next, not from
  a principal axis of a model with side chains.
- The Builder's DNA generator joins rigid units, so its helix is a 10-fold B-form
  with a rise of 3.29-3.48 A and a twist of 35-37 degrees per step, not an exact
  screw; layer lines fall within 1 percent of l/34 A^-1 for this sample.
- A flat film with the fibre perpendicular to the beam cuts the Ewald sphere so
  that a meridional reflection at Z lies at R = (1 - sqrt(1 - (lambda Z)^2)) /
  lambda, not on the axis; the (R, Z) map has it at R = 0.
- The dialog's CSV and the CLI's are the same function: the dialog passes only
  the structure file, an atom list and the fields, so a results window can open
  it the same way as the Builder.

## Powder (#587)

- `ecce-xrd powder` merges reflections of equal d (tolerance 1e-4 A) into one
  line and counts the multiplicity, so no space-group tables are needed. The
  library returns every line including the systematically absent ones (I ~ 0);
  the table, CSV and `--json` leave out I_rel < 0.001.
- gemmi's `calculate_sf_from_small_structure` counts a special position once per
  symmetry operation (48x too large for Cu); sum `get_all_unit_cell_sites()` with
  `element.it92.calculate_sf` instead (tests/xrd/powder_test.py).
- A periodic structure reaches the dialog as an .xyz whose comment line is
  `Lattice="ax ay az bx by bz cx cy cz"` (`Builder::writeStructureXyz`). The
  Diffraction entry in a calculation's results is added by hand to the Builder's
  Properties menu and list for a structure with a lattice (`calcIsPeriodic`); it is
  not a parsed property. It uses the last GEOMTRACE step when the trace has one
  row per atom.
- Neutral-atom IT92 form factors: KCl (111) is weak (I ~ 0.3 % of (200)) rather
  than zero, which would need K+ and Cl-.

## Single crystal (#972)

- `ecce-xrd layer --zone l=0` (hk0), `k=0` (h0l), `h=0` (0kl) or `--normal u v w
  --offset N` (uh+vk+wl = N). Intensities are |F|^2 with no Lorentz factor
  (a precession picture); (000) is left out. A spot below 1e-6 of the strongest
  is "absent".
- The reciprocal axes are drawn only if they lie in the plane (b* is not in an
  h0l layer of a hexagonal cell); with none in the plane, their projections.
- The dialog does not recompute anything: it runs the CLI with `--out png
  --out csv --geometry FILE` and places the hover spots from the CSV x, y and
  the PNG's pixel scale in that file. Test hook: `calclayer` (Builder script).

## Window drawing and layer scale (#985, #984)

- The Diffraction window draws axes, titles, hkl and ring labels, layer spots and
  the powder curves itself (`DiffractionDialog::paint`, wxGraphicsContext, system
  font); Save PNG renders the same function at 2x. `ecce-xrd` PNGs with the 5x7
  font remain only for the CLI. Layers and frame overlays come from the CSV and
  the `--geometry` file (scale, rmax, axes, distance, wavelength), not from the PNG.
- `ecce-xrd layer` sets I_rel = 100 for the strongest reflection of the whole
  crystal within the resolution limit (`--scale-to layer` for the old per-layer
  scale), so KCl hk1 (111) is 0.3 % and NaCl's 4.5 %.
- wxGraphicsContext cannot draw into wxSVGFileDC: there is no SVG export from
  the window.
- The layer view is rendered by `DiffractionDialog::renderFilm` from the CSV
  (optical density, 1 - exp(-gain * density)); the CLI's disc PNG of `ecce-xrd
  layer` is separate and only for the command line. Fibre and frame PNGs from
  `ecce-xrd` are read back as 0..1 levels (`--no-invert`; the frame's light-on-
  dark `--dark` is not used by the dialog), so exposure and colour scheme apply
  in the dialog without recomputing. Script hooks: `calcmeasure`, and
  `calcdiffraction ... EXPOSURE SCHEME` (tests/apps/diffraction_examples_test.py).
