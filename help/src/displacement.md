# How far the atoms moved: the Displacement panel

The **Displacement** section of the Geometry Trace panel shows how far each
atom moved during a run, and how two calculations of the same atoms differ.
The numbers come from the command-line tool `ecce-displacement`, so the
same figures can be produced in a script.

## One calculation

1. Open the calculation in the Viewer and choose the **Geometry Trace**
   panel.
2. Open **Displacement** under the plot. The table has one row per atom:
   its number, its element, and the largest (**max**) and root-mean-square
   (**rms**) distance it was from its position in the first frame, in Å.
3. Choose **Export CSV...** to save the table.

The frames of a run share one coordinate frame, so nothing is aligned.

## Colour by displacement

Tick **Colour by displacement** to colour every atom by its **max**, from
pale yellow (no movement) through orange to dark red. The bar beside the box
gives the scale, ending at the largest displacement in Å. Untick it to get
the normal colours back. The colours also return when another panel takes
over the 3-D view.

## Comparing two calculations

1. Choose **Compare with...** and pick a second calculation from the same
   folder. Only calculations with the same number of atoms are offered. The
   atoms must be the same, in the same order.
2. The final structure of this calculation is fitted onto the last structure of
   the other one by a rotation and a shift (no mirror image), and the
   panel shows the aligned RMSD and each atom's displacement after the fit.
   Colour by displacement now uses these values.
3. Choose **(this calculation alone)** in the same dialog to go back to the
   trace of this calculation.

## Looking at part of the structure

The **Atoms** field restricts the table, the fit and the colours. Enter
atom numbers (`1 3 5`, counting from 1) or `within 3.5 of 1` for every atom
within 3.5 Å of atom 1, such as the pocket around an ion, and press Enter.
Atoms outside the selection are grey.
