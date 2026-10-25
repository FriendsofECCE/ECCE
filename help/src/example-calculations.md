# Example calculations

ECCE ships the outputs of five finished calculations, so that you can look
at orbitals, charges and vibrations without running anything and without any
computational chemistry code installed.

## Open them

The first time you start ECCE, the Organizer imports the `general` set of
examples by itself into a project called `Examples`. It does this once: if
you delete the project, it does not come back at the next start, and an
`Examples` project you already have is left as it is.

The examples come in sets. To import another set, or to find the first one
again, choose **File > Open Example Calculations** in the Organizer. A small
window lists the sets with a line on what each holds; choose one and click
**OK**. ECCE imports the set into its own project in your home folder, as
described in [Importing a calculation](importing-a-calculation.md). The
calculations show the run state "Imported" and take a few seconds to be read.

| Set | Project | Holds |
|---|---|---|
| General | `Examples` | The five calculations below. |
| Computational | `Examples_Computational` | A GROMACS molecular dynamics study of the Trp-cage. |
| Spectroscopy | `Examples_Spectroscopy` | Carbonyl stretches with substituent and charge, IR and Raman intensities, the IR spectrum of water and ethanol, and the sulfate stretches of a bridging sulfate complex ([(en)2Co(mu-OH)(mu-SO4)Co(en)2]3+, ORCA, PBE0-D3BJ/def2-SVP with CPCM(water), frequencies with IR and Raman). |

If the project of a set exists already, ECCE only selects it and never
changes it, so you can edit and delete the examples freely. The project
remembers which version of the example sets it was made from.

Select a calculation and click **Viewer**, then open
entries from the **Properties** menu.

## The general set

| Calculation | Code | What was run | What it shows |
|---|---|---|---|
| `water` | ECCE-QM | B3LYP/6-31G*, geometry optimisation and frequencies | Energies, vibrational frequencies and IR intensities, and the atomic charges from Mulliken, Löwdin, CHELPG and Merz-Kollman (MK) fits. |
| `benzene` | ORCA | B3LYP/6-31G*, geometry optimisation and frequencies | Molecular orbitals, vibrational frequencies, and the charges from CHELPG, Hirshfeld, MBIS and natural population analysis (NPA, from NBO 7). |
| `ethanol` | ECCE-QM | B3LYP/6-31G*, geometry optimisation, frequencies and Raman activities | Vibrational frequencies with IR intensities, Raman activities and depolarisation ratios, shown in the spectrum viewer's IR and Raman panes. |
| `ammonia` | MOPAC | PM7, geometry optimisation, then frequencies | Molecular orbitals and vibrational frequencies of a semiempirical calculation. |
| `methane` | NWChem | B3LYP/6-31G*, geometry optimisation, frequencies and properties | Molecular orbitals, vibrational frequencies and Mulliken charges. |

The original output and input files are in the folder
`data/client/examples` of the ECCE installation, one folder for each set
and in it one for each example. Gaussian is not among them, because its output may not be shipped.

## How the examples were made

The programs were run with the inputs in the example folders and nothing in
the outputs was edited. The NWChem example was run with the `ecce_print`
line that ECCE's own NWChem inputs have, because ECCE reads NWChem results
from that formatted output. The ORCA example used the NBO 7 program for the
natural population analysis.

## Citation

The natural population analysis in the `benzene` example was made with NBO 7:

E. D. Glendening, J. K. Badenhoop, A. E. Reed, J. E. Carpenter, J. A.
Bohmann, C. M. Morales, P. Karafiloglou, C. R. Landis, and F. Weinhold,
NBO 7.0, Theoretical Chemistry Institute, University of Wisconsin, Madison
(2018).
