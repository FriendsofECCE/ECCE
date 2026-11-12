# Diffraction: fibre patterns

**Tools > Diffraction...** (Builder) calculates the X-ray diffraction pattern of
the structure, or of the selected atoms. The window is not tied to the Builder:
it needs a structure and an optional atom selection, nothing else. **Fibre** is
the pattern type available now; **Powder** and **Single crystal** are listed
and greyed out, because they need a periodic structure.

A fibre pattern is what a stretched fibre of aligned molecules gives: the
molecules lie along one axis and are rotated at random about it. The intensity
is the average of |F|^2 over that rotation, so it depends on two numbers only:
R, the distance from the axis in reciprocal space, and Z, the position along it.
Spots fall on layer lines at Z = l / repeat. For a helix, layer line l carries
the Bessel function J_n(2 pi R r) of order n allowed by the helical symmetry.

## Calculating

1. Open the structure in the Builder. To use a part of it (one helix of a
   protein, say), select those atoms first.
2. Choose **Tools > Diffraction...** and press **Calculate**.
3. Move the pointer over the picture. The line below it gives the layer line l
   (when **Repeat** is set), R, Z, the spacing d = 1/|S| and the intensity.

The fields:

- **Wavelength** (Cu K-alpha, 1.5418 A, by default) and **Film distance** (30 mm)
  and **Film size** (40 mm): a flat film perpendicular to the beam, the fibre across the
  beam. The picture is the film. Where a reflection falls on it follows from
  the Ewald sphere, so the meridional reflections of high layer lines are
  displaced from the vertical axis or missing, as in a real photograph with an
  untilted fibre.
- **B** (20 Å² by default): an isotropic temperature factor, damping the high angles.
- **Fibre length**: broadens the layer lines along Z as a fibre of that length
  would (100 A by default). It is added to the broadening of the molecule's own
  length. 0 is off.
- **Misalignment** (8 degrees by default): the spread of the fibre axes, which draws the
  spots out into arcs. The smearing follows circles of constant |S| and ignores
  the change of the orientation weight along the arc, so it is a good picture
  of the arcs and not a quantitative profile.
- **Axis**: the principal axis of the atoms (the helix axis of a helix), or the
  line from the first to the last selected atom.
- **Dark spots on white** (on by default, `--no-invert` on the command line)
  turns the picture to light on dark. **Scaling**: logarithmic, square root or linear, and a gamma exponent. The
  intensity range of a pattern is wide; the logarithmic scale shows 2.5 decades
  below the strongest reflection. The centre, where the undiffracted beam
  would be, is left out of the scaling. The defaults give a picture like a
  photographic print of a fibre film; set B, fibre length and misalignment to 0
  for the sharp calculated pattern.

Form factors are the Cromer-Mann coefficients of the International Tables
(Vol. C, Table 6.1.1.4), for H, C, N, O, Na, Mg, P, S, Cl, K, Ca, Fe and Zn.

## Powder patterns: the K-alpha doublet

For a Cu, Mo or Co source the Powder pattern draws every reflection twice, at the
K-alpha1 and K-alpha2 wavelengths, with K-alpha2 at half the intensity of
K-alpha1 (untick **K-alpha1/K-alpha2 doublet** for a single line). The two
wavelengths differ by a fixed fraction, so by Bragg's law the lines are further
apart the larger the angle: the doublet is hardly visible at low 2-theta and is
resolved as two peaks at high angle. The peak list has one row per line, marked
Ka1 or Ka2. Other sources (synchrotron, a typed wavelength) have no doublet.

## Layers on one intensity scale

The single-crystal view draws a disc per reflection with its area following
I_rel, where I_rel = 100 is the strongest reflection of the whole crystal within
the resolution limit, not of the layer shown. Layers of one crystal and of
different crystals can therefore be compared: in KCl the all-odd reflections of
the hk1 layer have I_rel near 0.3 and are barely visible, in NaCl 4.5. The CSV
holds I and this I_rel. On the command line `--scale-to layer` gives the old
scale to the layer's own strongest spot.

## Detector frame of a rotating crystal

Choose **Detector frame** for what a rotation camera records. The crystal sits
in a monochromatic beam and turns about the vertical axis through the
**Oscillation** range, starting at the **Start angle phi**; a reflection is
recorded when its reciprocal-lattice point passes through the Ewald sphere
during the turn, so each frame holds only the reflections that cross in that
range and moving phi shows others. Spots are dark on light (untick for false
colour). Intensities carry the Lorentz and polarisation factors and counting
noise (untick **Noise** for a clean frame). **Label spots** writes hkl next to
the strongest spots and **Resolution rings** draws circles at d = 3.0, 2.0, 1.5
and 1.0 A with d and 2-theta; both are drawn on the picture only. The pointer
reports hkl, d and 2-theta of the nearest spot. The white bar and disc are the
beamstop and its arm. The same frame from the command line:
`ecce-xrd frame structure.cif --source mo --phi 0 --osc 30 --out frame.png`.

## Backdrop

**Load Backdrop...** puts a PNG or JPG (a scan of a measured film, for example)
behind the calculated pattern. The slider sets the opacity of the calculated
pattern over it. The image is stretched to the square of the film, so crop the
scan to the film first and put the beam centre in the middle.

## Saving

**Save PNG...** writes the picture as shown, backdrop included. **Save CSV...**
writes every pixel: its position on the film in mm, R, Z and the intensity.

## From the command line

    ecce-xrd fibre structure.xyz --out film.png --out film.csv
    ecce-xrd fibre helix.mvm --map rz --rmax 0.7 --zmax 0.7 --out rz.png

`--select 1-40,57` takes atoms, `--axis x,y,z` sets the axis, `--json` prints
what was done. The sample structures, a 20 base pair B-DNA and a 20 residue
polyalanine alpha-helix, are in the Structure Library under Teaching > Fibres.
For B-DNA with a 34 A repeat the layer lines lie at multiples of 1/34 A^-1, the
reflection of the 3.4 A stacking is on layer line 10, and layer line 4 is weak.
The alpha-helix has its 5.4 A pitch layer line and the 1.5 A rise reflection on
the meridian.
