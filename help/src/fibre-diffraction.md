# Diffraction

**Properties > Geometry > Diffraction** opens a property pane in the Builder.
The pattern fills the pane, with **View**, **Source** and a **Pattern** toggle
above it. **Pattern** moves the whole pattern into a larger separate window;
toggle it again, close that window, or choose **Pattern (return to pane)** to
return it. Settings and the calculated data are shared, so moving the pattern
does not calculate it again. **Tools > Diffraction...** also opens a separate
window for the current structure and selection.

**Options...** opens a modeless window for the current view. Its **Geometry**,
**Sample** and **Display** groups keep settings out of the pattern pane.
Changing view changes the options shown. **Close** keeps the settings;
**Restore Defaults** restores the current view's defaults. Every setting has
a tooltip. **Experimental...**, **Save PNG (view)...** and **Save CSV...** stay
below the pattern.

- **Fibre**: the pattern of molecules aligned on one axis, on a film or as an
  (R, Z) map. Auto uses a helical transform for a regular backbone and the
  finite-model transform for other structures.
- **Molecule (Debye scattering)**: the orientational average for a molecule,
  cluster or the contents of a unit cell, plotted against 2θ. It works for any
  structure.
- **Powder**: intensity against 2θ with hkl labels and a peak list, with the
  K-alpha doublet for Cu, Mo and Co sources. It needs a periodic structure.
- **Single crystal**: a reciprocal-lattice layer (hk0, h0l, 0kl or a higher
  layer) drawn as a precession photograph. It needs a periodic structure.
- **Detector frame, rotating crystal**: what a rotation camera records for an
  oscillation range. It needs a periodic structure.

A structure is periodic when it carries a unit cell: a structure written with
a lattice (an .xyz whose comment line has `Lattice="..."`, as the Builder
writes it for a crystal). For any other structure, Powder, Single crystal and
Detector frame require a unit cell and Fibre is selected; for a periodic one Powder
is selected when the window opens. **Experimental...** belongs to Powder and
overlays a measured pattern.

A fibre pattern is what a stretched fibre of aligned molecules gives: the
molecules lie along one axis and are rotated at random about it. The intensity
is the average of |F|^2 over that rotation, so it depends on two numbers only:
R, the distance from the axis in reciprocal space, and Z, the position along it.
Spots fall on layer lines at Z = l / repeat. For a helix, layer line l carries
the Bessel function J_n(2 pi R r) of order n allowed by the helical symmetry.

## Calculating

1. Open the structure in the Builder. To use a part of it (one helix of a
   protein, say), select those atoms first.
2. Choose **Properties > Geometry > Diffraction**. The first pattern calculates
   automatically. **Options...** contains the current view's settings.
3. Move the pointer over the picture. The line below it gives the layer line l
   (using the inferred or typed layer-line repeat), R, Z, the spacing d = 1/|S| and the intensity.

Changes to the wavelength, geometry or sample recalculate after a pause of
about 300 ms, so typing a number starts one run after typing stops. Enter also
accepts a text field; the start-angle slider calculates when released.
**Calculating...** appears in the pointer line while the external calculation
runs. The Builder remains usable. Another change cancels the old run, and
only the newest result is shown.

## Slow structures and Update

If a run takes more than about two seconds, Diffraction says so and shows
**Update** for that structure. Subsequent changes wait for Update, including
view and source changes; the first run finishes normally. A change during a
run cancels that run. Press Update to calculate the latest settings. Exposure,
colour scheme, intensity scaling, gamma, labels, overlays and backdrop opacity
redraw immediately without running the calculation.

## Geometry options and Source

**Source** stays above the pattern. Cu, Mo and Co fill the wavelength in
Options; typing a wavelength selects **Other**. For fibre Cu uses the mean
K-alpha wavelength, 1.5418 Å; the crystal and powder views use K-alpha1,
1.5406 Å. Powder and Debye have **2θ from**, **2θ to** and **2θ step** in degrees.
Single crystal has the zone, layer number and smallest spacing. Detector frame
has detector distance, start angle and oscillation.

## Fibre options: Geometry, Sample and Display

The fields:

- **Wavelength** (Cu K-alpha, 1.5418 Å, by default) and **Film distance** (30 mm)
  and **Film size** (40 mm): a flat film perpendicular to the beam, the fibre across the
  beam. The picture is the film. Where a reflection falls on it follows from
  the Ewald sphere, so the meridional reflections of high layer lines are
  displaced from the vertical axis or missing, as in a real photograph with an
  untilted fibre.
- **Thermal smearing B (Å²)** (20 Å² by default): an isotropic temperature factor, damping the high angles.
- **Fibre length**: broadens the layer lines along Z as a fibre of that length
  would (100 Å by default), with Gaussian FWHM 1/length in Å⁻¹. In helical mode
  this is the only length broadening. Finite-model mode also carries the
  molecule's own length fringes. 0 is off.
- **Misalignment** (8° by default): the spread of the fibre axes, which draws the
  spots out into arcs. The smearing follows circles of constant |S| and ignores
  the change of the orientation weight along the arc, so it is a good picture
  of the arcs and not a quantitative profile.
- **Axis**: the fitted screw axis in helical mode, the principal axis in
  finite-model mode, or the
  line from the first to the last selected atom.
- **Intensity scaling** and **Display gamma (contrast)**, under Display: logarithmic, square root or linear scaling, and a gamma exponent. The
  intensity range of a pattern is wide; the logarithmic scale shows 2.5 decades
  below the strongest reflection. The centre, where the undiffracted beam
  would be, is left out of the scaling. The defaults give a picture like a
  photographic print of a fibre film; set B, fibre length and misalignment to 0
  for the sharp calculated pattern.

Form factors are the Cromer-Mann coefficients of the International Tables
(Vol. C, Table 6.1.1.4), for H, C, N, O, Na, Mg, P, S, Cl, K, Ca, Fe and Zn.

## Molecule (Debye scattering)

Choose **Molecule (Debye scattering)** to average over all orientations of
the structure. This is suitable for a gas of molecules or a finite disordered
cluster. It uses the same neutral-atom IT92 form factors and **Source**,
**Wavelength**, **K-alpha1/K-alpha2 doublet**, angle range and **B** settings
as Powder. Cu, Mo and Co use their tabulated wavelengths; a custom wavelength
uses a single line. The doublet combines the two intensities in a 2:1 ratio.

The calculation is

    I(q) = sum_i f_i(q)^2 + 2 sum_{i<j} f_i(q) f_j(q) sin(q r_ij)/(q r_ij)
    q = 4 pi sin(theta)/lambda,  s = q/(4 pi)

Distances are in Å and q is in Å⁻¹. The self terms are included and
sin(qr)/(qr) is 1 at qr = 0. Each form factor is multiplied by exp(-B s²),
as in Powder. No powder Lorentz or polarisation correction or artificial
peak width is applied. The displayed curve is scaled to a maximum of 100
over the chosen angle range; it has no hkl labels or reflection table.

Up to 3000 atoms the sum uses exact pair distances. Above 3000 atoms it uses
an element-pair distance histogram with 0.01 Å bins; the status line states
the method. Building the histogram still takes O(N²) pair work, but avoids
evaluating every pair at every angle. The histogram is an approximation.

A periodic structure contributes the atoms in its cell once, without
periodic images or automatic replication. A single NaCl pair gives broad
oscillations; a larger ordered NaCl cluster develops peaks near the crystal's
Bragg positions. A water cluster can similarly be compared with crystalline
ice to discuss local and long-range order. Intensities and finite-size peak
widths differ from the infinite-crystal Powder view.

**Save PNG (view)...** saves the displayed curve and **Save CSV...** saves columns
`two_theta,intensity`, like Powder. The pointer reports 2θ, d, Q and intensity.
From the command line:

    ecce-xrd debye molecule.xyz --source cu --out debye.csv --out debye.png --json
    ecce-xrd debye cluster.xyz --wavelength 0.71073 --tt-min 0 --tt-max 90 --step 0.05

The CLI also accepts `.pdb`, `.mvm` and periodic `.cif` structures,
`--select 1-40,57`, `--B`, `--no-doublet` and `--measured` as for the existing
views. JSON records the method and distance bin width, the q grid and raw
and relative intensities. q is in Å⁻¹; the plot and CSV use 2θ in degrees.

## Helical transform

**Transform** chooses **Auto** (default), **Helical**, or **Finite model**.
Helical treats the model as a sample of an infinitely repeated helix. Auto fits
consecutive phosphorus atoms in a nucleic-acid backbone or consecutive peptide
nitrogens in file order. It splits chains at large gaps and fits a rigid screw
to the longest chain, requiring at least six anchors and RMS error below 0.15 Å.
Use a selected helix for a structure containing other molecules. A reordered,
bent or irregular backbone may not be recognized: Auto then uses Finite model;
Helical reports an error. The status line says which transform was used.
An explicitly selected axis must agree with the fitted screw axis.

The measured twist is approximated by t/u turns per unit (u at most 20,
within 1°); the rise h is measured, giving the axial repeat c = u h.
Complete interior steps are folded onto one screw unit and averaged, removing
end caps and averaging small deviations from ideal symmetry. The transform is
an idealization of that motif, not a refinement of the model. **Layer-line repeat for the pointer (Å), 0 = from the model** uses c
when it is zero. A typed non-zero value survives later calculations and changes
only the layer numbers in the pointer and tooltip; it is never passed to the
scattering calculation. For the Teaching B-DNA sample h = 3.387 Å, u = 10, t = 1,
c = 33.867 Å. For polyalanine h = 1.503 Å, u = 18, t = 5,
c = 27.061 Å (five pitches of 5.412 Å); the pitch line is l = 5 and the
1.503 Å meridional reflection l = 18.

On each line Z = l/c the amplitude of order n is
`A_n = u sum_j f_j J_n(2π R r_j) exp(-i n φ_j + 2π i l z_j/c)`,
where j runs over the averaged screw unit. Only orders with
`l - n t = u m` for integer m contribute; intensity is `sum_n |A_n|²`.
This is the helical selection rule described in the
[helical symmetry convention](https://journals.iucr.org/d/issues/2011/08/00/yt5034/index.html).
Gaussian length broadening and angular misalignment are applied to these line
intensities, followed by the same logarithmic film display. There are no
finite-model side fringes. The double-strand interference makes the B-DNA
fourth layer weak, not an exact extinction for these atomic coordinates;
no layer is removed by hand.

**Finite model** retains the cylindrical average of the entire finite
structure for non-helical fibres and comparisons. From the command line use
`--transform auto|helical|finite`; `--json` reports the mode and, when fitted,
the repeat, rise, units, turns, screw axis, anchor element and fit RMS error.
`--geometry FILE` writes the inferred repeat (zero for a finite model).

## Powder patterns: the K-alpha doublet

For a Cu, Mo or Co source the Powder pattern draws every reflection twice, at the
K-alpha1 and K-alpha2 wavelengths, with K-alpha2 at half the intensity of
K-alpha1 (untick **K-alpha1/K-alpha2 doublet** for a single line). The two
wavelengths differ by a fixed fraction, so by Bragg's law the lines are further
apart the larger the angle: the doublet is hardly visible at low 2θ and is
resolved as two peaks at high angle. The peak list has one row per line, marked
Ka1 or Ka2. Other sources (synchrotron, a typed wavelength) have no doublet.

## Experimental pattern

**Experimental...** (Powder view) opens a measured powder pattern and draws it
as a second curve, labelled "measured", under the calculated one, on the same
2θ axis. The file formats are:

- **Text columns** (.xy, .xye, .dat, .txt, .csv): 2θ in degrees in the first
  column and the intensity in the second, separated by blanks, commas,
  semicolons or tabs. Further columns, such as the error of an .xye file, are
  ignored. Lines that do not start with two numbers (headers, comments) are
  skipped. The points are sorted by 2θ.
- **PANalytical .xrdml**: the first scan that has 2θ positions and intensities
  (or counts). The positions are taken from the file's position list, or
  spread evenly between its start and end positions.
- **Bruker .brml**: the first raw-data record that holds measured points.
  2θ and the counts are read from each row; points the instrument never
  measured are left out.

The format is recognised from the content, not from the file name. The
measured curve is scaled so that its strongest point inside the 2θ range of
the calculation is 100, like the calculated curve, and is drawn only where the
file has data. A file with no point inside that range, or one that cannot be
read, gives a message that says why (for example no two-column data, or no 2θ
scan in an .xrdml file), and no measured curve is drawn. The
calculated peaks use the wavelength or K-alpha lines of the **Source**
setting, so choose the source the measurement was made with for the peaks to
line up. From the command line: `ecce-xrd powder structure.cif --measured
scan.xy --out powder.png`.

## Layers as precession photographs

The single-crystal view is a simulated precession photograph: the undistorted
reciprocal-lattice layer on film, each reflection a blurred spot whose darkness
saturates with its intensity and whose size grows gently with it, on a faint
background that rises toward the centre, with a little film grain and a small
round beamstop. Darkness follows I_rel, where I_rel = 100 is the strongest
reflection of the whole crystal within the resolution limit, not of the layer
shown. Layers of one crystal and of different crystals can therefore be
compared: in KCl the all-odd reflections of the hk1 layer have I_rel near 0.3
and the photograph is almost blank, in NaCl 4.5 and they show. The CSV holds I
and this I_rel. **Label spots (hkl)**, **Show extinct reflections** and
**Reciprocal axes** (a*, b* in the lower left corner) are overlays that you
can switch; they are not part of the data.

## Exposure and colour scheme

The **Display** group in Options holds for the detector frame and the
precession photograph an **Exposure** slider and a colour scheme. Exposure is a
gain on the intensity before saturation, in tenths of a stop (-2 to +2 stops),
like a shorter or longer exposure: weak spots appear and strong ones spread.
The data and the CSV do not change. **Film** is grey, dark on light (the
default); **Dark** runs from black through blue and orange to white at
saturation. **Save PNG (view)...** uses the current choice.

## Zoom and pan in image views

In the detector frame, precession photograph and fibre film, turn the mouse
wheel to zoom about the pointer. Drag a rectangle with the left button to
zoom into it; the image keeps its square proportions. Drag with the middle
or right button, or hold Shift while dragging, to pan. A click without a
drag still measures a spot, including Shift-click for a second detector spot.
Click the image to give it keyboard focus, then press **Home** to fit the
whole image, or double-click empty image space. **Escape** cancels a drag.
Zoom is limited to 32 times; panning stays inside the image. Spot and ring
labels keep their text size, and axes and ring lines are drawn sharply at
each zoom. The reciprocal-axis key stays in the lower left corner.

**Save PNG (view)...** saves the current zoomed view, including its overlays
and backdrop, at twice the displayed resolution. Press Home before saving
to export the whole image. CSV always contains the full calculated data.

## Measuring

- **Detector frame**: click a spot to read hkl, d, 2θ, I and its position
  on the detector in the pointer line and pinned tooltip. Shift-click a second spot
  for the distance between the two on the detector (mm), the angle between
  them seen from the beam centre, and the d of each.
- **Precession photograph**: click two spots. The pinned tooltip gives the reciprocal
  distance (Å⁻¹) and the d of that row. When both lie on one lattice row with n
  steps between them, it also gives a* = distance / n and the cell edge 1 / a*
  (valid for orthogonal axes).
- **Powder**: **Label peaks above (% of strongest)** sets which peaks carry an hkl label (8 by default, 100 for only the strongest). The pointer line gives 2θ, d, Q = 2π / d and I of the curve
  under the pointer, and the nearest reflection.

## Detector frame of a rotating crystal

Choose **Detector frame** for what a rotation camera records. The crystal sits
in a monochromatic beam and turns about the vertical axis through the
**Oscillation** range, starting at the **Start angle phi**; a reflection is
recorded when its reciprocal-lattice point passes through the Ewald sphere
during the turn, so each frame holds only the reflections that cross in that
range and moving phi shows others. Spots are dark on light (choose the Dark scheme for false
colour). Intensities carry the Lorentz and polarisation factors and counting
noise (untick **Noise** for a clean frame). **Label spots** writes hkl next to
the strongest spots and **Resolution rings** draws circles at d = 3.0, 2.0, 1.5
and 1.0 Å with d and 2θ; both are drawn on the picture only. The pointer
reports hkl, d and 2θ of the nearest spot. The round white disc is the
beamstop. The same frame from the command line:
`ecce-xrd frame structure.cif --source mo --phi 0 --osc 30 --out frame.png`.

## Backdrop

**Load Backdrop...** puts a PNG or JPG (a scan of a measured film, for example)
behind the calculated pattern. **Pattern opacity over backdrop** sets the opacity of the calculated
pattern over it. The image is stretched to the square of the film, so crop the
scan to the film first and put the beam centre in the middle.

## Saving

**Save PNG (view)...** writes the current view, backdrop included. Home
restores the whole image for export. **Save CSV...** writes every pixel: its position on the film in mm, R, Z and the intensity.

## From the command line

    ecce-xrd fibre structure.xyz --out film.png --out film.csv
    ecce-xrd fibre helix.mvm --map rz --rmax 0.7 --zmax 0.7 --out rz.png

`--select 1-40,57` takes atoms, `--axis x,y,z` sets the axis, `--json` prints
what was done. The sample structures, a 20 base pair B-DNA and a 20 residue
polyalanine alpha-helix, are in the Structure Library under Teaching > Fibres.
For B-DNA with a 34 Å repeat the layer lines lie at multiples of 1/34 Å⁻¹, the
reflection of the 3.4 Å stacking is on layer line 10, and layer line 4 is weak.
The alpha-helix has its 5.4 Å pitch layer line and the 1.5 Å rise reflection on
the meridian.

## Interactive pattern tooltips

Hover over the pattern to see a tooltip at the pointer as well as the pointer
line. Click to keep that tooltip while moving elsewhere; click another point
to replace it. A new calculation clears the pinned tooltip. In Powder, a click
also selects the reflection in the peak list under Options > Display.

- Powder reports hkl, 2θ, d, relative intensity and multiplicity.
- Single crystal and detector frame report hkl, d, |F| and I. Extinct reflections
  are retained for picking and say **extinct in this model (|F| ≈ 0)**. This is an
  intensity cutoff, not a space-group classification. The detector's I includes
  Lorentz and polarisation factors, so |F| is not the square root of I.
- Fibre reports layer line l, Z, R and the nearest spacing d = 1/|S|.
- Debye reports 2θ, q and intensity without a reflection assignment.

The detector CSV appends `F,absent` after its original columns; F is |F| and
absent is 1 for the extinct reflections. Layer CSV already includes those
values. Display overlays do not change the exported data.
