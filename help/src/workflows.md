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

## Making a workflow from a calculation

A calculation you have already set up or run can become the first step of a
workflow.

1. Right-click the calculation in the tree and choose **Make Workflow...**.
2. Give the workflow a name (the default is the calculation's name followed
   by `_workflow`) and choose **OK**.

The workflow is created in the calculation's folder and the calculation is
**moved** into it, not copied: its state, inputs, results and job record go
with it, and the original place is empty. The workflow view opens with the
calculation's box selected. A finished calculation's results are carried over
to the next step as for any finished step, so add the next steps from its box
or with **Apply Recipe**.

**Move into Workflow** in the same menu lists the workflows already in the
folder and moves the calculation into the chosen one as an unlinked step.

Both items are greyed out, with the reason as their tip, for a calculation
that is already in a workflow or that belongs to an MD study, and for one
whose job is submitted or running: its results are written back to where the
calculation is, so wait until the job has ended or terminate it.

From a terminal, with ECCE running:

    ecce-workflow make <calculation> [name]
    ecce-workflow move <calculation> <workflow>

## The view

Each step is a box with the code in its colour, the state of the calculation
(the icon and the name of the state, the same icon as in the Organizer tree;
see [Run states](run-states.md)) and the name
of the step. The first step is at the left; the steps that start from a step
are to its right, joined by an arrow labelled **geometry**; the label names what the
arrow carries, for example **geometry (maximum)** from a scan (see
Starting from a point of a scan, below).
Several arrows
from one box are branches.

Under the name, a small grey line gives the result of the step. A finished
optimisation or frequency step shows its total energy in hartree and, when it
has frequencies, how many are imaginary (below -1 cm-1); a finished CREST step
shows how many conformers, protonation sites, tautomers or clusters it found,
and the energy window of a conformer search. While a step runs the line is the
first line of its Progress box, for example **MTD 9 of 14**; a step that
stopped shows the reason given in its Progress box. The line is empty until
there is something to say.

Click a box to select it; the calculation is selected in the tree and the
table at the same time. Double-click a box to open the calculation in the
tree. The right-click menu of a box is that of the calculation.

The code buttons are at the right of the view. The text under the drawing
says what to do next.

## Canvas height and orientation

Drag the divider between the workflow view and the calculation table to change
the canvas height; the Organizer remembers the position.
Use **Change tree orientation** above the canvas to switch between left-to-right
and top-to-bottom flow; this choice is remembered separately from the Study view.

## Adding the first step

1. Click a code button, for example **xtb**.
2. Fill in the name and choose **Create**. The step has no earlier step, so
   nothing is carried over.
3. The Calculation Editor of the code opens on the new step. Set the
   structure in the Builder as for any calculation.

## Adding a step from another (branches)

1. Click the box the new step starts from.
2. Click the code button of the code the new step uses.
3. In the **New step** window (it opens however the step is made: from a
   code button of the view or from **New** in the tree):
   - **Name:** is the name of the calculation. It may contain letters,
     digits, `.`, `_` and `-`, and must differ from the names of the other
     steps.
   - **Starts from:** shows the code, name and state of the selected step.
   - A first step, which has no source, has no carry-over choices: the
     window says it starts from its own structure.
   - **Final geometry** gives the new step the last geometry of the
     selected step. If that step has no result yet, its input geometry is
     used and the window says so.
   - **Charge and multiplicity** carries the charge and the spin
     multiplicity.
   - **Start from the source's Hessian** (ORCA after ORCA, when the
     source has a stored Hessian; on by default then) copies the Hessian
     of the source step into the new step. If the new step is a geometry
     optimisation, its input reads that Hessian as the initial Hessian
     (**Initial Hessian** in the Details of the run type, unless you set
     it to something else). A Hessian from the gas phase is a good start
     for an optimisation in water.
   - **Orbitals as the starting guess** is not available yet.
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

## Starting from the best conformers of a CREST search

When the selected step is a CREST conformer search with a result, the
**New step** window also has **From the best conformers**, with a number
(3 to begin with, at most the number of conformers found). Ticking it makes
that many steps at once, side by side, each starting from one conformer: the
first from the conformer with the lowest relative energy, the second from the
next one, and so on. The arrows say which: **geometry (conformer 2)**. Each
step's Annotation gives the conformer's number and its relative energy.

For a cluster around an ion (for example Na+ with water) the window also has
**One per coordination pattern**. It makes one step for each pattern of
first-shell donor atoms found in the ensemble ("6 O", "5 O", "4 O" around
Na+), each starting from the lowest-energy conformer with that pattern, and
the Annotation names the pattern and how many conformers have it. The option
is not offered when the ensemble has no ion.

The relative energies belong to the CREST method alone, so they rank the
conformers of that search and are not energies to compare with a later
calculation. **Launch Ready Steps** launches all the steps at once.

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

**Launch Ready Steps** (the button beside the instructions below the diagram) launches every step
that has not been launched and whose source step is Complete, in one go. Each
step is launched with the launch settings it already stores, as in
[Launch Selected](launch-selected.md): the window lists the steps with the
machine, queue and cores, and a step without usable settings is listed in
grey with the reason and is not launched; choose **Open in Launcher** to
set it up.

Steps whose source is not Complete yet are named in the message under the
list and are not launched. Unless **Run the remaining steps automatically** is
ticked, nothing is launched when a job finishes: choose **Launch Ready Steps**
again when the steps before have completed.

### Run the remaining steps automatically

The checkbox **Run the remaining steps automatically** (under **Launch Ready
Steps**, and in the Apply Recipe panel) is stored with the workflow and is off
by default. When it is on, each step is launched as soon as the step it starts
from is Complete, with its stored launch settings and without a window. A step
that fails stops its branch: the steps after it keep waiting. A step is never
launched twice, and the Organizer shows a message when it launches steps for
you.

The launching is done by the Organizer. With no Organizer open, the chain
pauses, and it continues when you start the Organizer and open the workflow.

### The play button on a box

Each box has a small triangle at its lower right. It is green when the step
can be launched now, and grey otherwise; point at it to see why (waiting for a
source step, already run, or excluded). Click the green one to launch just that
step, with the settings it already stores.
