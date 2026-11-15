# Natural population analysis (NPA)

ECCE computes natural charges itself from a calculation's stored orbitals (`results.h5`), so NPA charges are available without the NBO program. The method is natural population analysis as published by Reed, Weinstock and Weinhold (J. Chem. Phys. 83, 735 (1985)). The same definition is used for every code whose results file has orbitals in a Gaussian basis: ECCE-QM, ORCA, Gaussian 16, NWChem and others.

## In the Viewer

1. Open the calculation in the Viewer.
2. In the **Properties** list, Charges group, choose **Natural (NPA) Charges, computed by ECCE**.

The first time, ECCE computes the charges (under a second for course-size molecules) and stores them with the calculation, so later selections show them at once. They are shown and coloured on the atoms like the other charge sets; **Assign Charges** puts them on the atoms.

If the code ran NBO itself (ORCA with NBO, Gaussian with `Pop=NBO`), the list also has **Natural (NPA) Charges**: those are NBO's numbers. The two rows use the same definition and normally agree to about 0.001 e.

The row is not offered for calculations without stored orbitals, and not for semiempirical or periodic ones (see below).

## From a terminal

Give the calculation's URL or folder, or a `results.h5` file:

    ecce-npa <calculation>
    ecce-npa <calculation> --json

The table lists, per atom, the natural charge and the natural population split into core, valence and Rydberg orbitals. An open-shell calculation also gets a **Spin** column (alpha minus beta population). Below the table is each atom's natural electron configuration, for example `[core]2s(1.75)2p(5.12)3p(0.01)`. `--json` gives the same numbers, with the population of every shell.

## What it does

1. The density of each spin is built from the stored orbitals and occupations, and the overlap of the basis functions is computed.
2. For each atom and angular momentum, the density block is averaged over the 2l+1 components and diagonalised: these are the pre-orthogonal natural atomic orbitals and their occupancies.
3. The most occupied of them form the natural minimal basis: the shells the free atom occupies. With an effective core potential only the valence shells are in the basis, and the core electrons the potential replaces are not counted in the population. The remaining orbitals are the Rydberg set.
4. The orbitals are made orthogonal between atoms, each weighted by its occupancy so that strongly occupied orbitals change least. Core orbitals are orthogonalised first, then the valence orbitals of the minimal basis, then the Rydberg orbitals.
5. Each atom's blocks are diagonalised again: the natural atomic orbitals. An atom's natural population is the number of electrons in its natural atomic orbitals. Its charge is the nuclear charge, less any ECP core, minus that population. An open-shell calculation uses the natural atomic orbitals of the total density, and its alpha and beta populations give the spin populations.

## Accuracy

The charges were checked against NBO 7's NPA on the same ORCA wavefunctions (water, ethene, s-cis butadiene, formaldehyde, acetate, the methyl radical, iodomethane with a def2 ECP, and [Fe(H2O)6]2+), and against the NBO built into Gaussian 16 on Gaussian 16 wavefunctions (water, iodomethane): every charge agrees within 0.005 e.

## Not available

- Semiempirical calculations (MOPAC, xTB): their basis is orthonormal, so natural populations equal the Löwdin populations.
- Periodic calculations (Quantum ESPRESSO, CP2K with a cell).
- Basis sets with Cartesian d or f functions.

In these cases `ecce-npa` says why in one line and gives no table.

ECCE records which atoms have an effective core potential, and how many core electrons it replaces, in the results file, from the stored basis set or from the code's output. For older results files that lack this, ECCE assumes the def2 (Stuttgart) core sizes when the electron count shows that a core is missing, and says so above the table.
