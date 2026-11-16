# Match bands between two calculations

The **Vibrational Frequencies** panel can tell you which band of one
calculation is which band of another, and how far it moved. This answers
questions such as how much a C=O stretch drops when a cation binds, or whether
amide I stays where it was.

## The table

1. Open a calculation that has normal modes, click **Open in window...** and
   then **Compare...** in the **Other spectra** group.
2. Choose another calculation of the same folder that has normal modes, the
   entry marked **(match modes)**.
3. Nothing is typed. The smaller calculation is looked up in the larger one
   as a bonded unit, by element and bond, so a free ligand or anion is found
   again in its complex. Contacts to metal atoms do not count as bonds.

The line **Common atoms** above the table says which atoms were used, this
calculation's first: `S1 O2 O3 O4 O5 = S23 O25 O26 O22 O24` pairs the atoms in
that order. The **fit RMSD** is how well the free molecule lies on those atoms
after the best rotation; a value well above 0.2 Å means the molecule is
distorted in the complex or the match is doubtful. If the molecule occurs
several times (solvent around an ion) there is one line per copy. If it is
not found, a message says which molecule was looked for and where.

The window lists, per matched band, the mode number, frequency and IR
intensity in this calculation (A) and in the other (B), the **Shift** (B minus
A, in cm⁻¹) and the **Overlap**. Click a row to select and animate that mode in
this calculation. Bands that found no partner are listed under the table. **Split** shows how
many bands of the other calculation one band is paired with.

## How the bands are matched

B's structure is fitted onto A's over the common atoms, and B's modes are
turned the same way. Each mode's motion is mass-weighted. The **Overlap**
compares the free molecule's motion with the part of the other mode on the
common atoms, measured against that mode's whole motion: 1 when the mode is the
same motion and moves only the common atoms, lower when part of it is in the
rest of the complex (a ligand), 0 for unrelated motion. A ligand mode that
barely moves the common atoms is therefore not paired with a band of the free
molecule. Each band is paired with at most one band of
the other calculation, the largest overlaps first, and only above 0.5. With
several copies of the molecule, each band of the complex is paired with the
free band it resembles most, whatever the phase between the copies, so one free
band can pair with several bands (in-phase and out-of-phase combinations).

A band that moves none of the common atoms, such as a vibration of a bound
cation, has no partner and is listed as unmatched. A band that is split or
mixed with others between the two calculations may fall below 0.5 and then also
stays unmatched; it is not paired with a doubtful partner.

## From the command line

`ecce-bandshift <calcA> <calcB> [--threshold T]` prints the same table, and `--csv` or `--json` give it in those formats. A and B are
calculation folders.
`--atoms 1-12:5-16` replaces the automatic map with a typed one (A's atoms 1-12
are B's atoms 5-16); the mapped atoms must be the same elements.
