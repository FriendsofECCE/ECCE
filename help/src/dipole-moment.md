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
