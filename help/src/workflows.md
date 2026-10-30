# Workflows

A workflow is a set of calculations in which each step starts from the
geometry of an earlier one. The Organizer draws the steps as boxes joined by
arrows, so a series such as a cheap xtb search followed by DFT refinements of
the best structures can be set up, launched and followed in one place.
Calculations outside a workflow are not affected.

A workflow can hold calculations of ECCE-QM, ORCA, Gaussian-16, NWChem,
MOPAC, xTB, Quantum ESPRESSO and CP2K, in any mixture.

## Making a workflow

1. Select a project in the tree.
2. Choose **File > New > Workflow...** (or **New > Workflow...** in the
   right-click menu of the project).
3. The workflow appears in the project. Selecting it shows the workflow view
   above and the table of its calculations below.

## The view

Each step is a box with the code in its colour, the state of the calculation
(a coloured dot and its name, as in [Run states](run-states.md)) and the name
of the step. The first step is at the left; the steps that start from a step
are to its right, joined by an arrow labelled **geometry**; the label names what the
arrow carries, for example **geometry (maximum)** from a scan (see
Starting from a point of a scan, below).
Several arrows
from one box are branches.

Click a box to select it; the calculation is selected in the tree and the
table at the same time. Double-click a box to open the calculation in the
tree. The right-click menu of a box is that of the calculation.

The code buttons are at the right of the view. The text under the drawing
says what to do next.

## Adding the first step

1. Click a code button, for example **xtb**.
2. Fill in the name and choose **Create**. The step has no earlier step, so
   nothing is carried over.
3. The Calculation Editor of the code opens on the new step. Set the
   structure in the Builder as for any calculation.

## Adding a step from another (branches)

1. Click the box the new step starts from.
2. Click the code button of the code the new step uses.
3. In the **New step** window:
   - **Name:** is the name of the calculation. It may contain letters,
     digits, `.`, `_` and `-`, and must differ from the names of the other
     steps.
   - **Starts from:** shows the code, name and state of the selected step.
   - **Final geometry** gives the new step the last geometry of the
     selected step. If that step has no result yet, its input geometry is
     used and the window says so.
   - **Charge and multiplicity** carries the charge and the spin
     multiplicity.
   - **Orbitals as the starting guess** and **Hessian** are not available
     yet.
4. Choose **Create**. The new calculation is made inside the workflow and
   linked to the selected step, and the Calculation Editor of its code opens
   with these values filled in.

Doing this several times from one box makes several steps that start from
the same geometry. A step can be set up before the step it starts from has
finished, using the input geometry of that step; set it up again, or
replace the geometry in the Builder, once the earlier step is Complete.

## Starting from a point of a scan

When the selected step is a relaxed scan, or an IRC, with a structure for
every point, the **New step** window has a **Structure of the scan** choice
instead of the final geometry:

- **Last point** is the end of the scan.
- **Maximum (highest energy)** is the point with the highest energy, the
  usual guess for a transition state.
- **Point number** is the point with that number in the scan's plot.

The new step starts from the structure of that point, and the arrow says
which: **geometry (last point)**, **geometry (maximum)** or **geometry
(point 5)**. If the scan has no such point, no step is made and the message
says so. Other steps are not offered the choice and carry their final
geometry.

## Joining steps

A transition-state search needs several structures: a reactant, a product
and, optionally, a guess for the transition state. A **join** is a step that
takes them from several steps at once.

1. Click the first box, then **Ctrl-click** the other boxes (two or three
   in all). Ctrl-click on a picked box takes it out again; a click without
   Ctrl leaves only that box.
2. Click a code button: **G16** or **ORCA**. Other codes cannot be joined.
3. In the **New join** window give the step a name and check the role of
   each source: **reactant**, **product** or **TS guess**. There must be
   one reactant and one product, and at most one TS guess. A source that is
   a scan has a choice of point as above. The window shows the search that
   will be made:
   - G16 with two sources: **QST2**. With three: **QST3**.
   - ORCA: **NEB-TS**, with the TS guess optional.
4. Choose **Create**. The step is made with the run type
   Transition State + Frequencies of the code, its own structure is the
   reactant's, and its charge and multiplicity are the reactant's. One arrow
   from each source goes to it, labelled with the role and the point, for
   example **TS guess (maximum)**.

The Calculation Editor opens on the join. Its **TS Search** box shows the
structures taken from the sources; they are matched atom by atom and renumbered
as for any other TS Search,
so sources that list their atoms in different orders are handled there. A
join starts only when all its sources are Complete: **Launch Ready Steps**
treats it like any step.

## Example: an SN2 reaction

**Examples > Workflows > SN2 Cl- + CH3Br** is a workflow of Gaussian 16
calculations at B3LYP/6-31+G(d), charge -1, for Cl- + CH3Br giving CH3Cl +
Br-:

1. the reactant complex, Opt+Freq;
2. a relaxed scan of the C...Cl distance from the reactant's geometry;
3. the product complex, Opt+Freq, from the last point of the scan;
4. a join of the reactant, the product and the maximum of the scan as TS
   guess: QST3 + Freq, with one imaginary frequency;
5. an IRC from the result of the QST3.

Open the calculations from the boxes to see the energies, the scan plot and
the vibrational modes.

## Launch Ready Steps

**Launch Ready Steps** (at the lower right of the view) launches every step
that has not been launched and whose source step is Complete, in one go. Each
step is launched with the launch settings it already stores, as in
[Launch Selected](launch-selected.md): the window lists the steps with the
machine, queue and cores, and a step without usable settings is listed in
grey with the reason and is not launched; choose **Open in Launcher** to
set it up.

Steps whose source is not Complete yet are named in the message under the
list and are not launched. Nothing is launched when a job finishes: choose
**Launch Ready Steps** again when the steps before have completed.
