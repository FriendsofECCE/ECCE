# Thermochemistry

The **Thermochemistry** panel of a calculation that has a Gibbs energy shows
enthalpy (H), entropy (S) and Gibbs energy (G) recomputed from the stored
frequencies, next to the values the code itself printed. The code's own values
are not changed.

## Choosing the treatment

- **Treatment** is how the low frequencies are handled: RRHO (every mode as a
  harmonic oscillator); Grimme's quasi-RRHO entropy with omega0 = 50, 100 or
  200 cm⁻¹ (the entropy of modes below omega0 is blended into a free rotor);
  Truhlar's treatment (modes below 100 cm⁻¹ are raised to 100 cm⁻¹); or
  Head-Gordon's quasi-RRHO enthalpy and entropy. ORCA's own G uses Grimme with
  omega0 = 100 cm⁻¹.
- **Temperature (K)** starts at the temperature of the calculation. Press Enter
  or leave the field to recompute.
- **Standard state** is 1 atm (gas) or 1 M (solution).
- **Symmetry number** is the rotational symmetry number used for the rotational
  entropy. Leave it empty to use the one the code printed (ORCA, Gaussian and
  ECCE-QM)
  or, where the code's output is not read for it, the one that follows from the
  stored point group. A number here replaces it.
- **Rotor** is the rotor type: As found (from the geometry), Linear or
  Non-linear. Linear can be chosen for a structure that is only slightly bent;
  Non-linear cannot be chosen for a molecule that is exactly linear.

The first line of the notes gives the point group, the symmetry number used and
where it came from, and the rotor type. If the structure is within 0.05 Å of a
point group with a larger symmetry number (or a linear one) than was used, the
notes say so and give that group's number. A structure a hair off its symmetry
otherwise gets too small a symmetry number and a G that is too high.

The table has two columns: **Recomputed (ecce-thermo)** for the choice above,
and the code's own stored values. Notes from the calculation, such as a
symmetry number that had to be assumed, appear above the table. If there are no
stored frequencies the panel says so and shows no recomputed numbers.

## Contributions and heat capacities

A second table, below the first, splits the recomputed numbers by where they
come from, for the same treatment, temperature, standard state, symmetry
number and rotor:

- **U (kJ/mol)** is the thermal energy above the electronic energy, without the
  zero-point energy: translational 3/2 RT, rotational RT (linear) or 3/2 RT
  (non-linear), 0 for an atom, and the vibrational part, which grows with T.
- **U + ZPE (kJ/mol)** adds the zero-point energy to the vibrational row and
  the total.
- **S (J K⁻¹ mol⁻¹)** is the entropy by contribution; the electronic part is
  R ln(2S + 1).
- **C_V (J K⁻¹ mol⁻¹)** is the heat capacity at constant volume: 3/2 R
  translational, R or 3/2 R rotational, and the vibrational part, near zero for
  a stiff vibration at room temperature (N₂) and approaching R per vibration at
  high temperature (I₂). The electronic part is zero, since only the ground
  state is counted.
- **C_P** (total only) is C_V + R, for an ideal gas.

The vibrational row follows the treatment: Truhlar's raises the low modes to
100 cm⁻¹ for U, S and C_V; Head-Gordon's damps the thermal energy of the low
modes toward RT/2 and C_V toward R/2; Grimme's changes only S.
Changing the temperature or any other choice recomputes both tables.

The numbers come from the `ecce-thermo` command, so running it on the same
calculation gives the same values; `ecce-thermo --contributions` prints the
second table as text, and `--json` always includes it. For the spread of a reaction G over the
treatments, see [Energy Calculator](energy-calculator.md).

## Reaction rates

From a reactant, a transition-state and (optionally) a product calculation that
all have stored frequencies, the `ecce-rate` command gives the rate constant of
the step by conventional transition-state theory:

    ecce-rate REACTANT[,REACTANT2] TS [PRODUCT[,PRODUCT2]] --T 250:400:25

The activation free energy dG, the activation enthalpy and entropy come from the
same thermochemistry as above (same treatment, temperature and standard state),
and k = kappa sigma (kB T/h) c0^(1-m) exp(-dG/RT), where m is the number of
reactants. The standard state gives the unit: s-1 for one reactant, M-1 s-1
(1 M) or cm3 molecule-1 s-1 (1 atm) for two. sigma is the reaction-path
degeneracy (`--sigma`, default 1); the rotational symmetry numbers of the
molecules are already in G.

The transition state must have exactly one imaginary frequency; with none or
several, `ecce-rate` stops and says so. A reactant or product with an imaginary
frequency is not a minimum and gives a warning.

kappa corrects for tunnelling. **Wigner**: kappa = 1 + (h nu / kB T)^2 / 24
with nu the imaginary frequency; it is reliable only when h nu / kB T is below
about 4. **Eckart**: the transmission through an Eckart barrier whose
width follows from the imaginary frequency and whose forward and reverse
heights are the barriers between the zero-point-corrected energies, averaged
over a Boltzmann distribution of energy. Without a product calculation the
barrier is taken as symmetric, and the output says so. With a product the
output also has the reverse rate constant and the equilibrium constant, which
agree with the reaction G, and over a range of temperatures the Arrhenius
(A, Ea) and Eyring (activation enthalpy and entropy) fits. The half-life is
ln 2 / k; for two reactants it is for the second one held at 1 M (or 1 atm).

Conventional theory has no recrossing or variational correction, and the
tunnelling factor is for a one-dimensional barrier. For a barrier near zero or
a very flat potential the numbers are an estimate only.
