# Importing a structure

You can use ECCE without a compute machine and without any computational
chemistry code installed. This page shows how to open a structure you
already have, or take one from the Structure Library, in the Builder,
which you also use in [Your first calculation](first-calculation.md). To
look at the results of a calculation that was run elsewhere, see
[Importing a calculation](importing-a-calculation.md).

You need ECCE started with `ecce` (see [Installation](installation.md)).

## Open a structure file

The Builder reads structure files in these formats:

| Format | File extensions |
|---|---|
| XYZ | `.xyz` |
| PDB | `.pdb`, `.ent` |
| Biosym/Accelrys CAR | `.car` |
| Crystallographic Information File (CIF) | `.cif` |
| ECCE MVM (ECCE's own format) | `.mvm` |

### Start the Builder without a calculation

1. In the Organizer, choose **File > New Structure...** (Ctrl+N).

The Builder opens on an empty structure that belongs to no calculation.
You can also open the Builder on a calculation, as in
[Your first calculation](first-calculation.md).

### Load a file into the Builder

1. In the Builder, choose **File > Add Structure from File...**.
2. In the file window, choose
   **Local Filesystem** in the server list at the top-left. The same list
   offers the data servers ECCE knows about.
3. Go to the folder with your file. **Files of type:** lists the formats
   above; "All Supported Types" shows all of them.
4. Select the file and click **OK**.

The structure is added to what is already in the Builder, so start from an
empty structure if you want only the file.

What happens next depends on the format:

- XYZ: ECCE asks for the units of the coordinates (the file does not
  record them): Ångströms, Bohr, picometers or nanometers.
- PDB: if the file has several models or alternate locations, the window
  "ECCE PDB Reader" asks which to use: **Read which model:**, **Alternate
  location:** and **Select chain:** (`All` reads every chain). A file with
  a single model and no alternate locations, such as glycine, loads without
  a question.
- CIF: loaded without a question. ECCE builds the whole unit cell (1 x 1 x 1)
  from the atoms and symmetry operations in the file, or from the space-group
  name when the file lists no operations. A site shared by several species
  keeps the one with the highest occupancy. The Builder's message area warns
  about partially occupied sites, about a structure without hydrogen atoms,
  and when the file has more than one data block (only the first is read). A
  file that cannot be read gives an error message. **File > Save As...** with
  the type CIF, P1 writes the cell and every atom as P1, which reads back as the
  same structure.
- CAR and MVM: loaded without a question. A CAR file such as `benzene.car`
  (12 atoms) loads directly.

The unit cell of a periodic CAR file is not drawn when you open the file.
Choose **Tools > Periodic Builder**; **Show Lattice** is ticked there by
default and draws the cell.

For XYZ, PDB and CAR files ECCE works out which atoms are bonded from the
coordinates. MVM files carry their own bonds. Every bond is drawn as a single
stick whatever order a file gives it; see [Bonds in the
Builder](bonds-and-clean.md).

To work on a file directly instead of adding it to a structure, choose
**File > Open...** (Ctrl+O) and pick the file the same way. The Builder
opens it as its own structure. The **Open structures** panel at the top
right lists every structure that is open; click an entry to switch to it.
**File > New** starts another empty structure and **File > Close** closes
the current one.

The same window also lists `.cube` and `.trj` files. A Gaussian cube file
opens as a structure, with a **Cube File** panel that shows its grids.
A trajectory (`.trj` or multi-frame `.xyz`) also opens with **File >
Open...**; **Add Structure from File...** reads only the four formats in
the table.

### Build from the structure library

The Structure Library holds ready-made molecules and fragments. It has the
libraries `Small_organic_molecules` (organised by compound class, for example
`Alicycles`, `Arenes`, `Heterocycles`), `Natural_products_and_drugs` (`Alkaloids`,
`Antibiotics`, `Sugars`, `Vitamins`), `Biomolecules` (`Amino_acids`, `DNA`, `RNA`),
`Inorganic` (`Ligands`, `Polyoxometalates`) and `Teaching` (see [Molecules for teaching](molecules-for-teaching.md)). Use it as in step 3 of
[Your first calculation](first-calculation.md#3-build-the-molecule):

1. Choose **Mode > Add Structure** (Ctrl+7).
2. In the **Structure Library** panel, open a folder and click a
   structure. A preview appears.
3. Click in the empty 3D view to add it. To attach it to a structure that
   is already there, first select the site to bond it to (see below).

A fragment needs a free site. In benzene every carbon is saturated, so
select the hydrogen that the fragment should replace before you click.
Otherwise ECCE says "Cannot unambiguously connect fragments. Please select
the sites that should be bonded."

You can draw atoms with **Mode > Atom** (Ctrl+5), and finish with **Build >
Add Hydrogen** and **Build > Clean**, as in the first calculation.

A structure from a PDB file has no free sites, so **Build > Add Hydrogen**
adds hydrogens to its amino-acid and nucleotide residues from the residue
templates. The status line gives the number of hydrogens added and names the
residues that got none (water). A ligand without a template, such as benzene,
gets its hydrogens from its geometry (aromatic rings are recognised by their
bond lengths).

A structure with no residues and no free sites, such as heavy atoms read from
an XYZ or CIF file, gets its hydrogens the same way from its geometry. If you
draw a fragment with free sites onto a PDB structure, **Add Hydrogen** fills
those sites and the residues in one step; Ctrl+Z undoes both.

## Select parts of a system

The **Edit** menu selects parts of a large system, such as a protein in
water from GROMACS:

- **Select All** (Ctrl+A) selects every atom.
- **Select Molecule** selects each molecule that has a selected atom. With
  nothing selected, it selects the solute: every atom that is neither
  solvent nor an ion.
- **Select Solvent** selects the water molecules. Water is recognised by
  its residue name (SOL, HOH, WAT, TIP3, TIP4, SPC, H2O and others), and
  by the solvent that **Solvate** adds. Ions are not selected.
- **Select Ions** selects ions: residues of one atom, and the usual ion
  names (NA, CL, K, MG, CA, ZN, NA+, CL- and others). In a file without
  residues, an unbonded atom of an ion element counts as an ion.
- **Select Backbone** and **Select Side Chains** select those parts of a
  protein or nucleic acid.
- **Invert Selection** (Ctrl+R) and **Clear Selection** (Ctrl+U).

A style from **Render > Style** applies to the selected atoms only. With
nothing selected, it applies to every atom. To draw the water as wireframe
and keep the protein as it is, choose **Select Solvent**, then **Render >
Style > Wireframe**.

## Cut a cluster out of a periodic structure

**Build > Cut Cluster...** turns a structure with a lattice (a CIF, or a
CP2K or Quantum ESPRESSO model) into a non-periodic cluster. Select the atom
or atoms to centre on first; the centre is their mean position. Enter the
radius in Å (0.5-50) and choose what to keep:

- **Cut through bonds** keeps exactly the atoms within the radius.
- **Keep whole molecules** keeps every bonded molecule that has an atom within
  the radius. Bonded networks, such as quartz, extend through the whole
  crystal and are cut at the radius as before.
- **Keep whole formula units** also trims the network part, outermost atoms
  first, to a whole number of formula units.

Atoms across the cell boundary are included: the cell is replicated as far as
the radius reaches. **Capping** adds a hydrogen (standard element-H distance,
for example C-H 1.09 Å, Si-H 1.48 Å) or a hydroxyl group on each bond the cut
severed, along the direction of that bond. An atom outside the radius that is bonded
to two or more cluster atoms is taken into the cluster first, so each cap
replaces a different atom; the cluster can therefore reach about two bonds
past the radius. The result replaces the structure,
without a lattice, and **Edit > Undo** brings the periodic structure back.

## Fill a box with solvent

**Build > Solvate...** surrounds the structure with solvent molecules, for any
code: GROMACS, LAMMPS, CP2K or Quantum ESPRESSO boxes, or a cluster model
with explicit solvent for ORCA, xTB or ECCE-QM. It needs no other program.

- **Solvent box** is one of the pre-equilibrated boxes ECCE ships
  (water216 and water64, both water at about 0.98 g/cm3). Copies of the box
  are laid side by side until the cell is covered.
- If the structure has a periodic cell, that cell is filled and the atoms
  stay where they are. Otherwise a rectangular cell is made that is the
  structure's extent plus the **Margin** (Å) on each side, and the structure
  is moved to its centre.
- A solvent molecule is left out when one of its atoms is closer to an atom
  of the structure than the **Overlap scale** times the sum of the two van
  der Waals radii (1.0 by default; a smaller value packs the solvent closer).
  Where the cell is not a whole number of solvent boxes, molecules that
  would clash with the next periodic image are also left out, so the density
  near the cell faces is a few percent lower.

Each water becomes its own HOH residue with atoms OW, HW1 and HW2, so
**GROMACS Prepare** takes the waters as they are: it skips `gmx solvate`,
keeps the cell, and its output says so. The result is one step, and
**Edit > Undo** removes the solvent again.

## Select within a distance

**Edit > Select Within Distance...** extends a selection to nearby atoms.
Select at least one atom first, then enter a centre-to-centre distance in
Å (0.1–50, initially 3.5). The Builder remembers the distance after OK.
An atom qualifies if it is at or within that distance of any originally
selected atom. Periodic structures use the nearest periodic image.

Choose **Atoms** to select qualifying atoms, or **Whole molecules** to
include every atom in each qualifying bonded fragment, as in Select Molecule.
Leave **Keep the original selection** checked to extend the selection;
uncheck it to select only new neighbours, excluding the originally selected
atoms even after whole-molecule expansion. For a hydration shell, select
Na+ or Cl− and choose Whole molecules to include complete nearby waters.
**OK** applies the selection as one undoable action; **Cancel** leaves it alone.

## Where the data is kept

A structure opened from a file with **File > Open...** can be changed, and
then **File > Save** is enabled. It writes back into that file, in ECCE's
format. The original file changes only when you save.

For a new structure that has no file, **File > Save** is greyed out because
there is nowhere to save it, and Ctrl+S asks where to save, as **File > Save
As...** does. It offers the
structure formats CAR, CIF, MVM, NWChem, PDB and XYZ, and calculation types. A
calculation can be saved into a project on the data server or into any
folder on your disk. A calculation saved in a folder can be reopened with
**File > Open...** and launched like any other.

Structures you save into a project are kept with your other data: the data server, or the local folder if you use local data mode (see [Installation](installation.md)).

## Exporting the structure to a file

**File > Export Structure...** writes the structure for another program. A
`.gro` (GROMACS) or `.pdb` file keeps the residues and the periodic cell, so a
Layer Builder stack keeps its layers, its water (residue HOH, atoms OW, HW1,
HW2) and its ions (one residue each, named by the element). An `.xyz` file has
only elements and positions. Atoms that have no residue are written as HOH
when they form a water, as their own residue when they are a lone ion, and
otherwise as one residue UNK. To type such a structure for GROMACS with MINFF
inside ECCE, use GROMACS Prepare with **Mineral from: the Builder** (see
[GROMACS Prepare](gromacs-prepare.md)).

## Saving a picture of the 3-D view

**File > Save Image...** writes a PNG of the 3-D view for a paper or a slide.
Choose the resolution as 1x, 2x (the default) or 4x the size of the view, say
whether the background should be white, and pick the file. Transparent
surfaces such as orbital lobes are drawn blended with what is behind them
instead of as a dotted pattern, edges are smoothed, and lines are thicker in
a larger image so it does not look like a shrunken screenshot. At 2x or 4x
the orbital or density on show is recomputed on a finer grid for the image
(the view on screen is left as it was).

## Next steps

To run a calculation yourself, continue with [Machines](machines.md) and
[Your first calculation](first-calculation.md).
