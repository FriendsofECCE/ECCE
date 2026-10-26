# Example calculations

ECCE ships the outputs of five finished calculations, so that you can look
at orbitals, charges and vibrations without running anything and without any
computational chemistry code installed.

## Open them

The first time you start ECCE, the Organizer imports the Getting Started set
of examples by itself into the project `Examples/Getting_Started`. It does
this once: if you delete the project, it does not come back at the next
start, and an `Examples` project you already have is left as it is.

The examples come in sets. To import another set, or to find the first one
again, choose **File > Open Example Calculations** in the Organizer. A small
window lists the sets with a line on what each holds; choose one and click
**OK**. ECCE imports the set into its own project in the project `Examples`, as
described in [Importing a calculation](importing-a-calculation.md). The
calculations show the run state "Imported" and take a few seconds to be read.

Each set is divided into topics, which are projects of their own:
`Examples/<Set>/<Topic>/<calculation>`. Project names have no spaces, so a
space in a set or topic name shows as an underscore.

| Set | Project | Holds |
|---|---|---|
| Getting Started | `Examples/Getting_Started` | The first calculation in each code: the five calculations below. |
| Computational | `Examples/Computational` | One showcase job per code, in a topic for each code: a GROMACS molecular dynamics study of the Trp-cage (topic GROMACS). |
| General Chemistry | `Examples/General_Chemistry` | Ninety-four ECCE-QM calculations in the topics Atoms (the neutral atoms hydrogen to argon), Ionisation energies and electron affinities (the cations and anions), Koopmans' theorem (Hartree-Fock orbital energies of the atoms), Molecular orbitals of diatomics, Polar bonds and dipole moments, VSEPR, and Lewis structures and resonance, and, in the topic Gases, liquids and solids, seven GROMACS molecular dynamics runs: argon gas and helium gas (300 K, 20 bar, 1000 atoms), liquid argon (85 K), SPC/E water, OPLS-AA decane, a NaCl crystal dissolving in water, and ice melting and evaporating as it is heated. The gases and liquids show the distribution of atomic speeds and the mean-square displacement, in the Speed Distribution and Displacement (MSD) panels described in [Speeds and diffusion](md-speeds-and-diffusion.md); use **Compare...** in those panels to draw one run over another. Each run holds its last structure, 26 frames of the trajectory (NaCl: 101 frames over the first 3 ns, while the crystal dissolves) and the energy plots of its log. |
| Organic Chemistry | `Examples/Organic_Chemistry` | Twenty-one calculations in the topics Bonding in hydrocarbons, Conformations and Reactions: ECCE-QM ethane, ethene and ethyne with localised orbitals; ORCA benzene; the Diels-Alder reaction (butadiene, ethylene, cyclohexene, transition state, IRC) and an SN2 reaction (reactants, complexes, transition state, IRC, products); and four ECCE-QM relaxed torsion scans (B3LYP/6-31G*) shown in the Relaxed Scan panel: ethane, butane, and N-methylacetamide scanned in both directions, whose two curves differ (hysteresis). |
| Spectroscopy | `Examples/Spectroscopy` | Nineteen calculations in the topics Sulfate coordination, Carbonyl compounds, Amides and peptides, Sugars and Small molecules: the S–O stretches of sulfate free, bound to cobalt through one oxygen (with ammine or with ethylenediamine and chloride ligands), chelating and bridging; carbonyl stretches with substituent and charge (formaldehyde, acetaldehyde, acetone, acetic acid, acetate, three esters); the amide bands of N-methylacetamide and of Ac-Ala-NHMe; α-D-glucopyranose and α-D-galactopyranose; and the IR spectrum of water and ethanol. The ORCA calculations use PBE0-D3BJ/def2-SVP, with frequencies, IR and Raman intensities and molecular orbitals. |

If the project of a set exists already, ECCE only selects it and never
changes it, so you can edit and delete the examples freely. The project
remembers which version of the example sets it was made from. If your
`Examples` projects are from an earlier version of ECCE, in which each set was
a project of its own (`Examples_Organic` and so on), ECCE replaces the example
calculations in them with the sorted ones at the first start; calculations of
your own that you had put there stay where they are.

Select a calculation and click **Viewer**, then open
entries from the **Properties** menu.

## The Getting Started set

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
