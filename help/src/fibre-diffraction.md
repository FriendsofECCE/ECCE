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
- **B** (20 A^2 by default): an isotropic temperature factor, damping the high angles.
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
