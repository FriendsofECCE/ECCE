# Vina: docking a ligand into a receptor

**Vina** is the code button for AutoDock Vina 1.2, a docking program. It places
a small molecule (the ligand) in a larger one (the receptor), tries many
positions and orientations, and ranks the poses it finds with an empirical
scoring function. Typical uses are a drug-like molecule in a protein pocket, or
a guest in a host cavity, as a quick first look before a quantum chemical
calculation.

Vina is not part of ECCE. On Debian it is the package `autodock-vina`. On the
**Codes** tab of the machine in **Register Machines**, register **Vina** and
set **Program** to the program, for example `/usr/bin/vina`; **Find** looks for
it. The Vina button in the code bar sits with the semi-empirical codes. The
code icon is a placeholder.

## The score

Vina reports a score for every pose, shown as **Vina score (kcal/mol)**. It is
an estimate of the binding affinity from a fitted function, not an energy from
a quantum calculation. A more negative score is a better pose. Scores are
never comparable with the energies of other codes, and ECCE does not offer to
compare them. Only compare Vina scores with other Vina scores from the same
receptor, and treat differences of less than about 1 kcal/mol as noise.

## Setting up a calculation

Build the whole system in the Builder, receptor and ligand together, and start
a **New Vina Docking Calculation**. There is one run type, **Docking (Vina)**.
The Runtype Details dialog holds these options.

**Receptor and Ligand.** The receptor and the ligand are told apart by atom
numbers.

- *Ligand atoms* are the atoms of the ligand, by their number in the Builder,
  written as a list such as `1-5,9`. Every other atom belongs to the receptor.
  Empty means the smallest of the separate molecules in the system, which is
  the guest in a host-guest pair. When the whole system is one molecule you
  must give the ligand atoms.
- *Ligand charge* is the charge of the ligand. Vina does not use it directly;
  it is kept with the calculation so the split is complete.

**Search Box.** Vina searches for the ligand only inside a box.

- *Centre the box on the ligand* is on by default. The box is then the
  ligand's own extent with 5 Å added on every side, and never less than 16 Å
  in any direction, so the ligand can move about its starting place. The six
  numbers below it are ignored.
- *Centre X, Y, Z* are the centre of the box in Å, in the coordinates of the
  structure. Used when the box is not centred on the ligand.
- *Size X, Y, Z* are the lengths of the box in Å. A box much larger than the
  pocket makes the search slower and less reliable; 20 to 25 Å is common for a
  protein pocket.

**Search.**

- *Exhaustiveness* is how much effort the search spends. The default is 8.
  Larger values (16 to 32) find the best pose more reliably and take
  proportionally longer. Use a higher value for a large ligand or a flexible
  one.
- *Number of poses* is the most poses Vina keeps, 9 by default.
- *Energy range* is how much worse than the best pose (in kcal/mol) a pose may
  be and still be kept, 3.0 by default.
- *Random seed* fixes the search so a run can be repeated. 0, the default,
  takes a different seed every time.

**Preparation.**

- *Amide bonds of the ligand may rotate* is off by default. An amide C-N bond
  is nearly a double bond and normally stays fixed.

## Results

The poses are stored with the calculation. They appear in the **Poses** panel
of the Builder, one row per pose with its Vina score, best first, and in the
Geometry Trace. Each pose is the whole system: the receptor stays where it
was and the ligand is moved to the pose. The atom order is the one of the
calculation. The Progress box shows the grid and the search, and the best
score when the run ends.

Hydrogens on carbon that Vina merges into their carbon (see below) are put
back in the stored poses from the ligand's own geometry. Their positions are
approximate and were not part of the scoring.

## How the molecules are prepared

Vina reads its molecules in the PDBQT format, which ECCE writes for you from
the structure. The preparation follows what AutoDock expects.

- Only polar hydrogens, those on N, O and S, are kept. Hydrogens on carbon are
  merged into their carbon, and their charge goes with it.
- Every atom gets an AutoDock atom type: aromatic and aliphatic carbon,
  hydrogen-bond donor and acceptor nitrogen and oxygen, and so on.
- Partial charges are Gasteiger charges.
- The receptor is rigid.
- The ligand is flexible. A bond can rotate when it is a single bond, outside
  a ring, and neither end is a terminal atom once the merged hydrogens are
  gone. Amide bonds are treated as rigid unless you tick the option above.
- Bonds are taken from the structure; when it holds none they are perceived
  from the geometry.
