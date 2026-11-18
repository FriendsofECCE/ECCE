# Difference and experimental spectra

The infrared plot in the **Vibrational Frequencies** panel can show how two
calculations differ, how a measured spectrum compares with the computed one,
and any wavenumber range you choose.

## Zoom to a range

Type the lowest and the highest wavenumber in the two **Range** fields and
press Enter. Dragging on the plot, the mouse wheel and **Reset zoom** work as
before.

## Difference spectrum

1. Click **Compare...** and choose a second calculation marked **(overlay spectrum)**. Its curve is drawn
   with the same line width and the same **Scale factor** as the first.
2. Tick **Difference**. A pane under the plot, with the same wavenumber axis and zoom,
   shows the second calculation's curve minus the first's on its own signed
   axis, with a solid zero line. A band that moved shows as a peak at its new
   place and a dip at its old one. Where the
   bands coincide the difference is zero.

## Experimental spectrum

Click **Experimental...** and choose a text file with wavenumber (cm⁻¹) in the
first column and absorbance or transmittance in the second, separated by comma,
semicolon, tab or spaces. Other lines, such as a header, are ignored.

**JCAMP-DX files** (`.jdx`, `.dx`, `.jcamp`) are read too, and then ECCE does not
guess: the units come from the file. The wavenumber axis may be 1/CM or
MICROMETERS (converted to cm⁻¹) and the ordinate ABSORBANCE or TRANSMITTANCE
(percent is converted to a fraction). Plain, SQZ/DIF/DUP-compressed and
XYPOINTS tables are understood, and a PEAK TABLE is drawn as narrow peaks.
The curve label shows the title, state and origin recorded in the file. A file
with several spectra (a LINK file) shows the first one, and the label says so.
A file that is not an infrared or Raman spectrum, such as NMR or mass
spectrometry, is refused with a message.

The NIST Chemistry WebBook has gas-phase infrared spectra of many small
molecules: on the compound's page use the **IR spectrum** download link,
"JCAMP-DX format", and open that file here.

- ECCE decides which it is: a spectrum whose baseline lies near its maximum,
  with the bands as dips, is transmittance. Values above 1.5 are read as
  percent. Transmittance T is converted to absorbance, A = -log10(T). The curve
  label says which was assumed.
- The curve is drawn dashed, scaled so that its highest point in the visible
  range equals the highest point of the computed curve there. No baseline is
  removed. It needs a line width above 0 (sticks only has no curve to
  scale to).
- The **Scale factor** moves the computed curve only; the experimental
  spectrum is never shifted.

Click **Remove** to remove it (and any comparison), or **Experimental...** again and answer **Yes**.

With a comparison or an experimental curve loaded the plot shows IR or Raman one at a time, chosen by the **IR | Raman** switch at its top right; with one spectrum they are stacked.

## Export

**Export .csv...** writes the stick table and then the curves on one grid of
1 cm⁻¹: the computed curve, the second calculation's, the difference when
**Difference** is ticked, and the experimental curve when one is loaded (empty
where the file has no data).

To write JCAMP-DX instead, give the file a name ending in `.jdx` in the
export dialog. The file holds two blocks: the broadened curve of the shown
spectrum (IR or Raman) with the width and scaling factor in header lines, and
the sticks as a peak table with the mode numbers. It opens in instrument
software, and **Experimental...** reads it back (the first block, the curve).

## From the command line

`ecce-spectrum <calcA> [<calcB>] [--exp FILE] [--fwhm W] [--scale S]
[--lo X --hi X --step D]` writes the same curves as CSV (or `--json`). The
panel reads experimental files with the same tool, so the two agree.
