# Match bands between two calculations

The **Vibrational Frequencies** panel can tell you which band of one
calculation is which band of another, and how far it moved. This answers
questions such as how much a C=O stretch drops when a cation binds, or whether
amide I stays where it was.

## The table

1. Open a calculation that has normal modes and click **Match bands...**.
2. Choose another calculation of the same folder that has normal modes.
3. If the two have different numbers of atoms, give the common atoms, this
   calculation's first, counted from 1. `1-12:5-16` maps this calculation's
   atoms 1-12 to the other one's atoms 5-16, for instance a free ligand and
   the same ligand in a complex. `1-12` names the same atoms in both. When
   both have the same atoms, nothing is asked and all atoms are used.

The window lists, per matched band, the mode number, frequency and IR
intensity in this calculation (A) and in the other (B), the **Shift** (B minus
A, in cm-1) and the **Overlap**. Click a row to select and animate that mode in
this calculation. Bands that found no partner are listed under the table.

## How the bands are matched

B's structure is fitted onto A's over the common atoms, and B's modes are
turned the same way. Each mode's motion is mass-weighted, cut down to the
common atoms and scaled to length one, so the **Overlap** of two modes is the
cosine of the angle between the motions of the common atoms: 1 for the same
motion, 0 for unrelated motion. Each band is paired with at most one band of
the other calculation, the largest overlaps first, and only above 0.5.

A band that moves none of the common atoms, such as a vibration of a bound
cation, has no partner and is listed as unmatched. A band that is split or
mixed with others between the two calculations may fall below 0.5 and then also
stays unmatched; it is not paired with a doubtful partner.

## From the command line

`ecce-bandshift <calcA> <calcB> [--atoms SET] [--threshold T]` prints the same
table, and `--csv` or `--json` give it in those formats. A and B are
calculation folders.
