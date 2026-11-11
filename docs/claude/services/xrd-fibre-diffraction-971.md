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
