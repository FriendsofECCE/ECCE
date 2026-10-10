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
- CAR and MVM: loaded without a question. A CAR file such as `benzene.car`
  (12 atoms) loads directly.

The unit cell of a periodic CAR file is not drawn when you open the file.
Choose **Tools > Periodic Builder**; **Show Lattice** is ticked there by
default and draws the cell.

For XYZ, PDB and CAR files ECCE works out the bond orders from the
coordinates. MVM files carry their own bonds.

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
libraries `SimpleStructures` (organised by compound class, for example
`Alicycles`, `Amines`, `Sugars`, `Vitamins`), `Amino_Acids`, `DNA_Bases`,
`RNA_Bases` and `Teaching` (see [Molecules for teaching](molecules-for-teaching.md)). Use it as in step 3 of
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

## Where the data is kept

A structure opened from a file with **File > Open...** can be changed, and
then **File > Save** is enabled. It writes back into that file, in ECCE's
format. The original file changes only when you save.

For a new structure that has no file, **File > Save** is greyed out because
there is nowhere to save it. Use **File > Save As...**. It offers the
structure formats CAR, MVM, NWChem, PDB and XYZ, and calculation types. A
calculation can be saved into a project on the data server or into any
folder on your disk. A calculation saved in a folder can be reopened with
**File > Open...** and launched like any other.

Structures you save into a project are kept with your other data: the data server, or the local folder if you use local data mode (see [Installation](installation.md)).

## Next steps

To run a calculation yourself, continue with [Machines](machines.md) and
[Your first calculation](first-calculation.md).
