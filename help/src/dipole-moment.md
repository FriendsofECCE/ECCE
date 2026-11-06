# Dipole moment

The **Dipole Moment** panel (Properties list, with the other molecular
properties) shows the three components of the dipole moment in debye and draws
the moment in the 3-D view.

The arrow runs from the **negative pole** to the **positive pole**, along
the dipole moment the code reports, the direction the IUPAC definition gives
the dipole moment. A small maroon sphere marks each pole. The atomic charges
say how much charge the poles carry: each carries the total positive charge
of the atoms, ±q. The poles are as far apart as that charge must be to give
the dipole moment, d = μ/q, and they lie centred between the centres of the
negative and of the positive charges. The text under the table gives q and
d, for example **The poles carry ±1.08 e (the positive Mulliken charge) and
are 0.30 Å apart**.

Atomic charges alone can point some way off the dipole moment, because point
charges on the atoms leave out how each atom's electron cloud is shifted. In
carbon monoxide they even point the other way: the charges make oxygen
negative, but the measured dipole moment, and the one most methods give, has
carbon at the negative end, where the lone pair outweighs the charge carbon
gives to oxygen. The arrow always follows the dipole moment, and the text in
the corner adds when the charges point the other way, for example **Dipole
moment μ = 0.06 D (ESP charges point the other way)**. With very small
charges, as in CO, the poles end up far apart: little charge must be
separated far to give even a small moment. ESP-fitted charges (CHELPG,
Merz-Kollman) follow the dipole moment more closely than Mulliken charges and
are used when the calculation has them.

The atomic charges are the CHELPG (ESP-fitted) charges when the calculation has
them, then Merz-Kollman charges, otherwise Mulliken charges. The panel names the
set it used. A calculation with no atomic charges gets a 1 Å arrow through the
centre of nuclear charge. The arrow colour is set with the colour button.

The arrow is drawn through atoms and bonds, so a diatomic's arrow can be seen
inside its bond.

The dipole moment of an ion depends on the origin it is measured from, so the
value is only meaningful together with the origin. ECCE-QM reports an ion's
dipole moment about the centre of mass (the panel says so next to the value);
for a neutral molecule the origin does not matter. ORCA's output states its
electric origin, which ECCE shows when the output gives it. Gaussian 16 does not
state the origin in its output; its default for the dipole moment is the centre of
nuclear charge, so ion dipole moments from different codes are not directly
comparable.

## Bond dipoles

The **Bond dipoles** box (off by default) splits the dipole moment into one
dipole per bond, from the atomic charges of the set chosen next to it
(Mulliken, Löwdin, Hirshfeld, CHELPG or Merz-Kollman, whichever the
calculation stores). The partition depends on the charge set, and the panel
names the one used.

Each atomic charge is written as a sum of charge transfers along its bonds,
q<sub>i</sub> = Σ<sub>j</sub> δ<sub>ij</sub> with δ<sub>ji</sub> = −δ<sub>ij</sub>.
For a molecule without rings the transfers are fixed by the charges. For rings
they are not, and the smallest set of transfers (the minimum-norm solution) is
taken, so no charge circulates round a ring. The bond dipole is
μ<sub>ij</sub> = δ<sub>ij</sub> (r<sub>i</sub> − r<sub>j</sub>), and the bond
dipoles add up exactly to the point-charge dipole Σ q<sub>i</sub> r<sub>i</sub>.

The table gives each bond (written from the negative to the positive atom), |μ|
in D and the direction as a unit vector, then these lines:

- **Sum of bonds**, the bond dipoles added as vectors.
- **Net-charge term**, only for a charged molecule: a net charge cannot be
  written as transfers between bonded atoms, so its share is kept apart.
- **Point charges**, Σ q<sub>i</sub> r<sub>i</sub>. For an ion the origin is the
  centre of mass, as for the dipole moment; the bond dipoles do not depend on it.
- **Computed**, the dipole moment the calculation reports.
- **Remainder**, computed minus point charges (computed minus the sum of
  bonds for a neutral molecule). It is the part atomic charges cannot carry:
  lone pairs and atomic densities that are not spherical.

In the viewer each bond gets a opaque purple arrow, slightly wider than the bond so that it sheathes it, in the same direction as the
molecular dipole arrow (negative to positive). The arrows share one scale: the
largest bond dipole fills its bond, the others follow |μ|, each is centred on
its bond and kept inside it. A very small bond dipole still gets a short arrow,
unless the table shows 0.00 D.
