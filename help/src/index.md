# Getting started

ECCE is the Extensible Computational Chemistry Environment, pronounced
"etch-ā". *Ecce* is Latin for "behold".

This chapter takes you from a package to a finished calculation. If you
only want to look at a structure or at existing results, you can stop after
the second page.

## First steps

1. At the first question, choose **Store data on this computer**. See
   [Installation](installation.md).
2. Choose **File > Open Example Calculations** to get finished calculations
   to look at. See [Example calculations](example-calculations.md).
3. Choose **File > New > Project...** (or right-click **Local data** and
   choose **New... > Project...**). A project is a folder. Select the project
   and choose **File > New** again to create a calculation in it. See
   [Your first calculation](first-calculation.md).
4. ECCE-QM is built in. For other codes, see
   [Running codes on Linux](running-codes-on-linux.md),
   [macOS](running-codes-on-macos.md) or
   [Windows](running-codes-on-windows.md).
5. Register a code under **Tools > Register Machines...**. See
   [Machines](machines.md).
6. **Help > ECCE Help** (Ctrl+H) opens this help.
7. **Help > Report a Problem...** builds an archive with diagnostic data to
   attach to a bug report. See
   [Installation](installation.md#reporting-a-problem).

If something does not work, choose **Help > Report a Problem...**. It builds
a zip file with the ECCE version, your system and the ECCE logs. Attach the
zip to a new issue at <https://github.com/FriendsofECCE/ECCE/issues> and
describe what you did and what happened.

1. [Installation](installation.md): which packages to install, and what you
   see when you start ECCE for the first time.
2. [Importing a structure](importing-a-structure.md): open a structure file
   or take one from the Structure Library, with no compute machine and no
   code installed.
3. [Importing a calculation](importing-a-calculation.md): look at the results
   of a calculation that ran elsewhere.
   [Example calculations](example-calculations.md) are four finished ones
   shipped with ECCE, for the same purpose.
4. [Machines](machines.md): where calculations run, and how to tell ECCE
   where a code is installed.
5. [Your first calculation](first-calculation.md): a geometry optimisation
   of water with NWChem, from a new project to the results; also the
   plane-wave (NWPW) theories PSPW and BAND of NWChem.
6. [Your first Quantum ESPRESSO calculation](qe-first-calculation.md): bulk
   silicon and a water molecule in a box, with a periodic cell, plane waves
   and pseudopotentials.
   [Choosing Quantum ESPRESSO settings](qe-choosing-settings.md) says how to
   pick the cutoff, the k-points, smearing and the box.
   [CP2K: the SCF loop](cp2k-scf.md) explains the OT minimiser and the outer
   SCF loop.
   [CP2K: basis sets and potentials](cp2k-basis-potential.md) explains the
   basis level, the library files and the per-element overrides.
   [xTB: tight-binding calculations with xtb](xtb.md): GFN2-xTB, GFN1-xTB and
   GFN-FF energies, optimisations and molecular dynamics.
   [CREST: conformer search and related run types](crest.md): the low-energy
   conformers, protonation sites and tautomers of a molecule, a non-covalent
   complex, and a solvation shell, shown in the Viewer.
   [ML Potential: a machine-learned potential (MACE)](mlp.md): screening
   energies and relaxations of periodic cells.
   [Your first GROMACS calculation](gromacs-first-calculation.md): a small
   protein through Prepare, Optimize, Equilibrate and Dynamics.
7. [Molecules for teaching](molecules-for-teaching.md): the small molecules
   of a first laboratory, ready in the Structure Library at their measured
   bond lengths.
   [Arranging the Viewer's panels](viewer-panels.md): take a panel out into a
   window of its own when its plot or table needs more room.
   [How far the atoms moved](displacement.md): displacement from the first
   frame, colour by displacement, and the aligned difference between two
   calculations.
   [Compare Energies](compare-energies.md): the selected calculations, or
   those of a folder, from the lowest energy up, dE and dG in kJ/mol and
   eV, mismatches flagged.
   From the command line: [Export for Supporting Information](si-export.md),
   [Checks table](checks-table.md) and
   [Excluded calculations](excluded.md); see
   [Scripting and AI assistants](scripting-and-ai.md).
8. [Constraints and dummy atoms](constraints-and-dummy-atoms.md): hold a
   bond, an angle or a torsion fixed during an optimisation, and use a dummy
   atom as a reference point; the potential of ammonia as the example.
9. [Organometallic complexes: metal-π bonds](organometallics.md): nubs, σ
   ligands, and π ligands such as cyclopentadienyl and benzene bonded to a
   metal with the Metal order.
10. [Running codes on Linux](running-codes-on-linux.md): NWChem, MOPAC,
   GROMACS, Quantum ESPRESSO and CP2K from the package archive; ORCA,
   Gaussian and NBO from their makers.
11. [Running codes on macOS](running-codes-on-macos.md): MOPAC, ORCA and NWChem
   on a Mac, without administrator rights, and Open MPI for ORCA on more
   than one core.
12. [Running codes on Windows](running-codes-on-windows.md): MOPAC and ORCA
   on a Windows PC, and Microsoft MPI for ORCA on more than one core.
13. [Preparing a protein for GROMACS](gromacs-prepare.md): from a PDB file
   to a solvated, neutral system ready for minimisation, with the GROMACS MD
   Prepare task.
14. [Stacking layers: the Layer Builder](layer-builder.md): build graphite,
   layered oxides or a clay from one layer, with a spacing, a sideways shift,
   and ions and water between the layers.
15. [GROMACS MD: run settings](gromacs-md-settings.md): defines and position
   restraints, groups by residue name, the barostat and its coupling type,
   and extra `.mdp` lines.
16. [Bonds in the Builder](bonds-and-clean.md): bonds have no order; the atom's
   geometry decides, and what Clean assumes.
17. [The Density Map](density-map.md): the electron density or one orbital as
   contours on a plane through the molecule; the plane, the offset, picking
   atoms, reading the map and exporting it.
18. [ELF, LOL and NCI in the 3-D view](electron-density-views.md): where
   electrons are localised, and the weak interactions between molecules
   coloured by their strength; what the surfaces and colours mean and the
   isovalue and density cut-off boxes.
19. [The MO diagram](mo-diagram.md): the orbitals of a calculation as levels
   joined to the orbitals of its parts; what the labels, the star, nb and the
   line weights mean.
20. [Mode composition](mode-composition.md): what each normal mode is made of
   (stretches and bends), the amber highlight in the spectrum, and why it is
   not a full PED.
21. [Localised orbitals](localised-orbitals.md): the occupied orbitals as bonds,
   lone pairs and core orbitals, and how to read their names.

22. [Density profile](density-profile.md): the electron density and each
   orbital's share of it along a line through two atoms, or averaged around one
   atom: shells, penetration, nodes and the build-up in a bond.

23. [Dipole moment](dipole-moment.md): the arrow from the negative to the
   positive pole, its label, and which atomic charges place the poles.

24. [Speeds and diffusion](md-speeds-and-diffusion.md): the distribution of
   atomic speeds of a dynamics run against Maxwell-Boltzmann, and the
   mean-square displacement with the diffusion coefficient.

25. [Energy Calculator](energy-calculator.md): the difference between the final
   energies of two calculations in Hartree, kJ/mol, kcal/mol and eV, and the
   notes on differing code, method or basis set.

24a. [Thermochemistry](thermochemistry.md): H, S and G recomputed from the
   stored frequencies with a chosen low-frequency treatment, temperature and
   standard state.

26. [Reconnect Selected](reconnect-selected.md): reconnecting several jobs
   to their calculations after a restart or a lost connection, in one step.

27. [Launch Selected](launch-selected.md): submitting several calculations
   in one step, with their stored settings or on another machine.

28. [Run states](run-states.md): what Created, Running, Completed,
   Incomplete, Killed, Monitor Error, System Error and the other states
   mean, and where the reason for an ended job is shown.

28a. [The Organizer's right-click menus](organizer-menus.md): what a calculation's
   menu offers, Compare Energies, and the project table's columns.

29. [Re-Parse Outputs](reparse-outputs.md): running the parsers again over the
   outputs stored with a calculation, without logging in to the machine.

30. [Allocations](allocations.md): the usage of each cluster account, from
   Tools > Allocations.

31. [Workflows](workflows.md): calculations that start from the geometry of
   an earlier one (or from a point of a scan, or several for a TS search),
   drawn as a tree, with Launch Ready Steps.

32. [Renaming and moving calculations](rename-and-move.md): Rename, Cut and
   Paste, dragging, and why a calculation with a running job stays where it is.

33. [Scripting and AI assistants](scripting-and-ai.md): the command-line
   tools, and the AGENTS.md file that tells a script or an assistant how ECCE
   stores its data and how to work with it.

34. [Recipes](recipes.md): workflow templates for a kind of study (TS search with
   IRC, conformers), the set-up panel with its preview, the key-data box,
   Save Workflow as Recipe, and recipes for a site.

35. [Writing a recipe](writing-a-recipe.md): the file format, every field, the
   folders recipes are read from, and a complete example.

36. [New Series](new-series.md): one calculation's setup applied to a list of
   structures (xyz files or calculations), with `ecce-series`.

Reference:

- [ORCA composite methods](orca-composite-methods.md): r2SCAN-3c, B97-3c,
  PBEh-3c, wB97X-3c and HF-3c, which bring their own basis set and dispersion.
- [How to cite ECCE](citing-ecce.md): the DOI, a BibTeX entry, and the
  original PNNL papers.
- [ECCE-QM](ecce-qm.md): the built-in Hartree-Fock and density functional
  program: methods, basis sets, run types, limits and how it was checked.
- [ECCE-QM input and output files](ecce-qm-files.md): every keyword of the
  built-in engine's input, and what its output file contains.

- [Register Machines](register-machines.md): the window's tabs, the site
  and user layers of each setting, and examples of common changes.

- [The results file format](results-file-format.md): what results.h5 holds,
  the orbital order, the ecce-results commands and which codes are covered.

25. [Match bands](match-bands.md): which band of one calculation is which band
   of another, the shift of each, and how the modes are matched.

26. [Difference and experimental spectra](ir-spectrum-difference-experiment.md):
   zoom to a range, the difference curve of two calculations, an imported
   absorbance or transmittance spectrum and the CSV export.
