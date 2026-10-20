**ECCE 9.3.0.** Please report problems as [issues](https://github.com/FriendsofECCE/ECCE/issues).

**Platforms.** Linux (Debian 13; Ubuntu 24.04, RHEL/Rocky 9 and Fedora packages are built on those systems but not tested). Windows 10 and 11. macOS, Apple silicon and Intel: experimental.

## GROMACS (#262, #334, #336, #363, #375)
- **Prepare task:** a task that builds the system for the MD tasks that follow. Sources: a PDB file (`pdb2gmx`), a mineral or clay (MINFF, or ClayFF as the force field), or a structure from the Builder. Verify shows a labelled summary, the real file names with sizes and the commands that will run; a force field can be imported (folder or zip) into a per-user library and is staged with the job.
- **Minerals (MINFF, ClayFF):** the MINFF unit cells, atomipy and `min.ff` are bundled, so this route needs no download. The editor sets layer repeats (one field per axis), ions (`genion -rmin`, a neutralise tick box, ion parameter set) and a gap for a cell that bonds to its own image along c; `min.itp` carries position restraints on heavy atoms. ClayFF parameters follow the published ClayFF set. The Layer builder in the Periodic Builder stacks a layer with ions and water, undoably.
- **Launch check:** stopping warnings and `grompp` errors are shown in a dialog with Cancel and Launch anyway; harmless warnings are not shown. Launch without a structure says what each source needs (#399).
- **Results:** one MD Summary panel with final values and run averages with their standard deviations; minimisation shows the energy and the maximum force (#334). The trajectory is kept as a geometry trace that the Builder animates (at most 100 frames). The Viewer draws the true box, including the rhombic dodecahedron and the truncated octahedron.
- **Run importer:** File > Import GROMACS Run... turns a finished run into a GROMACS MD study (#375). A protein example (the 1L2Y Trp-cage) made from a real run is imported at first start, with a first-calculation tutorial in the help.
- **MD editor:** defines, restraints, groups, barostat type and extra `.mdp` lines. A Dynamics task after Equilibrate or Dynamics opens with Continue ticked (#374).
- **Fixes:** a Prepare placed after other tasks no longer inherits their structure and topology (#397); the box is drawn around the atoms (#398); the Inputs and Run settings pages scroll instead of being cut off.

## CP2K (#329)
A new code: registration, Theory Details and Runtype Details, input generation and checking, output parsing with Progress, and reasons for failure.

## Quantum ESPRESSO
- Verify checks the sections and marks the deck lines it examined.
- On a cluster, the code's `ESPRESSO_PSEUDO` setting replaces the client's folder in `pseudo_dir` when the job runs.

## Gaussian 16: relaxed scan (#393, #396)
A scan option in the Geometry Constraints window (Scan... and Save, the scan shown in its row). The step count and step go to the `ModRedundant` line as `S n step`; the step is always written as a number with a decimal point. The Viewer has a Relaxed Scan panel with CSV export, read from the Summary of Optimized Potential Surface Scan. Verify no longer counts `ModRedundant` lines as atoms.

## Duplicate with Final Orbitals, ORCA (#75)
Duplicate with Final Orbitals starts the copy from the source calculation's MOs. The Calculation Editor's initial-guess line says when the orbital file is missing or older than the input file, and follows changes of property.

## Vibrational Frequencies (#367, #392, #369)
- The table gives each mode's symmetry and a description; frequency and description are on their own lines, and the detail line breaks at its separators.
- **Export .csv** writes the spectrum for a spreadsheet (#392).
- The panel opens on the first vibration with a 0-3400 spectrum.
- MOPAC `FORCE` normal modes are rotated into the stored geometry's frame (#369).

## Register Machines
- A Modules line on the Codes tab, with Find loading modules; one box per setting.
- The Machine tab has per-user fields: run directory and account. A job that runs on this computer shows no remote login in the Launcher.
- First start registers every code found on the PATH (#377); on macOS the login shell's PATH and the Homebrew and MacPorts folders are searched too.

## Calculation menu and Organizer
- **Gaussian 09** is no longer offered under New Calculation; existing calculations still open.
- **Double-click** on a calculation or task opens its default tool: the editor while it is being set up.
- The right-click menu offers Import Calculation from Output File.
- The Organizer detail pane shows no Basis Set button for a code whose theories need no basis set; ECCE-QM keeps it.

## MO diagram (experimental, #301, #368)
Rebuilt as a model-first fragment-orbital diagram: choose it by picking one atom or one bond, with a verdict, an About dialog and a Ligand-field view. It remains experimental and can be incomplete for larger molecules.

## Other changes
- ECCE-QM lists the spin expectation value S^2 only for an unrestricted reference, and for restricted open-shell runs (#390).
- Builder: an editable calculation opens with the Build tool shown; units appear as kJ/mol, kcal/mol and K; elements 110-118 use the IUPAC symbols.
- ECCE-QM basis menu grouped (minimal, Pople, Dunning, Karlsruhe), with def2-TZVP added.
- NWChem MD: the force-field defaults no longer depend on the `DataServers` file, and the solvent boxes are found from the job script (#380).
- Windows that fit small screens: the Basis Set Tool, the Geometry Step Plots, the Properties list and the panel decoration (#337, #335, #351, #379).
- About names the commit of a development build.

## Fixed from the 9.3 test round
- **Lattices from cell parameters were transposed** for cells without right angles (#398).
- **Relaxed scan input:** a scanned coordinate carried a value on its `ModRedundant` line (#396).
- **Prepare after other tasks** inherited their structure and topology (#397).
- **Progress plot:** a plot is drawn again from two points (#400); a time estimate needs two finished steps.
- **Help pages:** the GROMACS and Quantum ESPRESSO walkthroughs are reworded to match the windows.

## Known issues
- **GROMACS, CP2K and Quantum ESPRESSO** remain experimental.
- **The MO diagram** is experimental.

## Packages

| System | Files |
|---|---|
| Debian 13 | `ecce-client_9.3.0_amd64.deb`, `ecce-server_9.3.0_amd64.deb` |
| Ubuntu 24.04 | `ecce-client_9.3.0_ubuntu24.04_amd64.deb`, `ecce-server_9.3.0_ubuntu24.04_amd64.deb` |
| RHEL/Rocky 9 | `ecce-client-9.3.0-1.el9.x86_64.rpm`, `ecce-server-9.3.0-1.el9.x86_64.rpm` |
| Fedora | `ecce-client-9.3.0-1.fedora.x86_64.rpm`, `ecce-server-9.3.0-1.fedora.x86_64.rpm` |
| macOS 11 or newer (experimental) | `ECCE-9.3.0-arm64.dmg` (Apple silicon), `ECCE-9.3.0-x86_64.dmg` (Intel) |
| Windows 10/11 | `ECCE-windows-9.3.0.msi` (installer) or `ECCE-windows-9.3.0.zip` |

Installation: [Installing ECCE](https://github.com/FriendsofECCE/ECCE/blob/main/docs/INSTALLING.md). Videos: [chem4eva on YouTube](https://www.youtube.com/@chem4eva).
