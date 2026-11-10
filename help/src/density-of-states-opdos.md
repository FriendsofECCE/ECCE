# Density of states (OPDOS)

The **Density of States (OPDOS)** panel plots, against orbital energy, the total density of states (TDOS), the partial density of states (PDOS) of two groups of atoms, **A** and **B**, and the overlap population density of states OPDOS(A,B) of one calculation. Open it from the **Properties** list, in the Orbitals group. It needs a calculation with stored orbitals (`results.h5`).

For MO *i* with coefficients c, the overlap population between A and B is OP(A,B) = 2 Σ c(μ,i) c(ν,i) S(μ,ν), summed over the basis functions μ on A and ν on B. OPDOS(E) adds one broadened peak for each orbital, weighted by OP(A,B). Positive is bonding between A and B at that energy, negative is antibonding, near zero is non-bonding. PDOS(X) weights each orbital by the Mulliken gross population on X.

The panel says which population scheme it uses: **Mulliken**, for both OPDOS and PDOS. Mulliken populations depend on the basis set, so compare calculations only with the same basis.

## Controls

- **A** and **B**: the atom numbers of the two groups as shown in the structure, for example `1,2,5-8`. Press Enter to apply. A starts as the first transition-metal atom (or the first atom) and B as all other atoms. No atom may be in both groups.
- **Use selection** (under each group's box): take the atoms selected in the 3-D view for that group. The other group is taken as it stands in its box, typed or not. An atom can belong to only one group: if it is in both, nothing is changed and the message names the atom.
- **Width (eV)**: the full width at half maximum of the peak drawn for each orbital, 0.30 eV to begin with. A narrow width resolves single orbitals, a wide one shows bands.
- **Shape**: Gaussian (the default) or Lorentzian peaks, both of area 1. Lorentzian peaks have long tails, so neighbouring orbitals overlap more and a little of each peak lies outside the energy range shown.
- **Sticks**: one thin line per orbital at its energy, as high as its OPDOS contribution.
- **Integrated OPDOS**: the running integral of OPDOS in a second plot. At the HOMO it equals the total overlap population between A and B (the sum of occupation times OP(A,B) over the orbitals); the text under the plots gives that number.
- **Occupied only**: leave out the empty orbitals. Mulliken overlap populations of virtual orbitals in a diffuse basis are large, of both signs, and cancel, so the empty part of the curves can be misleading; this switch removes them.
- **From** and **To** (eV): the energy range shown, in both plots; the vertical scale fits the part inside. **Reset** shows the whole range again. The range is remembered for this panel, so the region around the HOMO stays enlarged the next time it opens.
- **Export CSV** writes all curves on the energy grid; **Export PNG** saves the plot.

The occupied energy region is shaded and the HOMO is marked with a dashed line. For an open-shell calculation (UHF, UKS) the alpha and beta orbitals are counted separately, once each: TDOS, PDOS and OPDOS of the alpha orbitals are drawn upwards and those of the beta orbitals downwards (dashed), and the integrated OPDOS has one curve per spin. The occupied A–B overlap population in the table is the sum of the two.

The explanation below the plots is formatted text: the formula OPDOS(A,B) = Σᵢ 2 Σ c c S g(E − εᵢ) with the sum over μ in A and ν in B, what each symbol means, a small table with A, B, the occupied A–B overlap population (in electrons) and the largest deviation of CᵀSC from 1, and the caution about diffuse basis sets on its own highlighted line. Messages (a refused atom list, the orbitals listed after a click) appear in the small text field under it, never among the controls.

The first time a calculation is opened, building the overlap matrix and the populations for a large basis can take a while. A progress dialog appears after half a second and names the stage (reading the stored results, building the overlap matrix of the basis, computing the overlap populations). The overlap matrix is kept while the panel is open, so changing A or B afterwards is faster.

Both plots have axis titles: Energy (eV) on x; states / eV (upper) and Integrated OPDOS (electrons) (lower) on y.

## Reading the plot

Click on a curve: a list under the plots shows the orbitals within one width of that energy, nearest first, with their number, spin (open shell), energy, occupation, OP(A,B) and gross populations on A and B, so a peak can be traced to its orbitals.

Double-click an orbital in the list, or select it and press **Draw orbital**, to draw its isosurface in the 3-D view, as a click on a level of the MO diagram does. The surface is computed by the MOs panel with its current settings (isovalue, grid); for an open-shell calculation the orbital of the listed spin is drawn.

## Checks and limits

- Before drawing, the panel checks that every orbital is normalised in the basis (cᵀSc = 1). If the largest deviation is above 0.001, the coefficients do not match the basis functions for that code: the panel draws nothing and its message field names the deviation and the orbital where it is largest. Between 0.0001 and 0.001 the curves are drawn, with a warning in the message field that the populations are approximate.
- Semiempirical calculations (MOPAC) use an orthonormal basis, so the Mulliken overlap population between atoms is zero for every orbital. The panel says so instead of drawing an empty OPDOS. A bond-order density of states is not available yet.
- The same numbers come from the command `ecce-fragorb dos <calculation> --set A=1 --set B=2-13 --pair A:B`.
