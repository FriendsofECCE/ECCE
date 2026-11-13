# Energy Calculator

**Tools > Energy Calculator...** in the Organizer gives the difference between
the energies of two sides, each a sum of finished calculations. It is how an
ionisation energy, an electron affinity, a reaction energy, a solvation energy
or the energy of a conformer relative to another is read off.

## Choosing the calculations

1. Select two calculations in the Organizer (in a project's table, with the
   Ctrl key held), then choose **Tools > Energy Calculator...**. The window opens
   with the first as the only entry of **Side A** and the second as the only
   entry of **Side B**.
2. With anything else selected, choose the calculations from the lists. They
   list the calculations of the project that holds the selection, in
   alphabetical order (capital letters and small letters count the same, and
   numbers count by value, so C2 comes before C10).
3. **Add calculation** adds an entry to a side; the **x** button removes one.
   Each entry has a sign (**+** or **-**) and a whole-number coefficient. For
   butadiene + ethene -> cyclohexene, put butadiene and ethene on side A and
   cyclohexene on side B; for 2 H2 + O2 -> 2 H2O, put H2 with coefficient 2 and
   O2 on side A, and H2O with coefficient 2 on side B. A side is the sum of its
   entries, each energy multiplied by its coefficient (and by -1 for a **-**
   entry).

Changing anything updates the result at once.

## The result

The **Energies** area is a table with the sum of side A and of side B in
Hartree, and **B - A** in kJ/mol, kcal/mol and eV:

- **E** is the final total energy of each calculation, the one the
  calculation's summary shows as **Final energy**.
- **E + ZPE**, **H** and **G** (at the temperature of the frequency
  calculations, shown in the row name) need a frequency calculation for every
  calculation on both sides; otherwise the row says "needs frequencies" and a
  note names the calculations that lack it.
- The solvent model of each side is listed under the table.

Energies in Hartree are shown to 6 decimals, differences to 1 decimal in kJ/mol
and kcal/mol and to 2 decimals in eV, which is about what the methods carry.
**Copy** puts the table on the clipboard with all digits. The Reaction Energies
panel of a transition state search uses the same rounding.

For an ionisation energy, take the neutral atom as A and the cation as B:
IE = E(cation) - E(atom). For an electron affinity, take the anion as A and the
neutral atom as B: EA = E(atom) - E(anion). A positive value in both cases means
that the energy rises from A to B.

A gas-phase and a solvent calculation of one molecule (same atoms, charge,
multiplicity, method and basis set) give **B - A** as the solvation energy, and
the window says so.

## 1 M standard state and log K

The free energies the codes store refer to an ideal gas at 1 atm. For a
reaction in solution the usual standard state is 1 mol/L. Tick **1 M standard
state (solution)**, under the table, to apply that change to the **G** row. It
is off by default and it is applied to the stored 1 atm free energies, so a
free energy that was already referred to 1 mol/L would be corrected twice.

The correction is added to **B - A** of G only:

    delta n * R * T * ln(R' * T / p0)

where delta n is the sum of the coefficients on side B minus the sum on side A
(a **-** entry counts as negative), T is the temperature of the G row, R is
the gas constant in kJ/(mol K), R' = 0.0820574 L atm/(K mol) and p0 = 1 atm.
At 298.15 K, R*T*ln(24.47) = 7.93 kJ/mol per species. A line under the table
gives delta n and the correction in kJ/mol; for delta n = 0 it says that the
correction is zero. E, E + ZPE and H are not changed.

When the G row has a value, a line under the table gives **log K**:

    log K = -(B - A of G) / (R * T * ln 10)

with the correction included when the box is ticked. **Copy** includes both
lines.

## G over treatments

All treatments use G = H − TS. They differ only in how the vibrational
entropy S (and for Truhlar and Head-Gordon also H) of the soft modes, those below about
100 cm⁻¹, is computed.

H and S come from the frequencies, each vibration treated as a harmonic
oscillator. That is good for stiff modes. For a mode near zero frequency the
harmonic entropy grows without limit (roughly as −R ln ν), and such modes
are usually torsions, methyl rotations or motions between the molecules of
a complex: poorly described as vibrations, and their computed frequencies are
the least reliable. One 15 cm⁻¹ mode can move G by several kJ/mol. This
matters most for ΔG of association (A + B → AB) and for flexible molecules.

The six treatments:

- **RRHO**: every mode harmonic; the textbook G = H − TS.
- **Grimme qRRHO, ω0 = 50, 100 or 200 cm⁻¹**: below about ω0 each mode's
  entropy is gradually replaced by that of a free rotor, which stays finite
  as ν → 0. Only S changes. 100 cm⁻¹ is Grimme's choice and ORCA's default.
- **Truhlar**: every frequency below 100 cm⁻¹ is raised to 100 cm⁻¹ before
  S, H and the zero-point energy are computed.
- **Head-Gordon**: as Grimme's, but the enthalpy of the soft modes is
  interpolated as well.

When every calculation has stored frequencies, the **G** row has an extra
column, **G spread**: the largest minus the smallest **B - A** of G over
these six treatments, in kJ/mol at 1 atm. It is a warning, not a choice to
make: a spread of about 1 kJ/mol says the soft modes do not matter for this
reaction; a spread of 10 kJ/mol or more says ΔG depends on how they are
treated, which is common for loosely bound complexes and clusters.

**G treatment** chooses what the G row itself shows: the code's own Gibbs energy (the
default) or G recomputed with one of the treatments. The recomputed numbers
come from the `ecce-thermo` command, so they equal what it prints for the same
calculations. If the stored frequencies cannot be read, a note says why and
the code's own Gibbs energy is kept.

## Warnings and notes

The **Notes** area, below the table, says when the sides are not directly
comparable. Nothing is blocked; the numbers are still shown. There is a warning
when

- the code, the method (theory), the basis set or the dispersion correction
  differs between any two of the calculations;
- the solvent model differs (except for the gas phase / solvent case above);
- the atoms on the two sides differ, so the sides are not a balanced reaction;
- the temperatures of the frequency calculations differ, so H and G are not
  comparable;
- a calculation gives a heat of formation (MOPAC) instead of a total energy, or
  an energy is the last point of a relaxed scan on one side only.

A calculation that has no final energy (it is not finished, or the code did not
report one) is named in a note and no table is made.

Energies from different methods or basis sets do not subtract to a physical
quantity. The warning is a reminder of that, and the number is still there if
the comparison is what you intend.

## Reaction overview

**Reaction overview...**, at the bottom of the Compare Energies window, lists
ΔE and ΔG for one or more reactions and draws an energy profile for each
reaction with a transition state. It starts with the reaction set up in the
window: side A as the reactants and side B as the products.

- The **Reactions** table has one row per reaction: its reactants, its
  transition state (if any) and its products. Opened on a workflow, it fills
  itself in from the workflow's reactant, transition-state and product steps,
  or, when the workflow has none of those marked, from an IRC step and the
  transition state it starts from. The × at the end of a row removes it.
- **Add Reaction...** opens a window with the calculations of the folder in
  three lists. Tick the reactants and the products and set how many of each in
  the box next to the name; choose the transition state in the middle list, or
  (none). Only a calculation with exactly one imaginary frequency can be a
  transition state; the others are greyed, and pointing at one tells how many
  it has. Select a row and press **Edit...** to change it the same way. A
  reaction with a transition state takes each species once.
- A reaction with a transition state gets an energy profile: reactants, TS and
  products as levels for ΔG
  (at the temperature in the **T / K** field) and for the electronic ΔE, both
  relative to the reactants, with the barrier ΔG‡ as an arrow and the rate
  constant k without tunnelling (TST) and with the Eckart correction beside
  it. With several such reactions, **Energy profile of** chooses which one is
  shown. The reactants, TS and product must be calculations stored on this
  machine. The rates are those of `ecce-rate`.
- **Details** below the results, open by default, lists the soft modes and the
  tunnelling corrections.
- A reaction without a transition state gets no chart; its numbers are in the text below.
- **G treatment** chooses the treatment of the low frequencies for ΔG (the
  default is Grimme's quasi-RRHO with omega0 = 100 cm⁻¹). **Update** redoes
  the numbers after the text is changed.
- The text below the profile lists for each reaction ΔE, ΔG and the spread of
  ΔG over the six treatments listed under
  [G over treatments](#g-over-treatments), the soft modes (below 100 cm⁻¹) of
  each species with their share of its entropy and of G, the profile's
  numbers, and k without tunnelling and with the Wigner and the Eckart
  correction, with κ for each. The text can be selected and copied;
  right-clicking the profile offers **Copy values** for its numbers.
- **Export CSV...** writes the numbers (with the barrier and rate columns when
  a reaction has a `[ts]`), and **Export PNG...** the energy profiles, one
  under the other. **Copy Details** puts the contents of the Details pane on
  the clipboard as plain text, for a lab report or a spreadsheet.

A reaction whose calculations differ in code, method, basis set, solvent or
dispersion correction is refused, and the text says why.
The numbers are those of `ecce-thermo --reactions`.

The **Details** pane under the table is formatted: for each reaction the soft
modes (below 100 cm⁻¹) with their share of the entropy and of the G correction,
and for a reaction with a transition state the Wigner and Eckart tunnelling
factors and the rate constants. It ends with the references for the methods
used in the results shown.

References for the methods offered:

- Transition-state theory: H. Eyring, *J. Chem. Phys.* 1935, **3**, 107–115.
- Wigner tunnelling correction: E. Wigner, *Z. Phys. Chem. B* 1932, **19**, 203–216.
- Eckart barrier: C. Eckart, *Phys. Rev.* 1930, **35**, 1303–1309.
- Grimme quasi-RRHO entropy: S. Grimme, *Chem. Eur. J.* 2012, **18**, 9955–9964.
- Truhlar quasi-harmonic treatment (low modes raised to 100 cm⁻¹): R. F. Ribeiro, A. V. Marenich, C. J. Cramer, D. G. Truhlar, *J. Phys. Chem. B* 2011, **115**, 14556–14562.
- Head-Gordon quasi-RRHO enthalpy: Y.-P. Li, J. Gomes, S. M. Sharada, A. T. Bell, M. Head-Gordon, *J. Phys. Chem. C* 2015, **119**, 1840–1850.


## Compare Forces

Compare Forces is used from the command line, with `ecce-force-compare` or
`ecce-si-export --compare-forces` (below); it has no menu item. It takes two
calculations, **A** and **B**. Both must have the same atom count
and element sequence, and a stored EGRADVEC gradient. The last stored gradient
and final geometry are used. The starting geometry is used if there is no
geometry trace. Missing forces, incompatible atoms or unknown gradient units
are reported instead of producing a comparison.

For each element, and for **all atoms**, the table gives the atom count,
maximum and RMS of the per-atom vector norm **|F(B) - F(A)|**, in eV/Å.
Forces are minus the energy gradients; 1 Hartree/Bohr = 51.42208619 eV/Å.
Use these statistics to assess cutoff, basis and pseudopotential convergence.
If the final geometries differ by more than 0.01 Å RMS per atom, a
warning reminds you that forces of different geometries are not a convergence
test. Coordinates and vectors are compared in their stored Cartesian frames,
without alignment or periodic wrapping.

With the same stored code and theory/functional (including dispersion), the
output also gives **Delta E = E(B) - E(A)** in eV per cell and meV per atom.
Different methods suppress the energy difference and leave the forces table.
Missing total energies also leave the forces table available.

The command takes stored calculation folders (or calculation URLs):

```sh
ecce-si-export --compare-forces A B
ecce-si-export --compare-forces A B --json
```

`--json` returns rows with `element`, `n`, `max` and `rms`, plus
`geometry_rms_angstrom`, `delta_e_ev`, `delta_e_mev_per_atom` and `notes`.
Unavailable energy differences are JSON `null`; refused comparisons have
`ok: false`, a `problem` and a nonzero exit status. `-o FILE` saves the output.
Older CP2K output containing only the `ATOMIC FORCES in [a.u.]` block does
not yet supply stored forces through ECCE's parser.
