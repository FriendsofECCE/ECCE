**ECCE 9.4.11**, a patch release of 9.4.10. Please report problems as [issues](https://github.com/FriendsofECCE/ECCE/issues).

**Platforms.** Linux (Debian 13; Ubuntu 24.04, RHEL/Rocky 9 and Fedora 44 packages are built on those systems but not tested). Windows 10 and 11. macOS, Apple silicon and Intel: experimental.

## Changes
- **Bond dipoles:** Properties > Dipole Moment can show the bond dipoles that make up the molecular dipole, from a chosen set of atomic charges (Mulliken, Löwdin, Hirshfeld, ESP-fitted), as arrows on the bonds and as a table with the sum of the bonds beside the computed dipole (#849).
- **New examples:** General Chemistry > How a chemical bond forms (the H₂ energy curve, with a Morse fit) and Computational chemistry > H₂ dissociation (RHF, UHF and CCSD); both now carry their molecular orbitals, so the σg and σu* orbitals can be shown at any point (#858). General Chemistry > Packing in solids: simple cubic, body-centred cubic, face-centred cubic and hexagonal close-packed metals, NaCl and CsCl, each as its conventional cell (#859, #916). Quantum ESPRESSO pw.x outputs can be imported (#447).
- **ma-def2-SVP** is in the basis-set library, so ORCA calculations that use it (the SN2 and halide examples among them) show their basis set (#898).
- **Dielectric constant without a unit:** the NWChem, Gaussian 03 and Gaussian 09 solvent settings labelled the dielectric constant in Debye; it is now εᵣ without a unit. Saved calculations keep their solvent (#848).
- **Orbital and density surfaces** cover every atom of the molecule, not only the selected ones (#892). **Periodic structures:** the cell and its a/b/c labels follow the atoms when they move or are replicated (#723). **Project table:** right-clicking one calculation shows the Run submenu again (#875).

## Known issues
- **Titanium** is not among the atoms: its restricted open-shell calculation does not reliably reach the 3d² 4s² ground state (#797).
- **HiDPI on Windows:** a few items are still unchecked or off at 150-200 % (#670). **Windows:** uninstalling takes long (#815); the first start takes about a minute on slower computers (#444).
- **GROMACS, CP2K and Quantum ESPRESSO** remain experimental. **The MO diagram** is experimental.
