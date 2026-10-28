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

The atomic charges can point the other way from the dipole moment. Carbon
monoxide is the best-known case: the charges make oxygen negative, but the
measured dipole moment, and the one most methods give, has carbon at the
negative end. Point charges on the atoms leave out how each atom's electron
cloud is shifted, and in CO the lone pair on the far side of carbon outweighs
the charge carbon gives to oxygen. The dipole moment is then the one to
believe. The arrow still follows it and runs between the two poles, from the
charges' positive pole to their negative one, and the text in the corner adds
that the charges point the other way, for example **Dipole moment μ = 0.06 D
(ESP charges point the other way)**.

The length of the arrow is the distance between the poles. It follows the
geometry of the molecule and does not scale with the size of the moment; a
water molecule gives a short arrow. The magnitude is in the upper right corner
of the 3-D view, for example **Dipole moment μ = 1.79 D (ESP charges)**, and
disappears when the panel is left. When the moment is zero, as for carbon
dioxide, there is no arrow and the text reads **Dipole moment μ = 0 D**.

The atomic charges are the CHELPG (ESP-fitted) charges when the calculation has
them, then Merz-Kollman charges, otherwise Mulliken charges. The panel names the
set it used. A calculation with no atomic charges gets a 1 Å arrow through the
centre of nuclear charge. The arrow colour is set with the colour button.

The arrow is drawn through atoms and bonds, so a diatomic's arrow can be seen
inside its bond.
