# Dipole moment

The **Dipole Moment** panel (Properties list, with the other molecular
properties) shows the three components of the dipole moment in debye and draws
the moment in the 3-D view.

The arrow runs from the **negative pole** to the **positive pole**, the
direction the IUPAC definition gives the dipole moment. The poles are the
centres of the negative and of the positive partial charges of the atoms: each
is the average of the atom positions of one sign, weighted by the charges. The
arrow lies along the dipole moment the code reports, and its tail is at the
negative pole. Where the two poles are not exactly on the line of the dipole
moment (the atomic charges do not reproduce it), the head is level with the
positive pole along that line.

The length of the arrow is the distance between the poles. It follows the
geometry of the molecule and does not scale with the size of the moment; a
water molecule gives a short arrow. The magnitude is in the upper right corner
of the 3-D view, for example **Dipole moment 1.79 D (ESP charges)**, and
disappears when the panel is left. When the moment is zero, as for carbon
dioxide, there is no arrow and the text reads **Dipole moment 0 D**.

The atomic charges are the CHELPG (ESP-fitted) charges when the calculation has
them, then Merz-Kollman charges, otherwise Mulliken charges. The panel names the
set it used. A calculation with no atomic charges gets a 1 Å arrow through the
centre of nuclear charge. The arrow colour is set with the colour button.

The arrow is drawn through atoms and bonds, so a diatomic's arrow can be seen
inside its bond.
