**ECCE 9.5.1**, a patch release of 9.5.0. Please report problems as [issues](https://github.com/FriendsofECCE/ECCE/issues).

**Platforms.** Linux (Debian 13; Ubuntu 24.04, RHEL/Rocky 9 and Fedora 44 packages are built on those systems but not tested). Windows 10 and 11. macOS, Apple silicon and Intel: experimental.

## Changes
- **Vibrational Frequencies panel, redesigned:** the spectrum or the table takes most of the panel, with the selected mode in full under it (frequency, irrep, IR and Raman intensity and its contributions), and one **Mode in the viewer** group: Show, Animation or Vectors, arrow length, and Reverse. The panel plot shows IR or Raman, chosen with the **IR | Raman** switch. The spectrum settings and the other spectra are in the Spectrum window (#845).
- **Spectrum window** (Open in window...): IR and Raman stacked, the selected mode in full at the bottom again, and the settings: width, scale factor, sticks, axis direction, range. **Compare...** overlays another calculation's spectrum or opens the band matching; Difference, Experimental..., Remove and Export .csv... are here too. When a second calculation, an experimental spectrum or a difference is loaded, the window shows one kind at a time with the IR | Raman switch (#846, #845).
- **Surfaces** is one entry under Orbitals with a drop-down (Density, Spin Density, ESP on Density, ELF, LOL, NCI) and no orbital table; the MOs drop-down lists orbitals only (#842).
- **Density difference:** the Density Map and Density Profile show the density of a complex minus that of its fragments (#726).
- **Thermochemistry panel:** the row labels are no longer cut off (#844).
- **Recipes and workflows:** a step gets the structure of its completed source before its editor opens; before, the next step's editor said there was no structure. The help has a walk-through of a workflow made from a recipe (#847). CREST steps start from an xTB optimisation in the recipes and in New step, and launching a CREST calculation with no optimisation before it gives a warning (#840).

## Known issues
- **Windows:** uninstalling takes long (#815); a few items are off at 150-200 % display scaling (#670); the first start takes about a minute on slower computers (#444).
- **GROMACS, CP2K, Quantum ESPRESSO** and the **ML potential** are experimental. **The MO diagram** is experimental.

The bundled atomipy (1.1) and min.ff (commit d12b895b) are those of 9.5.0. The newest min.ff renames its parameter files, which ECCE does not follow yet.
