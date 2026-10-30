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
are to its right, joined by an arrow labelled **geometry**. Several arrows
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
