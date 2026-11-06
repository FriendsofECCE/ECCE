# Thermochemistry

The **Thermochemistry** panel of a calculation that has a Gibbs energy shows
enthalpy (H), entropy (S) and Gibbs energy (G) recomputed from the stored
frequencies, next to the values the code itself printed. The code's own values
are not changed.

## Choosing the treatment

- **Treatment** is how the low frequencies are handled: RRHO (every mode as a
  harmonic oscillator); Grimme's quasi-RRHO entropy with omega0 = 50, 100 or
  200 cm-1 (the entropy of modes below omega0 is blended into a free rotor);
  Truhlar's treatment (modes below 100 cm-1 are raised to 100 cm-1); or
  Head-Gordon's quasi-RRHO enthalpy and entropy. ORCA's own G uses Grimme with
  omega0 = 100 cm-1.
- **Temperature (K)** starts at the temperature of the calculation. Press Enter
  or leave the field to recompute.
- **Standard state** is 1 atm (gas) or 1 M (solution).

The table has two columns: **Recomputed (ecce-thermo)** for the choice above,
and the code's own stored values. Notes from the calculation, such as a
symmetry number that had to be assumed, appear above the table. If there are no
stored frequencies the panel says so and shows no recomputed numbers.

The numbers come from the `ecce-thermo` command, so running it on the same
calculation gives the same values. For the spread of a reaction G over the
treatments, see [Energy Calculator](energy-calculator.md).
