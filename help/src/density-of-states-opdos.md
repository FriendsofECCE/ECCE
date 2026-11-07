# Density of states (OPDOS)

The **Density of States (OPDOS)** panel plots, against orbital energy, the total density of states (TDOS), the partial density of states (PDOS) of two groups of atoms, **A** and **B**, and the overlap population density of states OPDOS(A,B) of one calculation. Open it from the **Properties** list, in the Orbitals group. It needs a calculation with stored orbitals (`results.h5`).

For MO *i* with coefficients c, the overlap population between A and B is OP(A,B) = 2 Σ c(μ,i) c(ν,i) S(μ,ν), summed over the basis functions μ on A and ν on B. OPDOS(E) adds one broadened peak for each orbital, weighted by OP(A,B). Positive is bonding between A and B at that energy, negative is antibonding, near zero is non-bonding. PDOS(X) weights each orbital by the Mulliken gross population on X.

The panel says which population scheme it uses: **Mulliken**, for both OPDOS and PDOS. Mulliken populations depend on the basis set, so compare calculations only with the same basis.

## Controls

- **A** and **B**: the atom numbers of the two groups as shown in the structure, for example `1,2,5-8`. Press Enter to apply. A starts as the first transition-metal atom (or the first atom) and B as all other atoms. No atom may be in both groups.
- **Use selection** (next to each group): take the atoms selected in the 3-D view for that group.
- **Width (eV)**: the full width at half maximum of the Gaussian drawn for each orbital, 0.30 eV to begin with. A narrow width resolves single orbitals, a wide one shows bands.
- **Sticks**: one thin line per orbital at its energy, as high as its OPDOS contribution.
- **Integrated OPDOS**: the running integral of OPDOS in a second plot. At the HOMO it equals the total overlap population between A and B (the sum of occupation times OP(A,B) over the orbitals); the text under the plots gives that number.
- **Occupied only**: leave out the empty orbitals. Mulliken overlap populations of virtual orbitals in a diffuse basis are large, of both signs, and cancel, so the empty part of the curves can be misleading; this switch removes them.
- **Export CSV** writes all curves on the energy grid; **Export PNG** saves the plot.

The occupied energy region is shaded and the HOMO is marked with a dashed line. For an open-shell calculation the alpha curves are drawn upwards and the beta curves downwards (dashed).

## Reading the plot

Click on a curve: the text area lists the orbitals within one width of that energy, with their energy, occupation, gross populations on A and B and OP(A,B), so a peak can be traced to its orbitals.

## Checks and limits

- Before drawing, the panel checks that every orbital is normalised in the basis (cᵀSc = 1). If not, the coefficients do not match the basis functions for that code and the panel refuses with a message.
- Semiempirical calculations (MOPAC) use an orthonormal basis, so the Mulliken overlap population between atoms is zero for every orbital. The panel says so instead of drawing an empty OPDOS. A bond-order density of states is not available yet.
- The same numbers come from the command `ecce-fragorb dos <calculation> --set A=1 --set B=2-13 --pair A:B`.
